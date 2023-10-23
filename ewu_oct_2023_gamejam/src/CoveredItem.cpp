#include "CoveredItem.h"

#include <iostream>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include "Camera.h"
#include "AudioEngine.h"
#include "VkglTFModel.h"
#include "InputManager.h"
#include "RenderObject.h"
#include "PhysicsEngine.h"
#include "EntityManager.h"
#include "DataSerialization.h"
#include "GlobalState.h"
#include "imgui/imgui.h"


struct CoveredItem_XData
{
    RenderObjectManager* rom;
    Camera* camera;
    RenderObject* renderObj = nullptr;
    vec3 position = GLM_VEC3_ZERO_INIT;
    float_t yRotation = 0.0f;

    struct ItemType
    {
        std::string modelPath;
        vec3 collisionBoxExtent;
        vec3 interactionOrigin;
        float_t interactionRadius;
        float_t interactionIconYoff;
        vec2 uncoverTimeMinMax;
    };
    static size_t numItemTypes;
    static ItemType* itemTypes;

    size_t itemTypeId = 0;
    JPH::BodyID collisionBoxBodyId;
    bool prevIsInteractible = false;  // Whether the player position is within the interaction field.

    float_t uncoverTimer;
    float_t chosenUncoverTime;
    bool isUncovering = false;
    bool isCovered = true;

    int8_t dateId = -1;  // -1 is no date, then 0,1,2,3... is the date id.

    float_t rustleTimer = 0.0f;
    bool showInteractionCursor = false;
    RenderObject* interactionCursorRenderObj = nullptr;
    RenderObject* uncoverProgressRenderObj = nullptr;

#ifdef _DEVELOP
    bool requestChangeItemModel = false;
#endif
};

size_t CoveredItem_XData::numItemTypes = 1;
CoveredItem_XData::ItemType* CoveredItem_XData::itemTypes = new CoveredItem_XData::ItemType[] {  // @NOTE: this is not a std::vector bc for some reason this causes a compiler error on MSVC 14.35.32215 even with /Od optimization flags.  -Timo 2023/10/22
    {
        .modelPath = "CIClock",
        .collisionBoxExtent = { 1.0f, 1.0f, 1.0f },
        .interactionOrigin = { 0.0f, 0.0f, 0.0f },
        .interactionRadius = 4.0f,
        .interactionIconYoff = 4.0f,
        .uncoverTimeMinMax = { 0.5f, 0.75f },
    }
};

inline CoveredItem_XData::ItemType& myItemType(CoveredItem_XData* d)
{
    return CoveredItem_XData::itemTypes[d->itemTypeId];
}

void createInteractionROs(CoveredItem_XData* d, CoveredItem* _this, const std::string& myGuid)
{
    vkglTF::Model* interactionModel = d->rom->getModel("InteractionMarker", _this, []() {});
    vkglTF::Model* progressDialModel = d->rom->getModel("ProgressDial", _this, []() {});
    std::vector<vkglTF::Animator::AnimatorCallback> callbacks = {
        {
            "GoInvisible", [d]() {
                d->uncoverProgressRenderObj->renderLayer = RenderLayer::INVISIBLE;
            }
        }
    };
    d->rom->registerRenderObjects({
            {
                .model = interactionModel,
                .renderLayer = RenderLayer::INVISIBLE,
                .attachedEntityGuid = myGuid,
            },
            {
                .model = progressDialModel,
                .animator = new vkglTF::Animator(progressDialModel, callbacks),
                .renderLayer = RenderLayer::INVISIBLE,
                .attachedEntityGuid = myGuid,
            }
        },
        {
            &d->interactionCursorRenderObj,
            &d->uncoverProgressRenderObj
        }
    );
}

CoveredItem::CoveredItem(EntityManager* em, RenderObjectManager* rom, Camera* camera, DataSerialized* ds) : Entity(em, ds), _data(new CoveredItem_XData())
{
    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;

    _data->rom = rom;
    _data->camera = camera;

    if (ds)
        load(*ds);

    _data->requestChangeItemModel = true;
    createInteractionROs(_data, this, getGUID());
}

CoveredItem::~CoveredItem()
{
    delete _data->renderObj->animator;
    delete _data->interactionCursorRenderObj->animator;
    delete _data->uncoverProgressRenderObj->animator;
    _data->rom->unregisterRenderObjects({
        _data->renderObj,
        _data->interactionCursorRenderObj,
        _data->uncoverProgressRenderObj
    });
    _data->rom->removeModelCallbacks(this);

    delete _data;
}

void CoveredItem::physicsUpdate(const float_t& physicsDeltaTime)
{
    // Process uncovering item.
    if (_data->isUncovering)
    {
        if (!input::keyEPressed)
        {
            _data->uncoverProgressRenderObj->renderLayer = RenderLayer::INVISIBLE;
            _data->isUncovering = false;  // Exit if not holding uncover button.
        }
        else
        {
            // Count up timer.
            _data->uncoverTimer += physicsDeltaTime;
            if (_data->uncoverTimer > _data->chosenUncoverTime)
            {
                // Uncover!
                _data->renderObj->animator->setTrigger("goto_uncover");
                _data->isUncovering = false;  // @NOTE: don't set progress render obj to invisible bc want the "finish-disappear" anim to play and trigger the `GoInvisible` event.
                _data->isCovered = false;
                if (_data->dateId >= 0)
                {
                    // @TODO: implement if there's a date and goto the next phase.
                }
            }
        }
    }

    // Check whether this is at an interactible distance away.
    if (!globalState::playerGUID.empty() &&
        globalState::playerPositionRef != nullptr)
    {
        // @DEBUG: draw line showing distance to player.
        {
            vec3 origin;
            glm_vec3_add(_data->position, myItemType(_data).interactionOrigin, origin);
            vec3 delta;
            glm_vec3_sub(*globalState::playerPositionRef, origin, delta);
            glm_vec3_scale_as(delta, myItemType(_data).interactionRadius, delta);
            vec3 pt2;
            glm_vec3_add(origin, delta, pt2);
            physengine::drawDebugVisLine(origin, pt2, physengine::DebugVisLineType::KIKKOARMY);
        }

        bool isInteractible = (_data->isCovered && glm_vec3_distance2(*globalState::playerPositionRef, _data->position) < std::pow(myItemType(_data).interactionRadius, 2.0f));
        if (isInteractible)
        {
            DataSerializer msg;
            msg.dumpString("msg_request_interaction");
            msg.dumpString(getGUID());
            msg.dumpString("uncover");
            DataSerialized ds = msg.getSerializedData();
            _em->sendMessage(globalState::playerGUID, ds);
        }
        else if (_data->prevIsInteractible && !isInteractible)
        {
            DataSerializer msg;
            msg.dumpString("msg_remove_interaction_request");
            msg.dumpString(getGUID());
            DataSerialized ds = msg.getSerializedData();
            _em->sendMessage(globalState::playerGUID, ds);
        }
        _data->prevIsInteractible = isInteractible;
    }

    // Update interaction cursor render obj.
    if (_data->showInteractionCursor)
    {
        _data->interactionCursorRenderObj->renderLayer = (!_data->isUncovering ? RenderLayer::VISIBLE : RenderLayer::INVISIBLE);
        _data->showInteractionCursor = false;
    }
    else
        _data->interactionCursorRenderObj->renderLayer = RenderLayer::INVISIBLE;
}

void CoveredItem::update(const float_t& deltaTime)
{
    if (_data->requestChangeItemModel)
    {
        // Create Render object.
        if (_data->renderObj != nullptr)
        {
            // @BUG: validation error occurs right here... though, it'd just for development so not too much of a concern.
            delete _data->renderObj->animator;
            _data->rom->unregisterRenderObjects({ _data->renderObj });
            _data->rom->removeModelCallbacks(this);
        }

        vkglTF::Model* model = _data->rom->getModel(myItemType(_data).modelPath, this, []() {});
        std::vector<vkglTF::Animator::AnimatorCallback> callbacks = {
            {
                "RustleSfx", [&]() {
                    // AudioEngine::getInstance().playSound()
                }
            },
            {
                "UncoverSfx", [&]() {
                    // AudioEngine::getInstance().playSound()
                }
            },
        };
        _data->rom->registerRenderObjects({
                {
                    .model = model,
                    .animator = new vkglTF::Animator(model, callbacks),
                    .renderLayer = RenderLayer::VISIBLE,
                    .attachedEntityGuid = getGUID(),
                }
            },
            { &_data->renderObj }
        );

        // Create collision box.
        // @NOCHECKIN.

        // Choose uncover time.
        std::default_random_engine generator;
		std::uniform_real_distribution<float_t> distribution(myItemType(_data).uncoverTimeMinMax[0], myItemType(_data).uncoverTimeMinMax[1]);
        _data->chosenUncoverTime = distribution(generator);

        _data->requestChangeItemModel = false;
    }

    // Process rustle timer.
    _data->rustleTimer -= deltaTime;
    if (_data->rustleTimer <= 0.0f)
    {
        std::default_random_engine generator;
		std::uniform_real_distribution<float_t> distribution(1.0f, 8.0f);
        _data->rustleTimer = distribution(generator);
        _data->renderObj->animator->setTrigger("goto_rustle");
    }
}

void CoveredItem::lateUpdate(const float_t& deltaTime)
{
    {
        auto& roTransform = _data->renderObj->transformMatrix;
        glm_mat4_identity(roTransform);
        glm_translate(roTransform, _data->position);
        mat4 rotation;
        glm_euler_zyx(vec3{ 0.0f, _data->yRotation, 0.0f }, rotation);
        glm_mul_rot(roTransform, rotation, roTransform);
    }

    {
        auto& icroTransform = _data->interactionCursorRenderObj->transformMatrix;
        glm_mat4_identity(icroTransform);
        vec3 position;
        glm_vec3_add(_data->position, myItemType(_data).interactionOrigin, position);
        position[1] += myItemType(_data).interactionIconYoff;
        glm_translate(icroTransform, position);
        // mat4 view;
        // // glm_lookat(_data->camera->sceneCamera.gpuCameraData.cameraPosition, position, vec3{ 0.0f, 1.0f, 0.0f }, view);
        // glm_lookat(position, _data->camera->sceneCamera.gpuCameraData.cameraPosition, vec3{ 0.0f, 1.0f, 0.0f }, view);
        // glm_mul_rot(icroTransform, view, icroTransform);

        mat4 rotation;  // @COPYPASTA from Character.cpp
        {
            vec3 eulerAngles = { glm_rad(30.0f), 0.0f, 0.0f };
            glm_euler_zyx(eulerAngles, rotation);
        }
        mat4 rotation2;
        {
            vec3 eulerAngles = { 0.0f, std::atan2f(_data->camera->sceneCamera.facingDirection[0], _data->camera->sceneCamera.facingDirection[2]) + glm_rad(90.0f), 0.0f };
            glm_euler_zyx(eulerAngles, rotation2);
        }
        glm_mul_rot(icroTransform, rotation, icroTransform);
        glm_mul_rot(icroTransform, rotation2, icroTransform);
    }

    glm_mat4_copy(_data->interactionCursorRenderObj->transformMatrix, _data->uncoverProgressRenderObj->transformMatrix);
}

void CoveredItem::dump(DataSerializer& ds)
{
    Entity::dump(ds);
    ds.dumpVec3(_data->position);
    ds.dumpFloat(_data->yRotation);
    float_t itemTypeIdF = (float_t)_data->itemTypeId;
    ds.dumpFloat(itemTypeIdF);
}

void CoveredItem::load(DataSerialized& ds)
{
    Entity::load(ds);
    ds.loadVec3(_data->position);
    ds.loadFloat(_data->yRotation);
    float_t itemTypeIdF;
    ds.loadFloat(itemTypeIdF);
    _data->itemTypeId = (size_t)itemTypeIdF;
}

bool CoveredItem::processMessage(DataSerialized& message)
{
    std::string messageType;
    message.loadString(messageType);

    if (messageType == "msg_commit_interaction")
    {
        _data->uncoverTimer = 0.0f;  // Reset timer.
        _data->uncoverProgressRenderObj->animator->setUpdateSpeedMultiplier(1.0f / _data->chosenUncoverTime);
        _data->uncoverProgressRenderObj->animator->setTrigger("goto_fillup");
        _data->uncoverProgressRenderObj->renderLayer = RenderLayer::VISIBLE;
        _data->isUncovering = true;
        return true;
    }
    else if (messageType == "msg_front_of_interaction_queue")
    {
        _data->showInteractionCursor = true;
        return true;
    }

    return false;
}

void CoveredItem::reportMoved(mat4* matrixMoved)
{
    vec4 pos;
    mat4 rot;
    vec3 sca;
    glm_decompose(*matrixMoved, pos, rot, sca);
    glm_vec3_copy(pos, _data->position);
    vec3 rotEuler;
    glm_euler_angles(rot, rotEuler);
    _data->yRotation = rotEuler[1];
}

void CoveredItem::renderImGui()
{
    int32_t iti = _data->itemTypeId;
    if (ImGui::InputInt("CoveredItemId", &iti))
    {
        _data->itemTypeId = (size_t)glm_clamp(iti, 0, CoveredItem_XData::numItemTypes - 1);
        _data->requestChangeItemModel = true;
    }

    ImGui::Text("Edit covered item ID props");
    ImGui::DragFloat3("collisionBoxExtent", myItemType(_data).collisionBoxExtent);
    ImGui::DragFloat3("interactionOrigin", myItemType(_data).interactionOrigin);
    ImGui::DragFloat("interactionRadius", &myItemType(_data).interactionRadius);
    ImGui::DragFloat("interactionIconYoff", &myItemType(_data).interactionIconYoff);
    ImGui::DragFloat2("uncoverTimeMinMax", myItemType(_data).uncoverTimeMinMax);
}

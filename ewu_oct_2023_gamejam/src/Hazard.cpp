#include "Hazard.h"

#include <iostream>
#include "GlobalState.h"
#include "RenderObject.h"
#include "EntityManager.h"
#include "DataSerialization.h"
#include "RandomNumberGenerator.h"
#include "PhysicsEngine.h"
#include "VkglTFModel.h"
#include "imgui/imgui.h"

#include "InputManager.h"


struct Hazard_XData
{
    RenderObjectManager* rom;

    RenderObject* renderObj;
    JPH::BodyID dangerTriggerBodyId;

    vec3 position;
    int8_t hazardType = -1;

    int32_t hazardTimer = -1;
    bool isHazardAttacking = false;

    float_t hazardTriggerXOffset;
};

struct HazardHitscanData
{
    std::vector<int32_t> triggerActiveTimes;
};
HazardHitscanData hazardDatas[] = {
    {
        .triggerActiveTimes = { 10, 11, 12, }
    },
};

void loadRenderObjForHazard(Hazard_XData* d, Hazard* _this, const std::string& myGuid)
{
    if (d->hazardType < 0)
        return;

    std::string modelName;
    switch (d->hazardType)
    {
        case 0:
            modelName = "HazardType1";
            break;
    }
    vkglTF::Model* model = d->rom->getModel(modelName, _this, [](){});
    std::vector<vkglTF::Animator::AnimatorCallback> callbacks = {
        {
            "SetOffSfx", [&]() {
                // AudioEngine::getInstance().playSound()
            }
        },
    };
    d->rom->registerRenderObjects({
            {
                .model = model,
                .animator = new vkglTF::Animator(model, callbacks),
                .renderLayer = RenderLayer::VISIBLE,
                .attachedEntityGuid = myGuid,
            }
        },
        { &d->renderObj }
    );
    glm_translate(d->renderObj->transformMatrix, d->position);

    versor quatIdentity = GLM_QUAT_IDENTITY_INIT;
    d->dangerTriggerBodyId = physengine::createBoxColliderBody(myGuid, d->position, quatIdentity, vec3{ 0.05f, 5.0f, 10.0f }, true);
}

Hazard::Hazard(EntityManager* em, RenderObjectManager* rom, DataSerialized* ds) : Entity(em, ds), _data(new Hazard_XData())
{
    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;

    _data->rom = rom;

    if (ds)
        load(*ds);

    constexpr float_t MIDPOINT = 2.0f;  // 2.0 is guarateed to fall on character.
    constexpr float_t SPREAD = 3.0f;
    _data->hazardTriggerXOffset = rng::randomRealRange(MIDPOINT - SPREAD, MIDPOINT + SPREAD);

    loadRenderObjForHazard(_data, this, getGUID());

    globalState::phase1.registerHazard(this);
}

Hazard::~Hazard()
{
    _data->rom->unregisterRenderObjects({ _data->renderObj });
    physengine::destroyBody(_data->dangerTriggerBodyId);

    delete _data;
}

void Hazard::physicsUpdate(const float_t& physicsDeltaTime)
{
    if (_data->hazardTimer < 0)
    {
        // Visualize
        constexpr float_t lineExtent = 5.0f;
        vec3 p1, p2;
        glm_vec3_add(_data->position, vec3{ _data->hazardTriggerXOffset, 0.0f, -lineExtent }, p1);
        glm_vec3_add(_data->position, vec3{ _data->hazardTriggerXOffset, 0.0f,  lineExtent }, p2);
        physengine::drawDebugVisLine(p1, p2, physengine::DebugVisLineType::VELOCITY);

        // Trigger hazard.
        if ((*globalState::playerPositionRef)[0] < _data->position[0] + _data->hazardTriggerXOffset)
        {
            _data->renderObj->animator->setTrigger("goto_set_off");
            _data->hazardTimer = 0;
        }
    }

    // Update whether hazard is actively attacking this frame.
    if (_data->hazardTimer >= 0)
    {
        _data->hazardTimer++;

        _data->isHazardAttacking = false;
        for (auto& t : hazardDatas[(size_t)_data->hazardType].triggerActiveTimes)
            if (t == _data->hazardTimer)
            {
                _data->isHazardAttacking = true;
                break;
            }
    }
}

void Hazard::update(const float_t& deltaTime)
{
}

void Hazard::lateUpdate(const float_t& deltaTime)
{
}

void Hazard::dump(DataSerializer& ds)
{
    Entity::dump(ds);
}

void Hazard::load(DataSerialized& ds)
{
    Entity::load(ds);
    ds.loadVec3(_data->position);
    float_t hazardTypeF;
    ds.loadFloat(hazardTypeF);
    _data->hazardType = (int8_t)hazardTypeF;
}

bool Hazard::processMessage(DataSerialized& message)
{
    return false;
}

void Hazard::reportMoved(mat4* matrixMoved)
{
}

void Hazard::renderImGui()
{
    ImGui::DragFloat("hazardTriggerXOffset", &_data->hazardTriggerXOffset);
}

void Hazard::reportTriggerActivation(const std::string& entityGUID)
{
    if (!_data->isHazardAttacking)
        return;

    // Notify successful attack to entity.
    DataSerializer ds;
    ds.dumpString("msg_stunned_by_hazard");
    DataSerialized dsd = ds.getSerializedData();
    _em->sendMessage(entityGUID, dsd);
}

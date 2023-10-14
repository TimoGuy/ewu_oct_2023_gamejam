#include "CameraRail.h"

#include <iostream>
#include "VkglTFModel.h"
#include "RenderObject.h"
#include "EntityManager.h"
#include "PhysicsEngine.h"
#include "DataSerialization.h"
#include "GlobalState.h"
#include "imgui/imgui.h"


struct CameraRail_XData
{
    RenderObjectManager* rom;
    RenderObject* renderObj;
    vec3 position = GLM_VEC3_ZERO_INIT;
    vec3 forward = { 0.0f, 0.0f, 1.0f };
    vec3 right = { 1.0f, 0.0f, 0.0f };
    float_t scrollAlongForwardAmount = 100.0f;
    float_t scrollAlongRightAmount = 0.0f;

    bool isPicked = false;
};

bool alwaysShowRailLine = false;


CameraRail::CameraRail(EntityManager* em, RenderObjectManager* rom, DataSerialized* ds) : Entity(em, ds), _data(new CameraRail_XData())
{
    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;

    _data->rom = rom;

    if (ds)
        load(*ds);

    _data->rom->registerRenderObjects({
            {
                .model = _data->rom->getModel("DevCameraRail", this, [](){}),
                .renderLayer = RenderLayer::BUILDER,
                .attachedEntityGuid = getGUID(),
            }
        },
        { &_data->renderObj }
    );
    glm_translate(_data->renderObj->transformMatrix, _data->position);
    versor rot;
    glm_quat_from_vecs(vec3{ 0.0f, 0.0f, 1.0f }, _data->forward, rot);
    glm_quat_rotate(_data->renderObj->transformMatrix, rot, _data->renderObj->transformMatrix);

    globalState::addCameraRail(this);
}

CameraRail::~CameraRail()
{
    globalState::removeCameraRail(this);

    _data->rom->unregisterRenderObjects({ _data->renderObj });
    _data->rom->removeModelCallbacks(this);

    delete _data;
}

void CameraRail::physicsUpdate(const float_t& physicsDeltaTime)
{
    if (alwaysShowRailLine || _data->isPicked)
    {
        if (_data->scrollAlongForwardAmount > 0.0f)
        {
            vec3 pt1, pt2;
            glm_vec3_copy(_data->position, pt1);
            glm_vec3_copy(_data->position, pt2);
            glm_vec3_muladds(_data->forward, -_data->scrollAlongForwardAmount, pt1);
            glm_vec3_muladds(_data->forward, _data->scrollAlongForwardAmount, pt2);

            if (_data->scrollAlongRightAmount == 0.0f)
                physengine::drawDebugVisLine(pt1, pt2, physengine::DebugVisLineType::YUUJUUFUDAN);
            else
            {
                // Render all lines.
                vec3 wpt1 = GLM_VEC3_ZERO_INIT;
                vec3 wpt2 = GLM_VEC3_ZERO_INIT;
                glm_vec3_muladds(_data->right, -_data->scrollAlongRightAmount, wpt1);
                glm_vec3_muladds(_data->right, _data->scrollAlongRightAmount, wpt2);

                vec3 lpt1, rpt1, lpt2, rpt2;
                glm_vec3_add(pt1, wpt1, lpt1);
                glm_vec3_add(pt1, wpt2, rpt1);
                glm_vec3_add(pt2, wpt1, lpt2);
                glm_vec3_add(pt2, wpt2, rpt2);

                physengine::drawDebugVisLine(lpt1, lpt2, physengine::DebugVisLineType::YUUJUUFUDAN);
                physengine::drawDebugVisLine(rpt1, rpt2, physengine::DebugVisLineType::YUUJUUFUDAN);
                physengine::drawDebugVisLine(lpt1, rpt1, physengine::DebugVisLineType::SUCCESS);
                physengine::drawDebugVisLine(lpt2, rpt2, physengine::DebugVisLineType::SUCCESS);
            }
        }
        else if (_data->scrollAlongRightAmount > 0.0f)
        {
            // Render just width line.
            vec3 pt1, pt2;
            glm_vec3_copy(_data->position, pt1);
            glm_vec3_copy(_data->position, pt2);
            glm_vec3_muladds(_data->right, -_data->scrollAlongRightAmount, pt1);
            glm_vec3_muladds(_data->right, _data->scrollAlongRightAmount, pt2);
            physengine::drawDebugVisLine(pt1, pt2, physengine::DebugVisLineType::SUCCESS);
        }

        _data->isPicked = false;
    }
}

void CameraRail::update(const float_t& deltaTime)
{
}

void CameraRail::lateUpdate(const float_t& deltaTime)
{
    glm_mat4_identity(_data->renderObj->transformMatrix);
    glm_translate(_data->renderObj->transformMatrix, _data->position);
    versor rot;
    glm_quat_from_vecs(vec3{ 0.0f, 0.0f, 1.0f }, _data->forward, rot);
    glm_quat_rotate(_data->renderObj->transformMatrix, rot, _data->renderObj->transformMatrix);
}

void CameraRail::dump(DataSerializer& ds)
{
    Entity::dump(ds);
    ds.dumpVec3(_data->position);
    ds.dumpVec3(_data->forward);
    ds.dumpVec3(_data->right);
    ds.dumpFloat(_data->scrollAlongForwardAmount);
    ds.dumpFloat(_data->scrollAlongRightAmount);
}

void CameraRail::load(DataSerialized& ds)
{
    Entity::load(ds);
    ds.loadVec3(_data->position);
    ds.loadVec3(_data->forward);
    if (ds.getSerializedValuesCount() >= 1)
        ds.loadVec3(_data->right);
    else
        glm_vec3_crossn(_data->forward, vec3{ 0.0f, 1.0f, 0.0f }, _data->right);
    if (ds.getSerializedValuesCount() >= 1)
        ds.loadFloat(_data->scrollAlongForwardAmount);
    if (ds.getSerializedValuesCount() >= 1)
        ds.loadFloat(_data->scrollAlongRightAmount);
}

bool CameraRail::processMessage(DataSerialized& message)
{
    std::string messageType;
    message.loadString(messageType);

    // @TODO: process messages here.

    return false;
}

void CameraRail::reportMoved(mat4* matrixMoved)
{
    vec4 pos;
    mat4 rot;
    vec3 sca;
    glm_decompose(*matrixMoved, pos, rot, sca);
    glm_vec3_copy(pos, _data->position);
    glm_mat4_mulv3(rot, vec3{ 0.0f, 0.0f, 1.0f }, 0.0f, _data->forward);
    glm_mat4_mulv3(rot, vec3{ 1.0f, 0.0f, 0.0f }, 0.0f, _data->right);
}

void CameraRail::renderImGui()
{
    _data->isPicked = true;
    ImGui::Checkbox("Toggle showing all camera rails", &alwaysShowRailLine);
    ImGui::DragFloat("Scroll Along Forward Amount", &_data->scrollAlongForwardAmount, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Scroll Along Right Amount", &_data->scrollAlongRightAmount, 0.1f, 0.0f, 100.0f);
}

float_t CameraRail::findDistance2FromRailIfInlineEnough(vec3 queryPos, vec3 queryDir)
{
    if (std::abs(glm_vec3_dot(queryDir, _data->forward)) < std::cos(glm_rad(30.0f)))
        return std::numeric_limits<float_t>::max();
    
    vec3 delta;
    glm_vec3_sub(_data->position, queryPos, delta);
    vec3 forwardXDelta;
    glm_vec3_cross(_data->forward, delta, forwardXDelta);
    return glm_vec3_norm2(forwardXDelta);
}

void CameraRail::projectPositionOnRail(vec3 pos, vec3& outProjectedPosition)
{
    glm_vec3_copy(_data->position, outProjectedPosition);

    vec3 delta;
    glm_vec3_sub(pos, _data->position, delta);

    if (_data->scrollAlongForwardAmount > 0.0f)
    {
        vec3 projVec;
        glm_vec3_proj(delta, _data->forward, projVec);
        glm_vec3_scale_as(projVec, glm_min(glm_vec3_norm(projVec), _data->scrollAlongForwardAmount), projVec);
        glm_vec3_add(outProjectedPosition, projVec, outProjectedPosition);
    }

    if (_data->scrollAlongRightAmount > 0.0f)
    {
        vec3 rightProjVec;
        glm_vec3_proj(delta, _data->right, rightProjVec);
        glm_vec3_scale_as(rightProjVec, glm_min(glm_vec3_norm(rightProjVec), _data->scrollAlongRightAmount), rightProjVec);
        glm_vec3_add(outProjectedPosition, rightProjVec, outProjectedPosition);
    }
}

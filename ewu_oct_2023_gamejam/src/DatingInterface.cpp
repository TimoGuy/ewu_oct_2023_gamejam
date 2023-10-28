#include "DatingInterface.h"

#include <iostream>
#include "GlobalState.h"
#include "RenderObject.h"
#include "InputManager.h"
#include "Camera.h"
#include "imgui/imgui.h"


struct DatingInterface_XData
{
    RenderObjectManager* rom;
    Camera* camera;

    RenderObject* renderObj;

};

DatingInterface::DatingInterface(EntityManager* em, RenderObjectManager* rom, Camera* camera, DataSerialized* ds) : Entity(em, ds), _data(new DatingInterface_XData())
{
    _data->rom = rom;
    _data->camera = camera;

    if (ds)
        load(*ds);

    _data->rom->registerRenderObjects({
            {
                .model = _data->rom->getModel("DevDatingInterface", this, [](){}),
                .renderLayer = RenderLayer::BUILDER,
                .attachedEntityGuid = getGUID(),
            }
        },
        { &_data->renderObj }
    );
    glm_translate(_data->renderObj->transformMatrix, vec3{ 0.0f, 500.0f, 0.0f });

    globalState::phase2.registerDatingInterface(this);
}

DatingInterface::~DatingInterface()
{
    delete _data;
}

void DatingInterface::physicsUpdate(const float_t& physicsDeltaTime)
{
}

void DatingInterface::update(const float_t& deltaTime)
{
    if (input::keyDownPressed)
    {
        // @STUB:
        std::cout << "JASDKFAKSJDFKASJDKFJKASJDFKJASDKFJKASDKFJASDKFJKASDF================================================================================================" << std::endl;
        globalState::phase1.transitionToPhase1FromPhase2();
    }
}

void DatingInterface::lateUpdate(const float_t& deltaTime)
{
}

void DatingInterface::dump(DataSerializer& ds)
{
    Entity::dump(ds);
}

void DatingInterface::load(DataSerialized& ds)
{
    Entity::load(ds);
}

bool DatingInterface::processMessage(DataSerialized& message)
{
    return false;
}

void DatingInterface::reportMoved(mat4* matrixMoved)
{
}

void DatingInterface::renderImGui()
{
}

void DatingInterface::activate(size_t dateIdx)
{
    _data->camera->mainCamMode.setMainCamTargetObject(_data->renderObj);

    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;
}

void DatingInterface::deactivate()
{
    Entity::_enablePhysicsUpdate = false;
    Entity::_enableUpdate = false;
    Entity::_enableLateUpdate = false;
}

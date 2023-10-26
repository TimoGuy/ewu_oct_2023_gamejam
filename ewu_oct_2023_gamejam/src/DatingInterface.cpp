#include "DatingInterface.h"

#include <iostream>
#include "GlobalState.h"
#include "imgui/imgui.h"


struct DatingInterface_XData
{
    RenderObjectManager* rom;
    Camera* camera;
};

DatingInterface::DatingInterface(EntityManager* em, RenderObjectManager* rom, Camera* camera, DataSerialized* ds) : Entity(em, ds), _data(new DatingInterface_XData())
{
    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;

    _data->rom = rom;
    _data->camera = camera;

    if (ds)
        load(*ds);

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
}

void DatingInterface::deactivate()
{
}

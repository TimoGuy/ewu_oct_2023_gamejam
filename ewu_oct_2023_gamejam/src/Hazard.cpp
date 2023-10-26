#include "Hazard.h"

#include <iostream>
#include "GlobalState.h"
#include "imgui/imgui.h"


struct Hazard_XData
{
    RenderObjectManager* rom;
};

Hazard::Hazard(EntityManager* em, RenderObjectManager* rom, DataSerialized* ds) : Entity(em, ds), _data(new Hazard_XData())
{
    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;

    _data->rom = rom;

    if (ds)
        load(*ds);

    globalState::phase1.registerHazard(this);
}

Hazard::~Hazard()
{
    delete _data;
}

void Hazard::physicsUpdate(const float_t& physicsDeltaTime)
{
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
}

void Hazard::reset()
{
}

void Hazard::setOff()
{
}

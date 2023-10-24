#include "MainMenu.h"

#include <iostream>
#include "VkglTFModel.h"
#include "RenderObject.h"
#include "EntityManager.h"
#include "AudioEngine.h"
#include "DataSerialization.h"
#include "InputManager.h"
#include "GlobalState.h"
#include "Textbox.h"
#include "Debug.h"
#include "imgui/imgui.h"


struct MainMenu_XData
{
    RenderObjectManager* rom;
    RenderObject* renderObj;
};


MainMenu::MainMenu(EntityManager* em, RenderObjectManager* rom, DataSerialized* ds) : Entity(em, ds), _data(new MainMenu_XData())
{
    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;

    _data->rom = rom;

    if (ds)
        load(*ds);

    globalState::isGameActive = false;

    _data->rom->registerRenderObjects({
            {
                .model = _data->rom->getModel("DevMainMenu", this, [](){}),
                .renderLayer = RenderLayer::BUILDER,
                .attachedEntityGuid = getGUID(),
            }
        },
        { &_data->renderObj }
    );
}

MainMenu::~MainMenu()
{
    _data->rom->unregisterRenderObjects({ _data->renderObj });
    _data->rom->removeModelCallbacks(this);
    delete _data;
}

void MainMenu::physicsUpdate(const float_t& physicsDeltaTime)
{
    
}

void MainMenu::update(const float_t& deltaTime)
{
    if (input::onKeyJumpPress)
        scene::loadScene("hello_hello_world.ssdat");
}

void MainMenu::lateUpdate(const float_t& deltaTime)
{
    
}

void MainMenu::dump(DataSerializer& ds)
{
    Entity::dump(ds);
}

void MainMenu::load(DataSerialized& ds)
{
    Entity::load(ds);
}

bool MainMenu::processMessage(DataSerialized& message)
{
    return false;
}

void MainMenu::reportMoved(mat4* matrixMoved)
{
}

void MainMenu::renderImGui()
{
    ImGui::Button("Add Menu Item");
}

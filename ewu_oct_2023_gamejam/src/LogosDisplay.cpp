#include "LogosDisplay.h"

#include <iostream>
#include "VkglTFModel.h"
#include "VulkanEngine.h"
#include "RenderObject.h"
#include "EntityManager.h"
#include "AudioEngine.h"
#include "DataSerialization.h"
#include "RandomNumberGenerator.h"
#include "InputManager.h"
#include "SceneManagement.h"
#include "GlobalState.h"
#include "TextMesh.h"
#include "UIQuad.h"
#include "Camera.h"
#include "Debug.h"
#include "imgui/imgui.h"


struct LogosDisplay_XData
{
    RenderObjectManager* rom;
    Camera* camera;
    VulkanEngine* engine;
    RenderObject* renderObj;

    float_t displayTimer = 1.0f;
    float_t moveToMainMenuTimer = 1.5f;

    std::vector<std::string> uiLogoTextureNames = {
		"LogoTimoEngine",
		"LogoVulkan",
		"LogoFMOD",
		"LogoSDL",
		"LogoglTF",
		"LogoJolt",
		"SpecialThanks",
    };
    std::vector<ui::UIQuad*> uiLogos;
};


LogosDisplay::LogosDisplay(EntityManager* em, RenderObjectManager* rom, Camera* camera, VulkanEngine* engine, DataSerialized* ds) : Entity(em, ds), _data(new LogosDisplay_XData())
{
    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;

    _data->rom = rom;
    _data->camera = camera;
    _data->engine = engine;

    if (ds)
        load(*ds);

    _data->rom->registerRenderObjects({
            {
                .model = _data->rom->getModel("DevMainMenu", this, [](){}),
                .renderLayer = RenderLayer::BUILDER,
                .attachedEntityGuid = getGUID(),
            }
        },
        { &_data->renderObj }
    );

    // Create ui quads.
    for (size_t i = 0; i < _data->uiLogoTextureNames.size(); i++)
        _data->uiLogos.push_back(ui::registerUIQuad(&engine->_loadedTextures[_data->uiLogoTextureNames[i]]));
    
    // @HARDCODE: adjust ui quad sizes.
    glm_vec3_copy(vec3{ -14.0f, 183.0f, 0.0f }, _data->uiLogos[0]->position);
    glm_vec3_copy(vec3{ 226.0f, 113.0f, 1.0f }, _data->uiLogos[0]->scale);

    glm_vec3_copy(vec3{ 461.0f, 140.0f, 0.0f }, _data->uiLogos[1]->position);
    glm_vec3_copy(vec3{ 200.0f, 100.0f, 1.0f }, _data->uiLogos[1]->scale);

    glm_vec3_copy(vec3{ -484.0f, 128.0f, 0.0f }, _data->uiLogos[2]->position);
    glm_vec3_copy(vec3{ 130.0f, 65.0f, 1.0f }, _data->uiLogos[2]->scale);

    glm_vec3_copy(vec3{ 256.0f, -12.0f, 0.0f }, _data->uiLogos[3]->position);
    glm_vec3_copy(vec3{ 122.0f, 61.0f, 1.0f }, _data->uiLogos[3]->scale);

    glm_vec3_copy(vec3{ -284.0f, 2.0f, 0.0f }, _data->uiLogos[4]->position);
    glm_vec3_copy(vec3{ 150.0f, 75.0f, 1.0f }, _data->uiLogos[4]->scale);

    glm_vec3_copy(vec3{ -18.0f, 5.0f, 0.0f }, _data->uiLogos[5]->position);
    glm_vec3_copy(vec3{ 70.0f, 70.0f, 1.0f }, _data->uiLogos[5]->scale);

    glm_vec3_copy(vec3{ 0.0f, -390.0f, 0.0f }, _data->uiLogos[6]->position);
    glm_vec3_copy(vec3{ 650.0f, 65.0f, 1.0f }, _data->uiLogos[6]->scale);

    // @NOTE: this shouldn't be here, but it's a pre-warm music bc it needs to sync up with the ui elements.
    AudioEngine::getInstance().loadSound("res/music/title.ogg");
    AudioEngine::getInstance().loadSound("res/music/searchies.ogg");
    AudioEngine::getInstance().loadSound("res/music/chase_hall.ogg");
}

LogosDisplay::~LogosDisplay()
{
    for (auto& logo : _data->uiLogos)
        ui::unregisterUIQuad(logo);

    _data->rom->unregisterRenderObjects({ _data->renderObj });
    _data->rom->removeModelCallbacks(this);
    delete _data;
}

void LogosDisplay::physicsUpdate(const float_t& physicsDeltaTime)
{
    
}

void LogosDisplay::update(const float_t& deltaTime)
{
    if ((_data->displayTimer -= deltaTime) < 0.0f)
        for (auto& l : _data->uiLogos)
            l->visible = false;

    if ((_data->moveToMainMenuTimer -= deltaTime) < 0.0f)
        scene::loadScene("main_menu.ssdat", true);
}

void LogosDisplay::lateUpdate(const float_t& deltaTime)
{
    
}

void LogosDisplay::dump(DataSerializer& ds)
{
    Entity::dump(ds);
}

void LogosDisplay::load(DataSerialized& ds)
{
    Entity::load(ds);
}

bool LogosDisplay::processMessage(DataSerialized& message)
{
    return false;
}

void LogosDisplay::reportMoved(mat4* matrixMoved)
{
}

// @COPYPASTA from DatingInterface.cpp
void imguiUIQuad3(const std::string& uiQuadName, ui::UIQuad* uiQuad)
{
    std::string nameSuffix = "##69420uiquad" + uiQuadName;
    if (ImGui::TreeNode((uiQuadName + nameSuffix).c_str()))
    {
        ImGui::Checkbox(("visible" + nameSuffix).c_str(), &uiQuad->visible);
        ImGui::Checkbox(("useNineSlicing" + nameSuffix).c_str(), &uiQuad->useNineSlicing);
        if (uiQuad->useNineSlicing)
        {
            ImGui::DragFloat(("nineSlicingSizeX" + nameSuffix).c_str(), &uiQuad->nineSlicingSizeX);
            ImGui::DragFloat(("nineSlicingSizeY" + nameSuffix).c_str(), &uiQuad->nineSlicingSizeY);
        }
        ImGui::ColorPicker4(("tint" + nameSuffix).c_str(), uiQuad->tint);
        
        ImGui::Text("Transform");
        ImGui::DragFloat3(("Pos" + nameSuffix).c_str(), uiQuad->position);
        // @TODO: @INCOMPLETE: convert quaternion of the rotation to euler angles, so that they can be edited here!
        // vec3 eulerAngles;
        // glm_decompose
        // if (ImGui::DragFloat3(("Rot" + nameSuffix).c_str(), eulerAngles))
        // {
        //     mat4 newRot;
        //     glm_euler_zyx(eulerAngles, newRot);
        //     glm_mat4_quat(rotation, rotationV);
        // }
        ImGui::DragFloat3(("Sca" + nameSuffix).c_str(), uiQuad->scale);

        ImGui::DragFloat(("renderOrder" + nameSuffix).c_str(), &uiQuad->renderOrder);

        ImGui::TreePop();
        ImGui::Separator();
    }
}

void LogosDisplay::renderImGui()
{
    for (size_t i = 0; i < _data->uiLogos.size(); i++)
    {
        imguiUIQuad3(_data->uiLogoTextureNames[i], _data->uiLogos[i]);
    }
}

#include "GameOverMenu.h"

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
#include "Camera.h"
#include "Debug.h"
#include "UIQuad.h"
#include "imgui/imgui.h"


struct GameOverMenu_XData
{
    RenderObjectManager* rom;
    Camera* camera;
    RenderObject* renderObj;

    ui::UIQuad* loseArt;
    ui::UIQuad* dateArt[3];

    float_t enableReturnToMainMenuTimer = 3.0f;
    textmesh::TextMesh* returnToMainMenuPromptText;
};


GameOverMenu::GameOverMenu(EntityManager* em, RenderObjectManager* rom, Camera* camera, VulkanEngine* engine, DataSerialized* ds) : Entity(em, ds), _data(new GameOverMenu_XData())
{
    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;

    _data->rom = rom;
    _data->camera = camera;

    if (ds)
        load(*ds);

    globalState::isGameActive = false;
    globalState::renderSkybox = false;

    _data->rom->registerRenderObjects({
            {
                .model = _data->rom->getModel("DevMainMenu", this, [](){}),  // @NOTE: @INCOMPLETE: ehhh, this model can do for now.
                .renderLayer = RenderLayer::BUILDER,
                .attachedEntityGuid = getGUID(),
            }
        },
        { &_data->renderObj }
    );

    _data->loseArt = ui::registerUIQuad(&engine->_loadedTextures["LoseArt"]);
    glm_vec3_copy(vec3{ 500.0f, 500.0f, 1.0f }, _data->loseArt->scale);
    _data->loseArt->visible = false;

    _data->dateArt[0] = ui::registerUIQuad(&engine->_loadedTextures["DateArt0"]);
    glm_vec3_copy(vec3{ 500.0f, 500.0f, 1.0f }, _data->dateArt[0]->scale);
    _data->dateArt[0]->visible = false;

    _data->dateArt[1] = ui::registerUIQuad(&engine->_loadedTextures["DateArt1"]);
    glm_vec3_copy(vec3{ 500.0f, 500.0f, 1.0f }, _data->dateArt[1]->scale);
    _data->dateArt[1]->visible = false;

    _data->dateArt[2] = ui::registerUIQuad(&engine->_loadedTextures["DateArt2"]);
    glm_vec3_copy(vec3{ 500.0f, 500.0f, 1.0f }, _data->dateArt[2]->scale);
    _data->dateArt[2]->visible = false;

    _data->returnToMainMenuPromptText = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::CENTER, textmesh::MID, -1.0f, "Press Spacebar to Return to Main Menu.");
    glm_vec3_copy(vec3{ 0.0f, -6.0f, 0.0f }, _data->returnToMainMenuPromptText->renderPosition);
    _data->returnToMainMenuPromptText->scale = 0.5f;
    _data->returnToMainMenuPromptText->excludeFromBulkRender = true;
}

GameOverMenu::~GameOverMenu()
{
    ui::unregisterUIQuad(_data->loseArt);
    for (size_t i = 0; i < 3; i++)
        ui::unregisterUIQuad(_data->dateArt[i]);

    _data->rom->unregisterRenderObjects({ _data->renderObj });
    _data->rom->removeModelCallbacks(this);
    delete _data;
}

void GameOverMenu::physicsUpdate(const float_t& physicsDeltaTime)
{
    
}

void GameOverMenu::update(const float_t& deltaTime)
{
    bool canReturn = (_data->enableReturnToMainMenuTimer < 0.0f);
    if (!canReturn)
        _data->enableReturnToMainMenuTimer -= deltaTime;
    if (canReturn && input::onKeyJumpPress)
    {
        scene::loadScene("first.ssdat", true);
    }

    // @DEBUG: prevent art from updating.
    // return;

    _data->loseArt->visible = !globalState::gameFinishState.isWin;
    for (size_t i = 0; i < 3; i++)
        _data->dateArt[i]->visible = (globalState::gameFinishState.isWin && globalState::gameFinishState.dateIdx == i);
}

void GameOverMenu::lateUpdate(const float_t& deltaTime)
{
    
}

void GameOverMenu::dump(DataSerializer& ds)
{
    Entity::dump(ds);
}

void GameOverMenu::load(DataSerialized& ds)
{
    Entity::load(ds);
}

bool GameOverMenu::processMessage(DataSerialized& message)
{
    return false;
}

void GameOverMenu::reportMoved(mat4* matrixMoved)
{
}

// @COPYPASTA from DatingInterface.cpp
void imguiUIQuad2(const std::string& uiQuadName, ui::UIQuad* uiQuad)
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

// @COPYPASTA from DatingInterface.cpp
void imguiTextMesh3(const std::string& textMeshName, textmesh::TextMesh* textMesh)  // That's why this is renamed to 3 bc of a symbol collision.
{
    std::string nameSuffix = "##69420textmesh" + textMeshName;
    if (ImGui::TreeNode((textMeshName + nameSuffix).c_str()))
    {
        ImGui::Checkbox(("excludeFromBulkRender" + nameSuffix).c_str(), &textMesh->excludeFromBulkRender);
        ImGui::DragFloat3(("renderPosition" + nameSuffix).c_str(), textMesh->renderPosition);
        ImGui::DragFloat(("scale" + nameSuffix).c_str(), &textMesh->scale);
        if (ImGui::DragFloat(("maxLineLength" + nameSuffix).c_str(), &textMesh->maxLineLength))
        {
            textmesh::regenerateTextMeshMesh(textMesh, "This is a test text mesh word so that you can see just how well the words wrap.");
        }

        ImGui::TreePop();
        ImGui::Separator();
    }
}

void GameOverMenu::renderImGui()
{
    imguiUIQuad2("loseArt", _data->loseArt);
    imguiUIQuad2("dateArt[0]", _data->dateArt[0]);
    imguiUIQuad2("dateArt[1]", _data->dateArt[1]);
    imguiUIQuad2("dateArt[2]", _data->dateArt[2]);
    imguiTextMesh3("returnToMainMenuPromptText", _data->returnToMainMenuPromptText);
}

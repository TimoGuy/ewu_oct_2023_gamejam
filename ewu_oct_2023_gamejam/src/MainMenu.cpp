#include "MainMenu.h"

#include <iostream>
#include "VkglTFModel.h"
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
#include "imgui/imgui.h"


struct MainMenu_XData
{
    RenderObjectManager* rom;
    Camera* camera;
    RenderObject* renderObj;

    std::vector<RenderObject*> tarotCardROs;

    mat4 launchTransform;
    float_t cardLaunchInterval = 0.05f;
    float_t cardLaunchIntervalTimer = 0.0f;
    std::vector<size_t> tarotCardDrawIndices;
    size_t currentDrawIdx = 0;
    size_t currentCard = 0;
    bool launchingCards = false;

    struct Bio
    {
        textmesh::TextMesh* name;
        textmesh::TextMesh* description;
        float_t enabledAmount = -1.0f;  // < 0.0 is invisible. | >0.0 is visible.
    };
    Bio bio1, bio2, bio3;

    float_t allowStartGameTimer = 2.0f;
    textmesh::TextMesh* startGamePromptText;

    // Sfx.
    int32_t titleMusicId;
    float_t titleMusicVolume = 1.0f;
};


MainMenu::MainMenu(EntityManager* em, RenderObjectManager* rom, Camera* camera, DataSerialized* ds) : Entity(em, ds), _data(new MainMenu_XData())
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
                .model = _data->rom->getModel("DevMainMenu", this, [](){}),
                .renderLayer = RenderLayer::BUILDER,
                .attachedEntityGuid = getGUID(),
            }
        },
        { &_data->renderObj }
    );

    // Choose which cards to draw.
    constexpr size_t numTarotCards = 78;
    for (size_t i = 0; i < 3; i++)
    {
        bool collided;
        size_t idx;
        do
        {
            collided = false;
            idx = rng::randomIntegerRange(0, numTarotCards - 1);
            for (size_t di : _data->tarotCardDrawIndices)
                if (di == idx)
                {
                    collided = true;
                    break;
                }
        } while (collided);
        _data->tarotCardDrawIndices.push_back(idx);
    }
    std::sort(_data->tarotCardDrawIndices.begin(), _data->tarotCardDrawIndices.end());
    
    // Create render objects.
    std::vector<RenderObject> inROs;
    std::vector<RenderObject**> outROs;
    _data->tarotCardROs.resize(numTarotCards, nullptr);
    for (size_t i = 0; i < numTarotCards; i++)
    {
        vkglTF::Model* model = _data->rom->getModel("tarot_card_empty", this, [](){});
        std::vector<vkglTF::Animator::AnimatorCallback> callbacks = {
            {
                "EventHideMe", [&, i]() {
                    _data->tarotCardROs[i]->renderLayer = RenderLayer::INVISIBLE;
                },
            },
            {
                "EventShowBio1", [&]() {
                    _data->bio1.enabledAmount = 0.0f;
                },
            },
            {
                "EventShowBio2", [&]() {
                    _data->bio2.enabledAmount = 0.0f;
                },
            },
            {
                "EventShowBio3", [&]() {
                    _data->bio3.enabledAmount = 0.0f;
                },
            },
        };
        inROs.push_back({
            .model = model,
            .animator = new vkglTF::Animator(model, callbacks),
            .renderLayer = RenderLayer::VISIBLE,
            .attachedEntityGuid = getGUID(),
        });
        outROs.push_back(&_data->tarotCardROs[i]);
    }
    _data->rom->registerRenderObjects(inROs, outROs);

    constexpr float_t strideY = 0.02f;
    float_t currentY = 0.0f;
    for (int64_t i = _data->tarotCardROs.size() - 1; i >= 0; i--)
    {
        // Set transform.
        auto& transform = _data->tarotCardROs[i]->transformMatrix;
        glm_mat4_identity(transform);
        glm_translate(transform, vec3{ 0.0f, currentY, 0.0f });
        currentY += strideY;
    }

    // Create launch transform.
    glm_euler_zyx(vec3{ 0.0f, glm_rad(180.0f), 0.0f }, _data->launchTransform);

    _data->camera->mainCamMode.setMainCamTargetObject(_data->renderObj);

    // Create bios.
    _data->bio1.name = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, -1.0f, "Nefertiti");
    glm_vec3_copy(vec3{ 10.0f, 1.0f, 0.0f }, _data->bio1.name->renderPosition);
    _data->bio1.name->scale = 1.0f;
    _data->bio1.name->excludeFromBulkRender = true;

    _data->bio1.description = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, 13.0f, "Mummy from blah blah blah blah blah blah.");
    glm_vec3_copy(vec3{ 9.75f, -0.5f, 0.0f }, _data->bio1.description->renderPosition);
    _data->bio1.description->scale = 0.5f;
    _data->bio1.description->excludeFromBulkRender = true;

    _data->bio2.name = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, -1.0f, "Ophelia");
    glm_vec3_copy(vec3{ 3.0f, 1.0f, 0.0f }, _data->bio2.name->renderPosition);
    _data->bio2.name->scale = 1.0f;
    _data->bio2.name->excludeFromBulkRender = true;

    _data->bio2.description = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, 13.0f, "Ghost from blah blah blah blah blah blah.");
    glm_vec3_copy(vec3{ 2.6f, -0.5f, 0.0f }, _data->bio2.description->renderPosition);
    _data->bio2.description->scale = 0.5f;
    _data->bio2.description->excludeFromBulkRender = true;

    _data->bio3.name = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, -1.0f, "Isabella");
    glm_vec3_copy(vec3{ -4.0f, 1.0f, 0.0f }, _data->bio3.name->renderPosition);
    _data->bio3.name->scale = 1.0f;
    _data->bio3.name->excludeFromBulkRender = true;

    _data->bio3.description = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, 13.0f, "Vampire from blah blah blah blah blah blah.");
    glm_vec3_copy(vec3{ -4.5f, -0.5f, 0.0f }, _data->bio3.description->renderPosition);
    _data->bio3.description->scale = 0.5f;
    _data->bio3.description->excludeFromBulkRender = true;

    _data->startGamePromptText = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::CENTER, textmesh::MID, -1.0f, "Press Spacebar to Start.");
    glm_vec3_copy(vec3{ 0.0f, -4.5f, 0.0f }, _data->startGamePromptText->renderPosition);
    _data->startGamePromptText->scale = 0.5f;
    _data->startGamePromptText->excludeFromBulkRender = true;

    // Play looped track.
    _data->titleMusicId = AudioEngine::getInstance().playSound("res/music/title.ogg", true);
    _data->titleMusicVolume = 1.0f;
}

MainMenu::~MainMenu()
{
    textmesh::destroyAndUnregisterTextMesh(_data->bio1.name);
    textmesh::destroyAndUnregisterTextMesh(_data->bio1.description);
    textmesh::destroyAndUnregisterTextMesh(_data->bio2.name);
    textmesh::destroyAndUnregisterTextMesh(_data->bio2.description);
    textmesh::destroyAndUnregisterTextMesh(_data->bio3.name);
    textmesh::destroyAndUnregisterTextMesh(_data->bio3.description);
    textmesh::destroyAndUnregisterTextMesh(_data->startGamePromptText);

    _data->rom->unregisterRenderObjects(_data->tarotCardROs);
    _data->rom->unregisterRenderObjects({ _data->renderObj });
    _data->rom->removeModelCallbacks(this);
    delete _data;
}

void MainMenu::physicsUpdate(const float_t& physicsDeltaTime)
{
    
}

void MainMenu::update(const float_t& deltaTime)
{
    if (input::onKeyJumpPress && !_data->launchingCards)
    {
        _data->cardLaunchIntervalTimer = 0.0f;
        _data->currentDrawIdx = 0;
        _data->currentCard = 0;
        _data->launchingCards = true;

        AudioEngine::getInstance().playSound("res/sfx/wip_start_game.wav", false);
    }

    if (_data->launchingCards)
    {
        // Fade out title music.
        if (_data->titleMusicVolume > 0.0f)
        {
            _data->titleMusicVolume -= deltaTime * 2.0f;  // @HARDCODE.
            AudioEngine::getInstance().setChannelVolume(_data->titleMusicId, AudioEngine::getInstance().volumeToDb(_data->titleMusicVolume));
            if (_data->titleMusicVolume <= 0.0f)
                AudioEngine::getInstance().stopChannel(_data->titleMusicId);
        }

        _data->cardLaunchIntervalTimer += deltaTime;
        while (_data->currentCard < _data->tarotCardROs.size() &&
            _data->cardLaunchIntervalTimer > _data->cardLaunchInterval)
        {
            auto& tarotCard = _data->tarotCardROs[_data->currentCard];
            glm_mat4_copy(_data->launchTransform, tarotCard->transformMatrix);

            // Choose whether card is drawn or discarded.
            if (_data->currentDrawIdx < _data->tarotCardDrawIndices.size() &&
                _data->tarotCardDrawIndices[_data->currentDrawIdx] == _data->currentCard)
            {
                tarotCard->animator->setTrigger("draw_to_" + std::to_string(_data->currentDrawIdx + 1));
                _data->currentDrawIdx++;
            }
            else
                tarotCard->animator->setTrigger("goto_fly_around");
            _data->cardLaunchIntervalTimer -= _data->cardLaunchInterval;
            _data->currentCard++;
        }

        if (_data->currentCard >= _data->tarotCardROs.size())
        {
            _data->allowStartGameTimer -= deltaTime;
            bool allowStart = (_data->allowStartGameTimer < 0.0f);
            _data->startGamePromptText->excludeFromBulkRender = !allowStart;
            if (allowStart && input::onKeyJumpPress)
            {
                // Start new game.
                globalState::resetGameState();
                scene::loadScene("hello_hello_world.ssdat", true);
            }
        }
    }

    _data->bio1.name->excludeFromBulkRender =
        _data->bio1.description->excludeFromBulkRender =
        (_data->bio1.enabledAmount < 0.0f);

    _data->bio2.name->excludeFromBulkRender =
        _data->bio2.description->excludeFromBulkRender =
        (_data->bio2.enabledAmount < 0.0f);

    _data->bio3.name->excludeFromBulkRender =
        _data->bio3.description->excludeFromBulkRender =
        (_data->bio3.enabledAmount < 0.0f);
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

// @COPYPASTA from DatingInterface.cpp
void imguiTextMesh2(const std::string& textMeshName, textmesh::TextMesh* textMesh)  // That's why this is renamed to 2 bc of a symbol collision.
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

void MainMenu::renderImGui()
{
    imguiTextMesh2("bio1.name", _data->bio1.name);
    imguiTextMesh2("bio1.description", _data->bio1.description);
    imguiTextMesh2("bio2.name", _data->bio2.name);
    imguiTextMesh2("bio2.description", _data->bio2.description);
    imguiTextMesh2("bio3.name", _data->bio3.name);
    imguiTextMesh2("bio3.description", _data->bio3.description);
    imguiTextMesh2("startGamePromptText", _data->startGamePromptText);
}

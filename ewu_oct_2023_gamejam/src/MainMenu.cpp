#include "MainMenu.h"

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


struct MainMenu_XData
{
    RenderObjectManager* rom;
    Camera* camera;
    VulkanEngine* engine;
    RenderObject* renderObj;

    std::vector<RenderObject*> tarotCardROs;

    mat4 launchTransform;
    float_t cardLaunchInterval = 0.05f;
    float_t cardLaunchIntervalTimer = 0.0f;
    size_t currentCard = 0;
    std::vector<size_t> shufflingPerm;
    bool launchingCards = false;

    ui::UIQuad* uiTitleLogo = nullptr;
    ui::UIQuad* uiCovering = nullptr;
    float_t titleLogoAlpha = 0.0f;

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


MainMenu::MainMenu(EntityManager* em, RenderObjectManager* rom, Camera* camera, VulkanEngine* engine, DataSerialized* ds) : Entity(em, ds), _data(new MainMenu_XData())
{
    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;

    _data->rom = rom;
    _data->camera = camera;
    _data->engine = engine;

    _data->engine->setLightDirection(vec4{ -0.009f, 0.505f, 0.863f, 0.0f });

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
    
    // Create render objects.
    constexpr size_t numTarotCards = 78;
    std::vector<RenderObject> inROs;
    std::vector<RenderObject**> outROs;
    _data->tarotCardROs.resize(numTarotCards, nullptr);
    for (size_t i = 0; i < numTarotCards; i++)
    {
        vkglTF::Model* model = nullptr;
        if (i == 0)
            model = _data->rom->getModel("tarot_card_mummy", this, [](){});
        else if (i == 1)
            model = _data->rom->getModel("tarot_card_ghost", this, [](){});
        else if (i == 2)
            model = _data->rom->getModel("tarot_card_vampire", this, [](){});
        else
            model = _data->rom->getModel("tarot_card_empty", this, [](){});
        std::vector<vkglTF::Animator::AnimatorCallback> callbacks = {
            {
                "EventHideMe", [&, i]() {
                    _data->tarotCardROs[i]->renderLayer = RenderLayer::INVISIBLE;
                },
            },
            {
                "EventCardFlipSfx", [&, i]() {
                    AudioEngine::getInstance().playSoundFromList({
                        "res/sfx/card_flip_1.wav",
                        "res/sfx/card_flip_2.wav",
                    });
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

    // Choose which cards to draw.
    for (size_t i = 0; i < numTarotCards; i++)
        _data->shufflingPerm.push_back(i);
    rng::shuffleVectorSizeType(_data->shufflingPerm);

    constexpr float_t strideY = 0.02f;
    float_t currentY = 0.0f;
    for (int64_t i = _data->tarotCardROs.size() - 1; i >= 0; i--)
    {
        // Set transform.
        auto& transform = _data->tarotCardROs[_data->shufflingPerm[i]]->transformMatrix;
        glm_mat4_identity(transform);
        glm_translate(transform, vec3{ 0.0f, currentY, 0.0f });
        currentY += strideY;
    }

    // Create launch transform.
    glm_euler_zyx(vec3{ 0.0f, glm_rad(180.0f), 0.0f }, _data->launchTransform);

    _data->camera->mainCamMode.setMainCamTargetObject(_data->renderObj);

    // Create ui quads.
    _data->uiTitleLogo = ui::registerUIQuad(&engine->_loadedTextures["TitleLogo"]);
    _data->uiTitleLogo->renderOrder = -1.0f;
    glm_vec3_copy(vec3{ 83.0f, 41.0f, 0.0f }, _data->uiTitleLogo->position);
    glm_vec3_copy(vec3{ 500.0f, 500.0f, 1.0f }, _data->uiTitleLogo->scale);
    glm_vec4_copy(vec4{ 1.0f, 1.0f, 1.0f, 0.0f }, _data->uiTitleLogo->tint);
    _data->uiTitleLogo->visible = true;
    _data->titleLogoAlpha = 0.0f;

    _data->uiCovering = ui::registerUIQuad(&engine->_loadedTextures["empty"]);
    _data->uiCovering->renderOrder = 100.0f;
    glm_vec3_copy(vec3{ 1000.0f, 1000.0f, 1.0f }, _data->uiCovering->scale);
    glm_vec4_copy(vec4{ 0.0f, 0.0f, 0.0f, 1.0f }, _data->uiCovering->tint);
    _data->uiCovering->visible = true;

    // Create bios.
    _data->bio1.name = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, -1.0f, "Nefertiti");
    glm_vec3_copy(vec3{ 10.0f, 1.0f, 0.0f }, _data->bio1.name->renderPosition);
    _data->bio1.name->scale = 1.0f;
    _data->bio1.name->excludeFromBulkRender = true;

    _data->bio1.description = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, 13.0f, "Mummy who loves\nmoney and status, and\nwill throw away all her\npreferences for them.");
    glm_vec3_copy(vec3{ 9.75f, -0.5f, 0.0f }, _data->bio1.description->renderPosition);
    _data->bio1.description->scale = 0.5f;
    _data->bio1.description->excludeFromBulkRender = true;

    _data->bio2.name = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, -1.0f, "Ophelia");
    glm_vec3_copy(vec3{ 3.0f, 1.0f, 0.0f }, _data->bio2.name->renderPosition);
    _data->bio2.name->scale = 1.0f;
    _data->bio2.name->excludeFromBulkRender = true;

    _data->bio2.description = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, 13.0f, "Ghost who gets spooked\neasily, hates her appear-\nance. Uncle beheaded her\nafter she rejected him.");
    glm_vec3_copy(vec3{ 2.6f, -0.5f, 0.0f }, _data->bio2.description->renderPosition);
    _data->bio2.description->scale = 0.5f;
    _data->bio2.description->excludeFromBulkRender = true;

    _data->bio3.name = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, -1.0f, "Isabella");
    glm_vec3_copy(vec3{ -4.0f, 1.0f, 0.0f }, _data->bio3.name->renderPosition);
    _data->bio3.name->scale = 1.0f;
    _data->bio3.name->excludeFromBulkRender = true;

    _data->bio3.description = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, 13.0f, "Blind vampire. Thus is\nvery picky about smells.\nShe is also a history buff\nand is 331 years old.");
    glm_vec3_copy(vec3{ -4.5f, -0.5f, 0.0f }, _data->bio3.description->renderPosition);
    _data->bio3.description->scale = 0.5f;
    _data->bio3.description->excludeFromBulkRender = true;

    _data->startGamePromptText = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::CENTER, textmesh::MID, -1.0f, "Press Spacebar to Start.\n\n\n\nPress Esc at Any Time to Quit.\nPress F11 to Toggle Fullscreen.");
    glm_vec3_copy(vec3{ 0.0f, -4.5f, 0.0f }, _data->startGamePromptText->renderPosition);
    _data->startGamePromptText->scale = 0.5f;
    _data->startGamePromptText->excludeFromBulkRender = true;

    // Play looped track.
    _data->titleMusicVolume = 0.0f;
    _data->titleMusicId = AudioEngine::getInstance().playSound("res/music/title.ogg", true, vec3{ 0.0f, 0.0f, 0.0f }, AudioEngine::getInstance().volumeToDb(_data->titleMusicVolume));
}

MainMenu::~MainMenu()
{
    ui::unregisterUIQuad(_data->uiTitleLogo);
    ui::unregisterUIQuad(_data->uiCovering);
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
    {
        _data->titleLogoAlpha += deltaTime / 2.0f * (_data->launchingCards ? -1.0f : 2.0f);
        _data->titleLogoAlpha = glm_clamp_zo(_data->titleLogoAlpha);
        glm_vec4_copy(vec4{ _data->titleLogoAlpha, _data->titleLogoAlpha, _data->titleLogoAlpha, _data->titleLogoAlpha }, _data->uiTitleLogo->tint);
        _data->uiCovering->tint[3] = 1.0f - _data->titleLogoAlpha;
        if (_data->titleLogoAlpha >= 1.0f)
        {
            _data->uiCovering->visible = false;  // Switch off covering permanently once everything loads up.
            _data->startGamePromptText->excludeFromBulkRender = false;  // Show start game prompt.
        }
    }

    if (!_data->launchingCards)
    {
        // Fade in title music.
        if (_data->titleMusicVolume < 1.0f)
        {
            _data->titleMusicVolume += deltaTime / 0.075f;
            if (_data->titleMusicVolume > 1.0f)
                _data->titleMusicVolume = 1.0f;
            AudioEngine::getInstance().setChannelVolume(_data->titleMusicId, AudioEngine::getInstance().volumeToDb(_data->titleMusicVolume));
        }

        if (_data->titleLogoAlpha >= 1.0f && input::onKeyJumpPress)  // Only start menu once the logo comes up fully.
        {
            _data->cardLaunchIntervalTimer = 0.0f;
            _data->currentCard = 0;
            _data->launchingCards = true;

            textmesh::regenerateTextMeshMesh(_data->startGamePromptText, "Press Spacebar to Start the Dating Game.");
            _data->startGamePromptText->excludeFromBulkRender = true;

            AudioEngine::getInstance().playSound("res/sfx/wip_start_game.wav", false);
        }
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
            auto& tarotCard = _data->tarotCardROs[_data->shufflingPerm[_data->currentCard]];
            glm_mat4_copy(_data->launchTransform, tarotCard->transformMatrix);

            // Choose whether card is drawn or discarded.
            if (_data->shufflingPerm[_data->currentCard] < 3)
            {
                tarotCard->animator->setTrigger("draw_to_" + std::to_string(_data->shufflingPerm[_data->currentCard] + 1));
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
                _data->engine->setLightDirection(vec4{ -0.243f, 0.740f, 0.627f, 0.0f });
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
    ImGui::Text("Title logo");
    ImGui::DragFloat3("TL rp", _data->uiTitleLogo->position);
    ImGui::DragFloat3("TL sca", _data->uiTitleLogo->scale);
    ImGui::Separator();

    imguiTextMesh2("bio1.name", _data->bio1.name);
    imguiTextMesh2("bio1.description", _data->bio1.description);
    imguiTextMesh2("bio2.name", _data->bio2.name);
    imguiTextMesh2("bio2.description", _data->bio2.description);
    imguiTextMesh2("bio3.name", _data->bio3.name);
    imguiTextMesh2("bio3.description", _data->bio3.description);
    imguiTextMesh2("startGamePromptText", _data->startGamePromptText);
}

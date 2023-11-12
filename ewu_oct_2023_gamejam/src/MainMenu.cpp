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
#include "Textbox.h"
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
    float_t cardLaunchInterval = 0.1f;
    float_t cardLaunchIntervalTimer = 0.0f;
    std::vector<size_t> tarotCardDrawIndices;
    size_t currentDrawIdx = 0;
    size_t currentCard = 0;
    bool launchingCards = false;
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
                }
            }
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
}

MainMenu::~MainMenu()
{
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
    // if (input::onKeyJumpPress)
    // {
    //     globalState::resetGameState();
    //     scene::loadScene("hello_hello_world.ssdat", true);
    // }
    if (input::onKeyJumpPress)
    {
        _data->cardLaunchIntervalTimer = 0.0f;
        _data->currentDrawIdx = 0;
        _data->currentCard = 0;
        _data->launchingCards = true;
    }

    if (_data->launchingCards)
    {
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
            _data->launchingCards = false;
    }
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

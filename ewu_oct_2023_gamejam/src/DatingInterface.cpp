#include "DatingInterface.h"

#include <iostream>
#include "VulkanEngine.h"
#include "GlobalState.h"
#include "RenderObject.h"
#include "InputManager.h"
#include "UIQuad.h"
#include "TextMesh.h"
#include "Camera.h"
#include "imgui/imgui.h"


struct DatingInterface_XData
{
    RenderObjectManager* rom;
    Camera* camera;

    RenderObject* renderObj;

    ui::UIQuad* dateQuads[3];
    ui::UIQuad* dateThinkingBoxTex;
    ui::UIQuad* dateThinkingBoxFill;
    ui::UIQuad* dateThinkingTrailingBubbles;
    textmesh::TextMesh* dateSpeechText = nullptr;
    ui::UIQuad* dateSpeechBox;

    ui::UIQuad* contestantThinkingBoxTex;
    ui::UIQuad* contestantThinkingBoxFill;
    ui::UIQuad* contestantThinkingTrailingBubbles;
    textmesh::TextMesh* contestantSpeechText = nullptr;
    ui::UIQuad* contestantSpeechBox;

    ui::UIQuad* menuSelectingCursor;

    struct SelectionButton
    {
        textmesh::TextMesh* text = nullptr;
        ui::UIQuad* background = nullptr;
    };
    #define NUM_SELECTION_BUTTONS 6
    SelectionButton buttons[NUM_SELECTION_BUTTONS];

    int8_t menuSelectionIdx = 0;
};

DatingInterface::DatingInterface(EntityManager* em, RenderObjectManager* rom, Camera* camera, VulkanEngine* engine, DataSerialized* ds) : Entity(em, ds), _data(new DatingInterface_XData())
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

    _data->dateQuads[0] = ui::registerUIQuad(&engine->_loadedTextures["Date1"]);
    _data->dateQuads[1] = ui::registerUIQuad(&engine->_loadedTextures["Date2"]);
    _data->dateQuads[2] = ui::registerUIQuad(&engine->_loadedTextures["Date3"]);
    _data->dateThinkingBoxTex = ui::registerUIQuad(&engine->_loadedTextures["ThinkingBox"]);
    _data->dateThinkingBoxFill = ui::registerUIQuad(nullptr);
    _data->dateThinkingTrailingBubbles = ui::registerUIQuad(&engine->_loadedTextures["ThinkingBoxTrailLeft"]);
    _data->dateSpeechText = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, "message");
    _data->contestantThinkingBoxTex = ui::registerUIQuad(&engine->_loadedTextures["ThinkingBox"]);
    _data->contestantThinkingBoxFill = ui::registerUIQuad(nullptr);
    _data->contestantSpeechText = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, "message");
    _data->contestantThinkingTrailingBubbles = ui::registerUIQuad(&engine->_loadedTextures["ThinkingBoxTrailRight"]);
    _data->menuSelectingCursor = ui::registerUIQuad(&engine->_loadedTextures["MenuSelectingCursor"]);
    _data->dateSpeechBox = ui::registerUIQuad(&engine->_loadedTextures["DateSpeechBox"]);
    _data->contestantSpeechBox = ui::registerUIQuad(&engine->_loadedTextures["ContestantSpeechBox"]);
    for (size_t i = 0; i < NUM_SELECTION_BUTTONS; i++)
    {
        _data->buttons[i].text = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::TOP, "message");
        _data->buttons[i].background = ui::registerUIQuad(&engine->_loadedTextures["SpeechSelectionButton"]);
    }

    globalState::phase2.registerDatingInterface(this);
}

DatingInterface::~DatingInterface()
{
    ui::unregisterUIQuad(_data->dateQuads[0]);
    ui::unregisterUIQuad(_data->dateQuads[1]);
    ui::unregisterUIQuad(_data->dateQuads[2]);
    ui::unregisterUIQuad(_data->dateThinkingBoxTex);
    ui::unregisterUIQuad(_data->dateThinkingBoxFill);
    ui::unregisterUIQuad(_data->dateThinkingTrailingBubbles);
    textmesh::destroyAndUnregisterTextMesh(_data->dateSpeechText);
    ui::unregisterUIQuad(_data->contestantThinkingBoxTex);
    ui::unregisterUIQuad(_data->contestantThinkingBoxFill);
    textmesh::destroyAndUnregisterTextMesh(_data->contestantSpeechText);
    ui::unregisterUIQuad(_data->contestantThinkingTrailingBubbles);
    ui::unregisterUIQuad(_data->menuSelectingCursor);
    ui::unregisterUIQuad(_data->dateSpeechBox);
    ui::unregisterUIQuad(_data->contestantSpeechBox);
    for (size_t i = 0; i < NUM_SELECTION_BUTTONS; i++)
    {
        textmesh::destroyAndUnregisterTextMesh(_data->buttons[i].text);
        ui::unregisterUIQuad(_data->buttons[i].background);
    }
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
    _data->uiQuad->visible = true;

    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;
}

void DatingInterface::deactivate()
{
    _data->uiQuad->visible = false;

    Entity::_enablePhysicsUpdate = false;
    Entity::_enableUpdate = false;
    Entity::_enableLateUpdate = false;
}

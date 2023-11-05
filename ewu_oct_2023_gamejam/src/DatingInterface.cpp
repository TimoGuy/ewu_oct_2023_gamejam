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
#include "imgui/ImGuizmo.h"


struct DatingInterface_XData
{
    RenderObjectManager* rom;
    Camera* camera;

    RenderObject* renderObj;

    size_t dateIdx;
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
    vec3    startingButtonPosition;
    float_t buttonStrideY;

    int8_t menuSelectionIdx;

    enum class DATING_STAGE
    {
        NONE = 0,
        CONTESTANT_ASK_SELECT,  // This could either go to CONTESTANT_ASK_EXECUTE or CONTESTANT_REJECT_DATE.
        CONTESTANT_ASK_EXECUTE,
        CONTESTANT_ASK_DATE_ON_DATE,
        CONTESTANT_REJECT_DATE,  // Ends dating interface after this.
        DATE_ANSWER_THINKING,
        DATE_ANSWER_EXECUTE,
        DATE_ASK_THINKING,  // This could either go to DATE_ASK_EXECUTE or DATE_REJECT_CONTESTANT after the thinking.
        DATE_ASK_EXECUTE,
        CONTESTANT_ANSWER_SELECT,
        CONTESTANT_ANSWER_EXECUTE,
        DATE_ACCEPT_DATE_INVITE,  // Ends with a game over; you win screen.
        DATE_REJECT_CONTESTANT,    // Ends dating interface after this.
        LAST_STAGE = DATE_REJECT_CONTESTANT,
    } currentStage = DATING_STAGE::NONE;
};

void initializePositionings(DatingInterface_XData* d)
{
    // @HARDCODE: this is all hardcoded!!!
    for (size_t i = 0; i < 3; i++)
    {
        d->dateQuads[i]->renderOrder = 10.0f;
        glm_translate(d->dateQuads[i]->transform, vec3{ -456.0f, 0.0f, 0.0f });
        glm_scale(d->dateQuads[i]->transform, vec3{ 500.0f, 500.0f, 1.0f });
    }

    d->dateThinkingBoxTex->renderOrder = 0.0f;
    glm_translate(d->dateThinkingBoxTex->transform, vec3{ 60.0f, 341.0f, 0.0f });
    glm_scale(d->dateThinkingBoxTex->transform, vec3{ 200.0f, 100.0f, 1.0f });
    glm_vec4_copy(vec4{ 0.5647058824f, 0.2f, 0.2f, 1.0f }, d->dateThinkingBoxTex->tint);

    d->dateThinkingBoxFill->renderOrder = 1.0f;
    glm_translate(d->dateThinkingBoxFill->transform, vec3{ -100.0f, 373.0f, 0.0f });
    glm_scale(d->dateThinkingBoxFill->transform, vec3{ 0.0f, 25.0f, 1.0f });
    glm_vec4_copy(vec4{ 0.0f, 0.0f, 0.0f, 1.0f }, d->dateThinkingBoxFill->tint);

    d->dateThinkingTrailingBubbles->renderOrder = 0.0f;
    glm_translate(d->dateThinkingTrailingBubbles->transform, vec3{ -101.0f, 276.0f, 0.0f });
    glm_scale(d->dateThinkingTrailingBubbles->transform, vec3{ 66.0f, 35.0f, 1.0f });

    d->dateSpeechBox->renderOrder = 0.0f;
    glm_translate(d->dateSpeechBox->transform, vec3{ 31.0f, 373.0f, 0.0f });
    glm_scale(d->dateSpeechBox->transform, vec3{ 188.0f, 50.0f, 1.0f });
    glm_vec4_copy(vec4{ 0.5647058824f, 0.2f, 0.2f, 1.0f }, d->dateSpeechBox->tint);

    d->contestantThinkingBoxTex->renderOrder = 0.0f;
    glm_translate(d->contestantThinkingBoxTex->transform, vec3{ 310.0f, 109.0f, 0.0f });
    glm_scale(d->contestantThinkingBoxTex->transform, vec3{ 200.0f, 100.0f, 1.0f });
    glm_vec4_copy(vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, d->contestantThinkingBoxTex->tint);

    d->contestantThinkingBoxFill->renderOrder = 1.0f;
    glm_translate(d->contestantThinkingBoxFill->transform, vec3{ 148.0f, 140.0f, 0.0f });
    glm_scale(d->contestantThinkingBoxFill->transform, vec3{ 0.0f, 25.0f, 1.0f });
    glm_vec4_copy(vec4{ 0.0f, 0.0f, 0.0f, 1.0f }, d->contestantThinkingBoxFill->tint);

    d->contestantThinkingTrailingBubbles->renderOrder = 0.0f;
    glm_translate(d->contestantThinkingTrailingBubbles->transform, vec3{ 456.0f, 39.0f, 0.0f });
    glm_scale(d->contestantThinkingTrailingBubbles->transform, vec3{ 66.0f, 35.0f, 1.0f });

    d->contestantSpeechBox->renderOrder = 0.0f;
    glm_translate(d->contestantSpeechBox->transform, vec3{ 292.0f, 142.0f, 0.0f });
    glm_scale(d->contestantSpeechBox->transform, vec3{ 188.0f, 50.0f, 1.0f });
    glm_vec4_copy(vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, d->contestantSpeechBox->tint);

    d->menuSelectingCursor->renderOrder = -1.0f;
    glm_vec4_copy(vec4{ 0.6823529412f, 0.8196078431f, 0.0313725490f, 1.0f }, d->menuSelectingCursor->tint);

    d->buttonStrideY = -80.0f;
    glm_vec3_copy(vec3{ 0.0f, -40.0f, 0.0f }, d->startingButtonPosition);

    vec3 currentPosition;
    glm_vec3_copy(d->startingButtonPosition, currentPosition);
    for (size_t i = 0; i < NUM_SELECTION_BUTTONS; i++)
    {
        if (i > 0)
            currentPosition[1] += d->buttonStrideY;

        auto& b = d->buttons[i];
        b.background->renderOrder = 0.0f;
        glm_translate(b.background->transform, currentPosition);
        glm_scale(b.background->transform, vec3{ 150.0f, 50.0f, 1.0f });
        glm_vec4_copy(vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, b.background->tint);  // Fade out if disabled!
        vec3 offset = { -134.0f, 7.0f, 0.0f };
        glm_vec3_add(currentPosition, offset, b.text->renderPosition);
    }
}

void setMenuSelectingCursor(DatingInterface_XData* d)
{
    vec3 pos = {
        d->startingButtonPosition[0] - 200.0f,
        d->startingButtonPosition[1] + d->buttonStrideY * d->menuSelectionIdx,
        d->startingButtonPosition[2]
    };
    glm_mat4_identity(d->menuSelectingCursor->transform);
    glm_translate(d->menuSelectingCursor->transform, pos);
    glm_scale(d->menuSelectingCursor->transform, vec3{ 20.0f, 20.0f, 1.0f });
}

void setupStage(DatingInterface_XData* d, DatingInterface_XData::DATING_STAGE newStage)
{
    d->currentStage = newStage;

    d->dateQuads[0]->visible = (d->currentStage > DatingInterface_XData::DATING_STAGE::NONE && d->dateIdx == 0);
    d->dateQuads[1]->visible = (d->currentStage > DatingInterface_XData::DATING_STAGE::NONE && d->dateIdx == 1);
    d->dateQuads[2]->visible = (d->currentStage > DatingInterface_XData::DATING_STAGE::NONE && d->dateIdx == 2);

    d->dateThinkingBoxTex->visible =
        d->dateThinkingBoxFill->visible =
        d->dateThinkingTrailingBubbles->visible =
            (d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ANSWER_THINKING ||
            d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ASK_THINKING);

    d->dateSpeechBox->visible =
        (d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ANSWER_EXECUTE ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ASK_EXECUTE ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ACCEPT_DATE_INVITE ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_REJECT_CONTESTANT);
    d->dateSpeechText->excludeFromBulkRender = !d->dateSpeechBox->visible;

    d->contestantThinkingBoxTex->visible =
        d->contestantThinkingBoxFill->visible =
        d->contestantThinkingTrailingBubbles->visible =
            (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_SELECT ||
            d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_SELECT);

    bool disableLastTwo = (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_SELECT);
    d->menuSelectingCursor->visible = d->contestantThinkingBoxTex->visible;
    for (size_t i = 0; i < NUM_SELECTION_BUTTONS; i++)
    {
        d->buttons[i].text->excludeFromBulkRender = !d->contestantThinkingBoxTex->visible;
        d->buttons[i].background->visible = d->contestantThinkingBoxTex->visible;
        if (i >= NUM_SELECTION_BUTTONS - 2)
            d->buttons[i].background->tint[3] = (disableLastTwo ? 0.25f : 1.0f);
    }

    d->contestantSpeechBox->visible =
        (d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ANSWER_EXECUTE ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ASK_EXECUTE ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_DATE_ON_DATE ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_REJECT_DATE);
    d->contestantSpeechText->excludeFromBulkRender = !d->contestantSpeechBox->visible;

    d->menuSelectionIdx = 0;  // Reset menu selection value.
    setMenuSelectingCursor(d);
}

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
    _data->dateSpeechText = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::MID, "message");
    _data->dateSpeechText->isPositionScreenspace = true;
    _data->dateSpeechText->scale = 40.0f;
    _data->contestantThinkingBoxTex = ui::registerUIQuad(&engine->_loadedTextures["ThinkingBox"]);
    _data->contestantThinkingBoxFill = ui::registerUIQuad(nullptr);
    _data->contestantSpeechText = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::MID, "message");
    _data->contestantSpeechText->isPositionScreenspace = true;
    _data->contestantSpeechText->scale = 40.0f;
    _data->contestantThinkingTrailingBubbles = ui::registerUIQuad(&engine->_loadedTextures["ThinkingBoxTrailRight"]);
    _data->menuSelectingCursor = ui::registerUIQuad(&engine->_loadedTextures["MenuSelectingCursor"]);
    _data->dateSpeechBox = ui::registerUIQuad(&engine->_loadedTextures["DateSpeechBox"]);
    _data->contestantSpeechBox = ui::registerUIQuad(&engine->_loadedTextures["ContestantSpeechBox"]);
    for (size_t i = 0; i < NUM_SELECTION_BUTTONS; i++)
    {
        _data->buttons[i].text = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::MID, "message\nsecondline.");
        _data->buttons[i].text->isPositionScreenspace = true;
        _data->buttons[i].text->scale = 40.0f;
        _data->buttons[i].background = ui::registerUIQuad(&engine->_loadedTextures["SpeechSelectionButton"]);
    }

    globalState::phase2.registerDatingInterface(this);

    initializePositionings(_data);

    setupStage(_data, DatingInterface_XData::DATING_STAGE::NONE);
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
    if (_data->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_SELECT ||
        _data->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_SELECT)
    {
        bool changed = false;
        if (input::onKeyUpPress)
        {
            if (_data->menuSelectionIdx > 0)
                _data->menuSelectionIdx--;
            else
                _data->menuSelectionIdx = NUM_SELECTION_BUTTONS - 1;
            changed = true;
        }
        if (input::onKeyDownPress)
        {
            if (_data->menuSelectionIdx < NUM_SELECTION_BUTTONS - 1)
                _data->menuSelectionIdx++;
            else
                _data->menuSelectionIdx = 0;
            changed = true;
        }
        setMenuSelectingCursor(_data);

        if (input::onKeyJumpPress)
        {

        }
    }

    // globalState::phase1.transitionToPhase1FromPhase2();
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

void imguiUIQuad(const std::string& uiQuadName, ui::UIQuad* uiQuad)
{
    std::string nameSuffix = "##69420uiquad" + uiQuadName;
    if (ImGui::TreeNode((uiQuadName + nameSuffix).c_str()))
    {
        ImGui::Checkbox(("visible" + nameSuffix).c_str(), &uiQuad->visible);
        ImGui::ColorPicker4(("tint" + nameSuffix).c_str(), uiQuad->tint);
        ImGui::DragFloat(("renderOrder" + nameSuffix).c_str(), &uiQuad->renderOrder);
        
        // @COPYPASTA: from VulkanEngine.cpp
        {
            vec3 position, eulerAngles, scale;
            ImGuizmo::DecomposeMatrixToComponents(
                (const float_t*)*uiQuad->transform,
                position,
                eulerAngles,
                scale
            );

            bool changed = false;
            ImGui::Text("Transform");
            changed |= ImGui::DragFloat3(("Pos" + nameSuffix).c_str(), position);
            changed |= ImGui::DragFloat3(("Rot" + nameSuffix).c_str(), eulerAngles);
            changed |= ImGui::DragFloat3(("Sca" + nameSuffix).c_str(), scale);
            if (changed)
            {
                // Recompose the matrix
                // @TODO: Figure out when to invalidate the cache bc the euler angles will reset!
                //        Or... maybe invalidating the cache isn't necessary for this window????
                ImGuizmo::RecomposeMatrixFromComponents(
                    position,
                    eulerAngles,
                    scale,
                    (float_t*)*uiQuad->transform
                );
            }
        }
        ////////////////////////////////////

        ImGui::TreePop();
        ImGui::Separator();
    }
}

void imguiTextMesh(const std::string& textMeshName, textmesh::TextMesh* textMesh)
{
    std::string nameSuffix = "##69420textmesh" + textMeshName;
    if (ImGui::TreeNode((textMeshName + nameSuffix).c_str()))
    {
        ImGui::Checkbox(("excludeFromBulkRender" + nameSuffix).c_str(), &textMesh->excludeFromBulkRender);
        ImGui::DragFloat3(("renderPosition" + nameSuffix).c_str(), textMesh->renderPosition);
        ImGui::DragFloat(("scale" + nameSuffix).c_str(), &textMesh->scale);

        ImGui::TreePop();
        ImGui::Separator();
    }
}

void DatingInterface::renderImGui()
{
    imguiUIQuad("dateQuads[0]", _data->dateQuads[0]);
    imguiUIQuad("dateQuads[1]", _data->dateQuads[1]);
    imguiUIQuad("dateQuads[2]", _data->dateQuads[2]);
    imguiUIQuad("dateThinkingBoxTex", _data->dateThinkingBoxTex);
    imguiUIQuad("dateThinkingBoxFill", _data->dateThinkingBoxFill);
    imguiUIQuad("dateThinkingTrailingBubbles", _data->dateThinkingTrailingBubbles);
    imguiTextMesh("dateSpeechText", _data->dateSpeechText);
    imguiUIQuad("dateSpeechBox", _data->dateSpeechBox);

    imguiUIQuad("contestantThinkingBoxTex", _data->contestantThinkingBoxTex);
    imguiUIQuad("contestantThinkingBoxFill", _data->contestantThinkingBoxFill);
    imguiUIQuad("contestantThinkingTrailingBubbles", _data->contestantThinkingTrailingBubbles);
    imguiTextMesh("contestantSpeechText", _data->contestantSpeechText);
    imguiUIQuad("contestantSpeechBox", _data->contestantSpeechBox);

    imguiUIQuad("menuSelectingCursor", _data->menuSelectingCursor);

    ImGui::Separator();
    ImGui::Text("Selection buttons");

    for (size_t i = 0; i < NUM_SELECTION_BUTTONS; i++)
    {
        imguiUIQuad("buttons[" + std::to_string(i) + "].background", _data->buttons[i].background);
        imguiTextMesh("buttons[" + std::to_string(i) + "].text", _data->buttons[i].text);
    }
}

void DatingInterface::activate(size_t dateIdx)
{
    _data->camera->mainCamMode.setMainCamTargetObject(_data->renderObj);

    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;

    _data->dateIdx = dateIdx;
    setupStage(_data, DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_SELECT);
}

void DatingInterface::deactivate()
{
    setupStage(_data, DatingInterface_XData::DATING_STAGE::NONE);

    Entity::_enablePhysicsUpdate = false;
    Entity::_enableUpdate = false;
    Entity::_enableLateUpdate = false;
}

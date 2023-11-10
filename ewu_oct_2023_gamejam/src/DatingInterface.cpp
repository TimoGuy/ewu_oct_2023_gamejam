#include "DatingInterface.h"

#include <iostream>
#include "VulkanEngine.h"
#include "GlobalState.h"
#include "RenderObject.h"
#include "InputManager.h"
#include "RandomNumberGenerator.h"
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
    float_t dateThinkingTimer = 0.0f;

    ui::UIQuad* contestantThinkingBoxTex;
    ui::UIQuad* contestantThinkingBoxFill;
    ui::UIQuad* contestantThinkingTrailingBubbles;
    textmesh::TextMesh* contestantSpeechText = nullptr;
    ui::UIQuad* contestantSpeechBox;
    float_t contestantThinkingTimer = 0.0f;

    float_t boxFillXOffset = -160.0f;
    float_t boxFillAmountMultiplier = 137.0f;
    const float_t contestantThinkingTimerTime = 10.0f;
    const float_t dateThinkingTimerTime = 5.0f;

    struct SelectionButton
    {
        textmesh::TextMesh* text = nullptr;
        ui::UIQuad* background = nullptr;
        bool disabled = false;
    };
    #define NUM_SELECTION_BUTTONS 6
    SelectionButton buttons[NUM_SELECTION_BUTTONS];
    vec3    startingButtonPosition;
    float_t buttonStrideY;

    ui::UIQuad* menuSelectingCursor;
    int8_t menuSelectionIdx;

    std::vector<size_t> selectionShuffledIndices = { 0, 1, 2, 3 };
    struct SelectionText
    {
        std::string contestantQuestionOrAnswer;
        std::string dateAnswer;
        bool disabled;
    };
    std::vector<SelectionText> selectionTexts = {
        { "VERY_GOOD", "VERY_GOOD_ANSWER", false },
        { "GOOD", "GOOD_ANSWER", false },
        { "BAD", "BAD_ANSWER", false },
        { "VERY_BAD", "VERY_BAD_ANSWER", false },
    };

    bool gotoRejection = false;
    int8_t dateProcessingBeingAskedOut = 0;  // 0: inactive | 1: rejects date invite | 2: accepts date invite.

    float_t stageTransitionTimer = -1.0f;  // This works by counting down if > 0.0, and if the timer goes <= 0.0, then set the next stage via some state machine-like thing.
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
        glm_vec3_copy(vec3{ -456.0f, 0.0f, 0.0f }, d->dateQuads[i]->position);
        glm_vec3_copy(vec3{ 500.0f, 500.0f, 1.0f }, d->dateQuads[i]->scale);
    }

    d->dateThinkingBoxTex->renderOrder = 0.0f;
    glm_vec3_copy(vec3{ 60.0f, 341.0f, 0.0f }, d->dateThinkingBoxTex->position);
    glm_vec3_copy(vec3{ 200.0f, 100.0f, 1.0f }, d->dateThinkingBoxTex->scale);
    glm_vec4_copy(vec4{ 0.5647058824f, 0.2f, 0.2f, 1.0f }, d->dateThinkingBoxTex->tint);

    d->dateThinkingBoxFill->renderOrder = 1.0f;
    glm_vec3_copy(vec3{ -100.0f, 373.0f, 0.0f }, d->dateThinkingBoxFill->position);
    glm_vec3_copy(vec3{ 0.0f, 25.0f, 1.0f }, d->dateThinkingBoxFill->scale);
    glm_vec4_copy(vec4{ 0.0f, 0.0f, 0.0f, 1.0f }, d->dateThinkingBoxFill->tint);

    d->dateThinkingTrailingBubbles->renderOrder = 0.0f;
    glm_vec3_copy(vec3{ -101.0f, 276.0f, 0.0f }, d->dateThinkingTrailingBubbles->position);
    glm_vec3_copy(vec3{ 66.0f, 35.0f, 1.0f }, d->dateThinkingTrailingBubbles->scale);
    glm_vec4_copy(vec4{ 0.5647058824f, 0.2f, 0.2f, 1.0f }, d->dateThinkingTrailingBubbles->tint);

    d->dateSpeechBox->renderOrder = 0.0f;
    glm_vec3_copy(vec3{ 31.0f, 373.0f, 0.0f }, d->dateSpeechBox->position);
    glm_vec3_copy(vec3{ 188.0f, 50.0f, 1.0f }, d->dateSpeechBox->scale);
    d->dateSpeechBox->useNineSlicing = true;
    d->dateSpeechBox->nineSlicingSizeX = 20.0f;
    d->dateSpeechBox->nineSlicingSizeY = 20.0f;
    glm_vec4_copy(vec4{ 0.5647058824f, 0.2f, 0.2f, 1.0f }, d->dateSpeechBox->tint);

    glm_vec3_copy(vec3{ -133.0f, 385.0f, 0.0f }, d->dateSpeechText->renderPosition);

    d->contestantThinkingBoxTex->renderOrder = 0.0f;
    glm_vec3_copy(vec3{ 310.0f, 109.0f, 0.0f }, d->contestantThinkingBoxTex->position);
    glm_vec3_copy(vec3{ 200.0f, 100.0f, 1.0f }, d->contestantThinkingBoxTex->scale);
    glm_vec4_copy(vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, d->contestantThinkingBoxTex->tint);

    d->contestantThinkingBoxFill->renderOrder = 1.0f;
    glm_vec3_copy(vec3{ 148.0f, 140.0f, 0.0f }, d->contestantThinkingBoxFill->position);
    glm_vec3_copy(vec3{ 0.0f, 25.0f, 1.0f }, d->contestantThinkingBoxFill->scale);
    glm_vec4_copy(vec4{ 0.0f, 0.0f, 0.0f, 1.0f }, d->contestantThinkingBoxFill->tint);

    d->contestantThinkingTrailingBubbles->renderOrder = 0.0f;
    glm_vec3_copy(vec3{ 456.0f, 39.0f, 0.0f }, d->contestantThinkingTrailingBubbles->position);
    glm_vec3_copy(vec3{ 66.0f, 35.0f, 1.0f }, d->contestantThinkingTrailingBubbles->scale);

    d->contestantSpeechBox->renderOrder = 0.0f;
    glm_vec3_copy(vec3{ 292.0f, 142.0f, 0.0f }, d->contestantSpeechBox->position);
    glm_vec3_copy(vec3{ 188.0f, 50.0f, 1.0f }, d->contestantSpeechBox->scale);
    d->contestantSpeechBox->useNineSlicing = true;
    d->contestantSpeechBox->nineSlicingSizeX = 20.0f;
    d->contestantSpeechBox->nineSlicingSizeY = 20.0f;
    glm_vec4_copy(vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, d->contestantSpeechBox->tint);

    glm_vec3_copy(vec3{ 122.0f, 148.0f, 0.0f }, d->contestantSpeechText->renderPosition);

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
        glm_vec3_copy(currentPosition, b.background->position);
        glm_vec3_copy(vec3{ 150.0f, 50.0f, 1.0f }, b.background->scale);
        b.background->useNineSlicing = true;
        b.background->nineSlicingSizeX = 20.0f;
        b.background->nineSlicingSizeY = 20.0f;
        glm_vec4_copy(vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, b.background->tint);  // Fade out if disabled!
        vec3 offset = { -134.0f, 7.0f, 0.0f };
        glm_vec3_add(currentPosition, offset, b.text->renderPosition);
    }
}

void setMenuSelectingCursor(DatingInterface_XData* d, int32_t directionOfMovement)
{
    // @NOTE: @CODESMELL: this shouldn't be here.
    while (d->buttons[d->menuSelectionIdx].disabled)
    {
        if (d->menuSelectionIdx + directionOfMovement < 0)
            d->menuSelectionIdx = NUM_SELECTION_BUTTONS - 1;
        else if (d->menuSelectionIdx + directionOfMovement >= NUM_SELECTION_BUTTONS)
            d->menuSelectionIdx = 0;
        else
            d->menuSelectionIdx += directionOfMovement;
    }
    /////////////////////////////////////////////

    vec3 pos = {
        d->startingButtonPosition[0] - 200.0f,
        d->startingButtonPosition[1] + d->buttonStrideY * d->menuSelectionIdx,
        d->startingButtonPosition[2]
    };
    glm_vec3_copy(pos, d->menuSelectingCursor->position);
    glm_vec3_copy(vec3{ 20.0f, 20.0f, 1.0f }, d->menuSelectingCursor->scale);
}

void setupStage(DatingInterface_XData* d, DatingInterface_XData::DATING_STAGE newStage)
{
    d->currentStage = newStage;
    d->stageTransitionTimer = -1.0f;  // Clear timer.

    d->dateQuads[0]->visible = (d->currentStage > DatingInterface_XData::DATING_STAGE::NONE && d->dateIdx == 0);
    d->dateQuads[1]->visible = (d->currentStage > DatingInterface_XData::DATING_STAGE::NONE && d->dateIdx == 1);
    d->dateQuads[2]->visible = (d->currentStage > DatingInterface_XData::DATING_STAGE::NONE && d->dateIdx == 2);

    d->dateThinkingBoxTex->visible =
        d->dateThinkingBoxFill->visible =
        d->dateThinkingTrailingBubbles->visible =
            (d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ANSWER_THINKING ||
            d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ASK_THINKING);
    if (d->dateThinkingBoxFill->visible)
        d->dateThinkingTimer = 0.0f;

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
    if (d->contestantThinkingBoxFill->visible)
        d->contestantThinkingTimer = 0.0f;

    bool disableLastTwo = (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_SELECT);
    d->buttons[NUM_SELECTION_BUTTONS - 2].disabled = disableLastTwo;
    d->buttons[NUM_SELECTION_BUTTONS - 1].disabled = disableLastTwo;

    d->menuSelectingCursor->visible = d->contestantThinkingBoxTex->visible;
    for (size_t i = 0; i < NUM_SELECTION_BUTTONS; i++)
    {
        d->buttons[i].text->excludeFromBulkRender = !d->contestantThinkingBoxTex->visible;
        d->buttons[i].background->visible = d->contestantThinkingBoxTex->visible;
        d->buttons[i].background->tint[3] = (d->buttons[i].disabled ? 0.25f : 1.0f);
    }

    d->contestantSpeechBox->visible =
        (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_EXECUTE ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_EXECUTE ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_DATE_ON_DATE ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_REJECT_DATE);
    d->contestantSpeechText->excludeFromBulkRender = !d->contestantSpeechBox->visible;

    // Setup dialogue options.
    bool setupDialogueOptions = false;
    if (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_SELECT)
    {
        // @TODO: load in the respective asks.
        //////////////////////////////////////
        auto& dateProps = globalState::dates[globalState::phase2.dateIdx];
        auto& q = dateProps.contestantQuestions[dateProps.currentContestantQuestionIdx];
        d->selectionTexts[0] = { q.question0.text, q.answer0.text, q.question0.used };
        d->selectionTexts[1] = { q.question1.text, q.answer1.text, q.question1.used };
        d->selectionTexts[2] = { q.question2.text, q.answer2.text, q.question2.used };
        d->selectionTexts[3] = { q.question3.text, q.answer3.text, q.question3.used };

        setupDialogueOptions = true;
    }
    else if (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_SELECT)
    {
        // @TODO: load in the respective answers.
        /////////////////////////////////////////
        auto& dateProps = globalState::dates[globalState::phase2.dateIdx];
        auto& q = dateProps.dateQuestions[dateProps.currentDateQuestionIdx];
        d->selectionTexts[0] = { q.veryGoodAnswer.text, "", false };
        d->selectionTexts[1] = { q.goodAnswer.text, "", false };
        d->selectionTexts[2] = { q.badAnswer.text, "", false };
        d->selectionTexts[3] = { q.veryBadAnswer.text, "", false };

        setupDialogueOptions = true;
    }
    if (setupDialogueOptions)
    {
        rng::shuffleVectorSizeType(d->selectionShuffledIndices);
        size_t i = 0;
        for (auto& ssi : d->selectionShuffledIndices)
        {
            textmesh::regenerateTextMeshMesh(d->buttons[i].text, d->selectionTexts[ssi].contestantQuestionOrAnswer);
            d->buttons[i].disabled = d->selectionTexts[ssi].disabled;
            i++;
        }

        d->menuSelectionIdx = 0;  // Reset menu selection value.
        setMenuSelectingCursor(d, 1);
    }

    // Setup dialogue executions.
    if (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_EXECUTE ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_EXECUTE)
    {
        auto& dateProps = globalState::dates[globalState::phase2.dateIdx];
        if (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_EXECUTE)
        {
            switch (d->selectionShuffledIndices[(size_t)d->menuSelectionIdx])
            {
                case 0:
                    dateProps.contestantQuestions[dateProps.currentContestantQuestionIdx].question0.used = true;
                    break;

                case 1:
                    dateProps.contestantQuestions[dateProps.currentContestantQuestionIdx].question1.used = true;
                    break;

                case 2:
                    dateProps.contestantQuestions[dateProps.currentContestantQuestionIdx].question2.used = true;
                    break;

                case 3:
                    dateProps.contestantQuestions[dateProps.currentContestantQuestionIdx].question3.used = true;
                    break;
            }

            dateProps.currentContestantQuestionIdx = (dateProps.currentContestantQuestionIdx + 1) % dateProps.contestantQuestions.size();
        }
        if (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_EXECUTE)
        {
            auto& dq = dateProps.dateQuestions[dateProps.currentDateQuestionIdx];
            if (!dq.isLastQuestion)
            {
                dq.question.used = true;
                while (dateProps.dateQuestions[dateProps.currentDateQuestionIdx].question.used)
                    dateProps.currentDateQuestionIdx = (dateProps.currentDateQuestionIdx + 1) % dateProps.dateQuestions.size();
            }
        }
        textmesh::regenerateTextMeshMesh(d->contestantSpeechText, d->selectionTexts[d->selectionShuffledIndices[(size_t)d->menuSelectionIdx]].contestantQuestionOrAnswer);
        textmesh::regenerateTextMeshMesh(d->dateSpeechText, d->selectionTexts[d->selectionShuffledIndices[(size_t)d->menuSelectionIdx]].dateAnswer);
        d->stageTransitionTimer = 1.5f;  // Give time to read dialog.
    }
    if (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_REJECT_DATE ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_REJECT_CONTESTANT)
    {
        textmesh::regenerateTextMeshMesh(
            (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_REJECT_DATE ?
                d->contestantSpeechText :
                d->dateSpeechText),
            "Sorry, I don\'t think we\'re a good match for each other."
        );
        d->stageTransitionTimer = 2.0f;
    }
    if (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_DATE_ON_DATE)
    {
        textmesh::regenerateTextMeshMesh(
            d->contestantSpeechText,
            "Will you go on a date with me?"
        );
        d->stageTransitionTimer = 2.0f;
    }
    if (d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ACCEPT_DATE_INVITE)
    {
        textmesh::regenerateTextMeshMesh(
            d->dateSpeechText,
            "Yes. I would love to."
        );
        d->stageTransitionTimer = 3.0f;
    }

    // Setup date thinkings.
    if (d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ASK_THINKING ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ANSWER_THINKING)
    {
        if (d->dateProcessingBeingAskedOut == 0)
        {
            if (d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ASK_THINKING)
            {
                auto& dateProps = globalState::dates[globalState::phase2.dateIdx];
                auto& q = dateProps.dateQuestions[dateProps.currentDateQuestionIdx];
                textmesh::regenerateTextMeshMesh(d->dateSpeechText, q.question.text);
            }
            d->stageTransitionTimer = rng::randomRealRange(0.5f, d->dateThinkingTimerTime);
        }
        else
        {
            auto& dateProps = globalState::dates[globalState::phase2.dateIdx];
            d->stageTransitionTimer = rng::randomRealRange(dateProps.acceptDateInviteWaitTimeRange[0], dateProps.acceptDateInviteWaitTimeRange[1]);
        }
    }
    if (d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ASK_EXECUTE ||
        d->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ANSWER_EXECUTE)
    {
        d->stageTransitionTimer = 1.5f;
    }
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
        if (i >= NUM_SELECTION_BUTTONS - 2)
        {
            if (i == NUM_SELECTION_BUTTONS - 2)
                _data->buttons[i].text = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::MID, "Sorry, I don\'t think we\'re a good match for each other.");
            else if (i == NUM_SELECTION_BUTTONS - 1)
                _data->buttons[i].text = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::MID, "Will you go on a date with me?");
        }
        else
            _data->buttons[i].text = textmesh::createAndRegisterTextMesh("defaultFont", textmesh::LEFT, textmesh::MID, "DEBUGmessage\nsecondline.");
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

void selectContestantDialogueOption(DatingInterface_XData* d)
{
    auto& dateProps = globalState::dates[globalState::phase2.dateIdx];

    size_t dialogueOption = (d->menuSelectionIdx < 4 ? d->selectionShuffledIndices[(size_t)d->menuSelectionIdx] : (size_t)d->menuSelectionIdx);
    bool rejected = false;
    switch (dialogueOption)
    {
        case 0:
            // VERY_GOOD
            dateProps.currentTrustLevel += dateProps.veryGoodTLA;
            break;

        case 1:
            // GOOD
            dateProps.currentTrustLevel += dateProps.goodTLA;
            break;

        case 2:
            // BAD
            dateProps.currentTrustLevel += dateProps.badTLA;
            if (dateProps.currentTrustLevel <= dateProps.harshRejectThreshold)
                d->gotoRejection = true;
            break;

        case 3:
            // VERY_BAD
            dateProps.currentTrustLevel += dateProps.veryBadTLA;
            if (dateProps.currentTrustLevel <= dateProps.harshRejectThreshold)
                d->gotoRejection = true;
            break;

        case 4:
            // We're not a very good match.
            setupStage(d, DatingInterface_XData::DATING_STAGE::CONTESTANT_REJECT_DATE);
            return;

        case 5:
            // Will you go on a date with me?
            if (dateProps.currentTrustLevel < dateProps.maybeAcceptDateInviteThreshold)
                d->dateProcessingBeingAskedOut = 1;
            else if (dateProps.currentTrustLevel < dateProps.acceptDateInviteThreshold)
                d->dateProcessingBeingAskedOut = (rng::randomReal() > 0.5f ? 1 : 2);
            else
                d->dateProcessingBeingAskedOut = (rng::randomReal() > 0.9f ? 1 : 2);

            if (d->dateProcessingBeingAskedOut == 1)
                dateProps.currentTrustLevel += dateProps.veryBadTLA;  // Add VERY_BAD if date rejects the invite.
            // d->dateProcessingBeingAskedOut = 2;  @DEBUG
            setupStage(d, DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_DATE_ON_DATE);
            return;
    }

    std::cout << "NEW TRUST LEVEL: " << dateProps.currentTrustLevel << std::endl;

    // Change stage (not applicable for case 4 & 5).
    if (d->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_SELECT)
        setupStage(d, DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_EXECUTE);
    else
        setupStage(d, DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_EXECUTE);
}

void DatingInterface::update(const float_t& deltaTime)
{
    if (_data->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_SELECT ||
        _data->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_SELECT)
    {
        bool changed = false;
        int32_t direction = 0;
        if (input::onKeyUpPress)
        {
            if (_data->menuSelectionIdx > 0)
                _data->menuSelectionIdx--;
            else
                _data->menuSelectionIdx = NUM_SELECTION_BUTTONS - 1;
            changed = true;
            direction = -1;
        }
        if (input::onKeyDownPress)
        {
            if (_data->menuSelectionIdx < NUM_SELECTION_BUTTONS - 1)
                _data->menuSelectionIdx++;
            else
                _data->menuSelectionIdx = 0;
            changed = true;
            direction = 1;
        }
        setMenuSelectingCursor(_data, direction);

        if (input::onKeyJumpPress)
        {
            selectContestantDialogueOption(_data);
        }
    }

    if (_data->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ASK_THINKING ||
        _data->currentStage == DatingInterface_XData::DATING_STAGE::DATE_ANSWER_THINKING)
    {
        _data->dateThinkingTimer += deltaTime;
        float_t fillAmount = _data->dateThinkingTimer / _data->dateThinkingTimerTime;
        float_t fillAmountReal = _data->boxFillAmountMultiplier * glm_clamp_zo(fillAmount);
        vec3 position = {
            60.0f + _data->boxFillXOffset + fillAmountReal,
            373.0f,
            0.0f
        };
        vec3 scale = {
            fillAmountReal,
            25.0f,
            1.0f
        };
        glm_vec3_copy(position, _data->dateThinkingBoxFill->position);
        glm_vec3_copy(scale, _data->dateThinkingBoxFill->scale);
    }

    if (_data->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_SELECT ||
        _data->currentStage == DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_SELECT)
    {
        _data->contestantThinkingTimer += deltaTime;
        float_t fillAmount = _data->contestantThinkingTimer / _data->contestantThinkingTimerTime;
        float_t fillAmountReal = _data->boxFillAmountMultiplier * glm_clamp_zo(fillAmount);
        vec3 position = {
            310.0f + _data->boxFillXOffset + fillAmountReal,
            140.0f,
            0.0f
        };
        vec3 scale = {
            fillAmountReal,
            25.0f,
            1.0f
        };
        glm_vec3_copy(position, _data->contestantThinkingBoxFill->position);
        glm_vec3_copy(scale, _data->contestantThinkingBoxFill->scale);

        if (_data->contestantThinkingTimer > _data->contestantThinkingTimerTime)
        {
            selectContestantDialogueOption(_data);  // Select the currently selected option if answering a question.
        }
    }

    if (_data->stageTransitionTimer > 0.0f)
    {
        _data->stageTransitionTimer -= deltaTime;
        // @DEBUG: show how much time remaining on transition timer.
        // std::cout << "STAGE_TRANS_TIMER: " << _data->stageTransitionTimer << std::endl;
        if (_data->stageTransitionTimer <= 0.0f)
        {
            // Transition!
            DatingInterface_XData::DATING_STAGE nextStage = DatingInterface_XData::DATING_STAGE::LAST_STAGE;
            switch (_data->currentStage)
            {
                case DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_EXECUTE:
                case DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_DATE_ON_DATE:
                    nextStage = DatingInterface_XData::DATING_STAGE::DATE_ANSWER_THINKING;
                    break;

                case DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_EXECUTE:
                    nextStage = DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_SELECT;
                    break;

                case DatingInterface_XData::DATING_STAGE::DATE_ASK_THINKING:
                    if (_data->gotoRejection)
                    {
                        nextStage = DatingInterface_XData::DATING_STAGE::DATE_REJECT_CONTESTANT;  // @NOTE: only transition to this when date is going to do an ask, not while answering a date's question (this mirrors the talking pattern of contestant too).
                        _data->gotoRejection = false;
                    }
                    else
                        nextStage = DatingInterface_XData::DATING_STAGE::DATE_ASK_EXECUTE;
                    break;

                case DatingInterface_XData::DATING_STAGE::DATE_ASK_EXECUTE:
                    nextStage = DatingInterface_XData::DATING_STAGE::CONTESTANT_ANSWER_SELECT;
                    break;

                case DatingInterface_XData::DATING_STAGE::DATE_ANSWER_THINKING:
                    if (_data->dateProcessingBeingAskedOut == 0)
                    {
                        nextStage = DatingInterface_XData::DATING_STAGE::DATE_ANSWER_EXECUTE;
                    }
                    else
                    {
                        if (_data->dateProcessingBeingAskedOut == 1)
                            nextStage = DatingInterface_XData::DATING_STAGE::DATE_REJECT_CONTESTANT;
                        else if (_data->dateProcessingBeingAskedOut == 2)
                            nextStage = DatingInterface_XData::DATING_STAGE::DATE_ACCEPT_DATE_INVITE;
                        _data->dateProcessingBeingAskedOut = 0;  // Reset.
                    }
                    break;

                case DatingInterface_XData::DATING_STAGE::DATE_ANSWER_EXECUTE:
                    nextStage = DatingInterface_XData::DATING_STAGE::DATE_ASK_THINKING;
                    break;

                case DatingInterface_XData::DATING_STAGE::CONTESTANT_REJECT_DATE:
                case DatingInterface_XData::DATING_STAGE::DATE_REJECT_CONTESTANT:
                    globalState::phase1.transitionToPhase1FromPhase2();
                    return;
                
                case DatingInterface_XData::DATING_STAGE::DATE_ACCEPT_DATE_INVITE:
                    globalState::gotoWinGame();
                    return;
            }
            setupStage(_data, nextStage);
        }
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

void imguiUIQuad(const std::string& uiQuadName, ui::UIQuad* uiQuad)
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

    ImGui::DragFloat("boxFillXOffset", &_data->boxFillXOffset);
    ImGui::DragFloat("boxFillAmountMultiplier", &_data->boxFillAmountMultiplier);
}

void DatingInterface::activate(size_t dateIdx)
{
    _data->camera->mainCamMode.setMainCamTargetObject(_data->renderObj);

    Entity::_enablePhysicsUpdate = true;
    Entity::_enableUpdate = true;
    Entity::_enableLateUpdate = true;

    _data->dateIdx = dateIdx;
    _data->gotoRejection = false;
    _data->dateProcessingBeingAskedOut = 0;
    setupStage(_data, DatingInterface_XData::DATING_STAGE::CONTESTANT_ASK_SELECT);
}

void DatingInterface::deactivate()
{
    setupStage(_data, DatingInterface_XData::DATING_STAGE::NONE);

    Entity::_enablePhysicsUpdate = false;
    Entity::_enableUpdate = false;
    Entity::_enableLateUpdate = false;
}

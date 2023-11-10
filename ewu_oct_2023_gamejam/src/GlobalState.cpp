#include "GlobalState.h"

#include "DataSerialization.h"
#include "RandomNumberGenerator.h"
#include "Debug.h"
#include "StringHelper.h"
#include "Camera.h"
#include "CameraRail.h"
#include "EntityManager.h"
#include "RenderObject.h"
#include "PhysicsEngine.h"
#include "CoveredItem.h"
#include "Character.h"
#include "Hazard.h"
#include "DatingInterface.h"


namespace globalState
{
    // Default values
    std::string savedActiveScene       = "first.ssdat";

    vec3    savedPlayerPosition        = GLM_VEC3_ZERO_INIT;    // Currently unused. @TODO
    float_t savedPlayerFacingDirection = 0.0f;                  // Currently unused. @TODO

    int32_t savedPlayerHealth          = 100;
    int32_t savedPlayerMaxHealth       = 100;

    std::string playerGUID = "";
    vec3* playerPositionRef = nullptr;

    float_t timescale = 1.0f;

    float_t DOFFocusDepth  = 1000.0f;  // 7.5f;  // @NOTE: I think these values are a cool tilt-shift... but probably doesn't have a place for this game.
    float_t DOFFocusExtent = 1000.0f;  // 4.0f;
    float_t DOFBlurExtent  = 0.0f;  // 2.0f;

    bool       isGameActive = false;
    GamePhases currentPhase;
    float_t    playTimeRemaining;
    bool       showDebugVisuals = true;

    void resetGameState()
    {
        // Reset state.
        currentPhase = GamePhases::P0_UNCOVER;
        isGameActive = true;
        playTimeRemaining = 150.0f;  // 2 min 30 sec
        phase0 = Phase0();
        phase1 = Phase1();
        phase2 = Phase2();

        // @HARDCODE values.
        // @NOCHECKIN: @TODO: fill in the correct values when creating.
        glm_vec3_copy(vec3{ 20.0f, 0.5f, 13.5f }, phase0.spawnBoundsOrigin);
        glm_vec2_copy(vec2{ 13.5f, 16.0f }, phase0.spawnBoundsExtent);
        phase0.numCoveredItemsToSpawn = 30;  // About 10 per contestant... ish.
        // phase0.numCoveredItemsToSpawn = 3;  // @DEBUG
        glm_vec3_copy(vec3{ 243.0f, 0.5f, 20.0f }, phase1.contASpawnPosition);
        glm_vec3_copy(vec3{ 233.0f, 0.5f, 20.0f }, phase1.dateSpawnPosition);
        glm_vec3_copy(vec3{ 243.0f, -1.5f, 12.5f }, phase1.contBSpawnPosition);
        glm_vec3_copy(vec3{ -3.0f, 0.0f, 0.0f }, phase1.contBSpawnStride);
        phase1.numHazardsToSpawn = 12;
        phase1.hazardStartDistance = -42.0f;
        phase1.hazardEndDistance = -150.0f;
        phase1.finishLineDistance = -189.0f;
        phase1.contADatePhase2ActivationDistance = 1.75f;  // Both radii of the capsules (0.75) times 2 plus 0.25

        // Monster 0: "Ophelia" (ghost).
        dates[0] = {
            .contestantQuestions = {
                {
                    .question0 = { "Hey." },
                    .answer0 = { "Hey." },
                    .question1 = { "I think you just took my breath away... from running." },
                    .answer1 = { "Yeah, it looks hard to keep up." },
                    .question2 = { "I like your hair, what era?" },
                    .answer2 = { "I just did it last week, thank you for asking." },
                    .question3 = { "Whoa. What happened to your head?" },
                    .answer3 = { "My uncle cut it off when I said I wouldn't have sex with him." },
                },
                {
                    .question0 = { "I feel like I don't know much about you... can you tell me more about yourself?" },
                    .answer0 = { "Hmm. There's not much to me, just someone who makes video games. It was a little hard to get the hand-eye coordination down." },
                    .question1 = { "On a scale of 1 - 10, how insane do you feel? I'm about an 8." },
                    .answer1 = { "Honestly, for agreeing to participate, a full Hamlet 10." },
                    .question2 = { "So do you know what a meme is?" },
                    .answer2 = { "Who even asks that?" },
                    .question3 = { "Do you like alone?" },
                    .answer3 = { "Why do you wanna know? Creep." },
                },
                {
                    // Final "question" but it's everything being blocked out and the only option is to select "we're not a good fit" or "go out with me?"
                    .question0 = { "", true },
                    .answer0 = { "", true },
                    .question1 = { "", true },
                    .answer1 = { "", true },
                    .question2 = { "", true },
                    .answer2 = { "", true },
                    .question3 = { "", true },
                    .answer3 = { "", true },
                },
            },
            .dateQuestions = {
                {
                    .question = { "How do you feel about incest?" },
                    .veryGoodAnswer = { "Whoa, what the fuck? This is not Alabama. Did that happen to you? Are you okay?" },
                    .goodAnswer = { "I've heard of people doing it. Weirds me out though." },
                    .badAnswer = { "What kind of incest? Oedipus?" },
                    .veryBadAnswer = { "It's a guilty pleasure of mine. 2TB worth." },
                },
                {
                    // Final "question" that's just like "enjoying the mixer?"
                    .question = { "Umm, enjoying the mixer?" },
                    .veryGoodAnswer = { "Oh heck yeah!" },
                    .goodAnswer = { "It's pretty cool." },
                    .badAnswer = { "It's awesome. It's like shooting fish in a barrel." },
                    .veryBadAnswer = { "As amazing as my fantasies with you." },
                    .isLastQuestion = true,
                },
            },
            .veryGoodTLA = 40.0f,
            .goodTLA = 20.0f,
            .badTLA = -20.0f,
            .veryBadTLA = -40.0f,
            .harshRejectThreshold = 40.0f,
            .maybeAcceptDateInviteThreshold = 50.0f,
            .acceptDateInviteThreshold = 90.0f,
            .acceptDateInviteWaitTimeRange = { 4.0f, 8.0f },
        };

        // Monster 1: "Nefertiti" (mummy).
        dates[1] = {
            .contestantQuestions = {
                {
                    .question0 = { "Your wraps look supa high quality?" },
                    .answer0 = { "Thanks they're Levie Vitt." },
                    .question1 = { "Your hands are so soft for a mummy." },
                    .answer1 = { "Maybe you should use some... you could use it for $15." },
                    .question2 = { "Areola sticking out." },
                    .answer2 = { "Pay me 20 and I'll put it away." },
                    .question3 = { "Are those natural?" },
                    .answer3 = { "No." },
                },
                {
                    .question0 = { "If we went on a vacation for our date, where would you wanna go?" },
                    .answer0 = { "You have the resources to take me? ... Greece." },
                    .question1 = { "Nefertiti, what does your name mean?" },
                    .answer1 = { "No one's asked me that before, it means \"The beautiful one has come\" ;)" },
                    .question2 = { "What kind of food would you want on our first date?" },
                    .answer2 = { "Hun, if you're gonna ask a girl like me out, you should already have everything planned out." },
                    .question3 = { "Would you wanna go back to my place and get... unwrapped?" },
                    .answer3 = { "Oh... sweetie no..." },
                },
                {
                    // Final "question" but it's everything being blocked out and the only option is to select "we're not a good fit" or "go out with me?"
                    .question0 = { "", true },
                    .answer0 = { "", true },
                    .question1 = { "", true },
                    .answer1 = { "", true },
                    .question2 = { "", true },
                    .answer2 = { "", true },
                    .question3 = { "", true },
                    .answer3 = { "", true },
                },
            },
            .dateQuestions = {
                {
                    .question = { "So... what kinda body mod is that?" },
                    .veryGoodAnswer = { "It was the best preservation money can buy." },
                    .goodAnswer = { "I had an accident at work." },
                    .badAnswer = { "Uh... I was just born this way." },
                    .veryBadAnswer = { "..." },
                },
                {
                    // Final "question" that's just like "enjoying the mixer?"
                    .question = { "Umm, enjoying the mixer?" },
                    .veryGoodAnswer = { "Oh heck yeah!" },
                    .goodAnswer = { "It's pretty cool." },
                    .badAnswer = { "It's awesome. It's like shooting fish in a barrel." },
                    .veryBadAnswer = { "As amazing as my fantasies with you." },
                    .isLastQuestion = true,
                },
            },
            .veryGoodTLA = 40.0f,
            .goodTLA = 20.0f,
            .badTLA = -20.0f,
            .veryBadTLA = -40.0f,
            .harshRejectThreshold = 40.0f,
            .maybeAcceptDateInviteThreshold = 50.0f,
            .acceptDateInviteThreshold = 90.0f,
            .acceptDateInviteWaitTimeRange = { 4.0f, 8.0f },
        };

        // Monster 2: "Isabella" (bat).
        dates[2] = {
            .contestantQuestions = {
                {
                    .question0 = { "Your beauty entranced me, and I just knew I had to get to know you." },
                    .answer0 = { "You might not like me, after getting to know me though." },
                    .question1 = { "What kind of qualities do you look--wait sorry--smell for in a man?" },
                    .answer1 = { "Someone that respects me and doesn't look down on me for being blind... and really old." },
                    .question2 = { "If you're blind, that means I have a chance, right?" },
                    .answer2 = { "I can still smell how ugly you are." },
                    .question3 = { "Have you tried glasses?" },
                    .answer3 = { "Are you actually mentally disabled." },
                },
                {
                    .question0 = { "My family specializes in Human Trafficking, want me to hook you with some blood?" },
                    .answer0 = { "That's the most romantic thing anyone has ever said to me." },
                    .question1 = { "What do I smell like to you?" },
                    .answer1 = { "Like shit." },
                    .question2 = { "What was your favorite time period?" },
                    .answer2 = { "Definitely the Dark Ages... I ate good those years." },
                    .question3 = { "Would you like to slurp me up?" },
                    .answer3 = { "I can already tell your blood is riddled with diabetes. No thanks." },
                },
                {
                    // Final "question" but it's everything being blocked out and the only option is to select "we're not a good fit" or "go out with me?"
                    .question0 = { "", true },
                    .answer0 = { "", true },
                    .question1 = { "", true },
                    .answer1 = { "", true },
                    .question2 = { "", true },
                    .answer2 = { "", true },
                    .question3 = { "", true },
                    .answer3 = { "", true },
                },
            },
            .dateQuestions = {
                {
                    .question = { "What brought you to sign up for this 'killer mixer'?" },
                    .veryGoodAnswer = { "If I tell you I just woke up here, would you believe me?" },
                    .goodAnswer = { "I'm not into vore, and everybody else tries to eat me." },
                    .badAnswer = { "I'm really ugly." },
                    .veryBadAnswer = { "Everyone looked hot." },
                },
                {
                    // Final "question" that's just like "enjoying the mixer?"
                    .question = { "Umm, enjoying the mixer?" },
                    .veryGoodAnswer = { "Oh heck yeah!" },
                    .goodAnswer = { "It's pretty cool." },
                    .badAnswer = { "It's awesome. It's like shooting fish in a barrel." },
                    .veryBadAnswer = { "As amazing as my fantasies with you." },
                    .isLastQuestion = true,
                },
            },
            .veryGoodTLA = 40.0f,
            .goodTLA = 20.0f,
            .badTLA = -20.0f,
            .veryBadTLA = -40.0f,
            .harshRejectThreshold = 40.0f,
            .maybeAcceptDateInviteThreshold = 50.0f,
            .acceptDateInviteThreshold = 90.0f,
            .acceptDateInviteWaitTimeRange = { 4.0f, 8.0f },
        };
    }

    void startNewGame()
    {
        // Trigger loading phase 0.
        phase0.transitionToPhase0(false);
        // phase1.loadTriggerFlag = true;  // @DEBUG
        // phase2.loadTriggerFlag = true;  // @DEBUG
    }

    void gotoWinGame()
    {
        std::cout << "YOU WON THE GAME CONGRATULATIONS!!!!" << std::endl;
    }

    void Phase0::setDateDummyPosition(size_t dateIdx, vec3 position)
    {
        dateDummyCharacter[dateIdx]->moreOrLessSpawnAtPosition(position);
    }

    void Phase0::uncoverDateDummy(size_t dateIdx)
    {
        dateDummyCharacter[dateIdx]->setRenderLayer(RenderLayer::VISIBLE);
    }

    void Phase0::transitionToPhase0(bool useTransitionTimer)
    {
        if (useTransitionTimer)
            phase0.transitionTimer = 1.0f;
        phase0.loadTriggerFlag = true;
    }

    Phase0 phase0 = Phase0();

    void Phase1::transitionToPhase1(size_t dateId, size_t contestantId)
    {
        clearContestants();
        setDateIndex(dateId);
        registerContestantA(phase0.contestants[contestantId]);
        for (size_t i = 0; i < NUM_CONTESTANTS; i++)
        {
            if (i == contestantId)
                continue;
            registerContestantB(phase0.contestants[i]);
        }
        phase1.transitionTimer = 1.0f;
        phase1.loadTriggerFlag = true;
    }

    void Phase1::transitionToPhase1FromPhase2()
    {
        phase1.transitionTimer = 1.0f;
        phase1.returnFromPhase2TriggerFlag = true;
    }

    Phase1 phase1 = Phase1();

    void Phase2::transitionToPhase2(size_t dateId)
    {
        Phase2::dateIdx = dateId;
        transitionTimer = 1.0f;
        loadTriggerFlag = true;
    }

    Phase2 phase2 = Phase2();
    DateProps dates[NUM_CONTESTANTS];

    SceneCamera* sceneCameraRef = nullptr;
    EntityManager* entityManagerRef = nullptr;

    // Harvestable items (e.g. materials, raw ores, etc.)
    std::vector<HarvestableItemOption> allHarvestableItems = {
        HarvestableItemOption{.name = "sheet metal", .modelName = "Box" },
        HarvestableItemOption{.name = "TEST slime", .modelName = "Box" },
    };

    std::vector<uint16_t> harvestableItemQuantities;  // This is the inventory data for collectable/ephemeral items.

    // Scannable items
    std::vector<ScannableItemOption> allAncientWeaponItems = {
        ScannableItemOption{
            .name = "Wing Blade",
            .modelName = "WingWeapon",
            .type = WEAPON,
            .requiredMaterialsToMaterialize = {
                { .harvestableItemId = 0, .quantity = 1 }
            },
            .weaponStats = {
                .weaponType = "twohanded",
                .durability = 32,
                .attackPower = 2,
                .attackPowerWhenDulled = 1,
            },
        },
        ScannableItemOption{
            .name = "TEST Slime girl",
            .modelName = "SlimeGirl",
            .type = FOOD,
            .requiredMaterialsToMaterialize = {
                { .harvestableItemId = 1, .quantity = 2 },
            },
        },
    };

    std::vector<bool> scannableItemCanMaterializeFlags;  // This is the list of materializable items.  @FUTURE: make this into a more sophisticated data structure for doing the "memory" system of aligning the data and overwriting previously written data.
    size_t selectedScannableItemId = 0;                  // This is the item selected to be materialized if LMB is pressed.


    //
    // Global state writing brain
    //
    const std::string gsFname = "global_state.hgs";
    tf::Executor tfExecutor(1);
    tf::Taskflow tfTaskAsyncWriting;

    void loadGlobalState()
    {
        // @TODO: for now it's just the dataserialization dump. I feel like getting the data into unsigned chars would be best though  -Timo
        std::ifstream infile(gsFname);
        if (!infile.is_open())
        {
            debug::pushDebugMessage({
                .message = "Could not open file \"" + gsFname + "\" for reading global state (using default values)",
                .type = 1,
            });
            return;
        }

        DataSerializer ds;
        std::string line;
        while (std::getline(infile, line))
        {
            trim(line);
            if (line.empty())
                continue;
            ds.dumpString(line);
        }

        DataSerialized dsd = ds.getSerializedData();
        dsd.loadString(savedActiveScene);
        dsd.loadVec3(sceneCameraRef->gpuCameraData.cameraPosition);
        dsd.loadVec3(sceneCameraRef->facingDirection);
        dsd.loadFloat(sceneCameraRef->fov);
        dsd.loadVec3(savedPlayerPosition);
        dsd.loadFloat(savedPlayerFacingDirection);

        float_t lf1, lf2;
        dsd.loadFloat(lf1);
        dsd.loadFloat(lf2);
        savedPlayerHealth = (int32_t)lf1;
        savedPlayerMaxHealth = (int32_t)lf2;

        debug::pushDebugMessage({
            .message = "Successfully read state from \"" + gsFname + "\"",
        });
    }

    void saveGlobalState()
    {
        // @TODO: for now it's just the dataserialization dump. I feel like getting the data into unsigned chars would be best though  -Timo
        std::ofstream outfile(gsFname);
        if (!outfile.is_open())
        {
            debug::pushDebugMessage({
                .message = "Could not open file \"" + gsFname + "\" for writing global state",
                .type = 2,
            });
            return;
        }

        DataSerializer ds;
        ds.dumpString(savedActiveScene);
        ds.dumpVec3(sceneCameraRef->gpuCameraData.cameraPosition);
        ds.dumpVec3(sceneCameraRef->facingDirection);
        ds.dumpFloat(sceneCameraRef->fov);
        ds.dumpVec3(savedPlayerPosition);
        ds.dumpFloat(savedPlayerFacingDirection);
        ds.dumpFloat(savedPlayerHealth);
        ds.dumpFloat(savedPlayerMaxHealth);

        DataSerialized dsd = ds.getSerializedData();
        size_t count = dsd.getSerializedValuesCount();
        for (size_t i = 0; i < count; i++)
        {
            std::string s;
            dsd.loadString(s);
            outfile << s << '\n';
        }

        debug::pushDebugMessage({
            .message = "Successfully wrote state to \"" + gsFname + "\"",
        });
    }

    void initGlobalState(SceneCamera& sc, EntityManager* em)
    {
        sceneCameraRef = &sc;
        entityManagerRef = em;

        // Initial values for inventory and list of materializable items.
        harvestableItemQuantities.resize(allHarvestableItems.size(), 0);
        scannableItemCanMaterializeFlags.resize(allAncientWeaponItems.size(), false);

        loadGlobalState();

        tfTaskAsyncWriting.emplace([&]() {
            saveGlobalState();
        });
    }

    void launchAsyncWriteTask()
    {
        tfExecutor.wait_for_all();
        tfExecutor.run(tfTaskAsyncWriting);
    }

    inline void generateSpawnLocation(vec3& outSpawnLocation)
    {
        vec3 spawnLocation = {
            rng::randomRealRange(-phase0.spawnBoundsExtent[0], phase0.spawnBoundsExtent[0]),
            5.0f,  // @HACK: to prevent spawned objects from falling beneath the floor.
            rng::randomRealRange(-phase0.spawnBoundsExtent[1], phase0.spawnBoundsExtent[1])
        };
        glm_vec3_add(spawnLocation, phase0.spawnBoundsOrigin, spawnLocation);
        glm_vec3_copy(spawnLocation, outSpawnLocation);
    }

    void update(float_t deltaTime, bool& requestSnapshotBlit, bool& skyboxIsSnapshotImage)
    {
        if (showCountdown() && !phase1.loadTriggerFlag)
        {
            playTimeRemaining -= deltaTime;
        }

        if (currentPhase == GamePhases::P1_HALLWAY && !phase2.loadTriggerFlag)
        {
            vec3 a, b;
            phase1.contACharacter->getPosition(a);
            phase1.dateCharacter->getPosition(b);
            a[1] = b[1] = 0.0f;  // Use flat pos.
            if (!phase1.contACharacter->isStunned() &&
                !phase1.dateCharacter->isStunned() &&
                glm_vec3_distance2(a, b) < phase1.contADatePhase2ActivationDistance * phase1.contADatePhase2ActivationDistance)
                phase2.transitionToPhase2(phase1.dateIdx);
        }

        if (phase0.loadTriggerFlag && (phase0.transitionTimer -= deltaTime) < 0.0f)
        {
            // Load phase 0.
            // Move contestants to somewhere within play bounds.
            for (size_t i = 0; i < NUM_CONTESTANTS; i++)
            {
                vec3 spawnLocation;
                generateSpawnLocation(spawnLocation);
                phase0.contestants[i]->setContestantIndex(i);
                phase0.contestants[i]->moreOrLessSpawnAtPosition(spawnLocation);
                if (i == phase0.playerContestantIdx)
                    phase0.contestants[i]->setAsCameraTargetObject();
            }

            // Delete all covered items.
            for (auto coveredItem : phase0.spawnedCoveredItems)
                entityManagerRef->destroyEntity(coveredItem);
            phase0.spawnedCoveredItems.clear();

            // Spawn `numCoveredItemsToSpawn` # of covered items.
            for (size_t i = 0; i < phase0.numCoveredItemsToSpawn; i++)
            {
                vec3 spawnLocation;
                generateSpawnLocation(spawnLocation);
                float_t yRotation = rng::randomRealRange(0.0f, M_PI * 2.0f);
                size_t itemType = rng::randomIntegerRange(0, CoveredItem::numItemTypes() - 1);

                DataSerializer ds;
                ds.dumpString("0");  // GUID
                ds.dumpVec3(spawnLocation);
                ds.dumpFloat(yRotation);
                ds.dumpFloat((float_t)itemType);
                DataSerialized dsd = ds.getSerializedData();

                phase0.spawnedCoveredItems.push_back(
                    static_cast<CoveredItem*>(scene::spinupNewObject(":covereditem", &dsd))
                );
            }

            // Assign `NUM_CONTESTANTS` # of covered items to have a date id.
            std::vector<size_t> reservedIndices;
            for (size_t i = 0; i < NUM_CONTESTANTS; i++)
            {
                size_t selectedIdx;
                bool collided;
                do
                {
                    collided = false;
                    selectedIdx = rng::randomIntegerRange(0, phase0.spawnedCoveredItems.size() - 1);  // @NOCHECKIN: see if integer range is inclusive or exclusive.
                    for (size_t ri : reservedIndices)
                        if (ri == selectedIdx)
                        {
                            collided = true;
                            break;
                        }
                } while (collided);
                reservedIndices.push_back(selectedIdx);
            }
            size_t dateId = 0;
            for (size_t ri : reservedIndices)
                phase0.spawnedCoveredItems[ri]->setDateId(dateId++);

            // Hide Date dummy render objects into INVISIBLE layer.
            for (size_t i = 0; i < NUM_CONTESTANTS; i++)
            {
                vec3 position;
                phase0.spawnedCoveredItems[reservedIndices[i]]->getPosition(position);
                phase0.dateDummyCharacter[i]->moreOrLessSpawnAtPosition(position);
                phase0.dateDummyCharacter[i]->setDateDummyIndex(i);
                phase0.dateDummyCharacter[i]->setRenderLayer(RenderLayer::INVISIBLE);
            }

            // Finished.
            currentPhase = GamePhases::P0_UNCOVER;
            phase0.loadTriggerFlag = false;
        }

        if (phase1.loadTriggerFlag && (phase1.transitionTimer -= deltaTime) < 0.0f)
        {
            // @DEBUG inject certain characters as the ones for phase 1.
            // phase1.clearContestants();
            // phase1.registerContestantA(phase0.contestants[0]);
            // phase1.registerContestantB(phase0.contestants[1]);
            // phase1.registerContestantB(phase0.contestants[2]);
            // phase1.setDateIndex(0);

            // Load phase 1.
            // Delete all covered items from phase 0.
            for (auto coveredItem : phase0.spawnedCoveredItems)
                entityManagerRef->destroyEntity(coveredItem);
            phase0.spawnedCoveredItems.clear();

            // Move contestant A and Date to hallway.
            phase1.contACharacter->moreOrLessSpawnAtPosition(phase1.contASpawnPosition);
            phase1.dateCharacter->moreOrLessSpawnAtPosition(phase1.dateSpawnPosition);

            // Move contestant Bs to trap controls.
            vec3 spawnPositionB;
            glm_vec3_copy(phase1.contBSpawnPosition, spawnPositionB);
            for (size_t i = 0; i < NUM_CONTESTANTS - 1; i++)
            {
                if (i > 0)
                    glm_vec3_add(spawnPositionB, phase1.contBSpawnStride, spawnPositionB);
                phase1.contBCharacters[i]->moreOrLessSpawnAtPosition(spawnPositionB);
            }

            // Activate player camera (for snapping camera to player view in hallway).
            phase0.contestants[phase0.playerContestantIdx]->setAsCameraTargetObject();

            // Reset hazards.
            for (auto hazard : phase1.hazards)
                entityManagerRef->destroyEntity(hazard);
            phase1.hazards.clear();

            std::vector<float_t> spawnLocationX;
            for (size_t i = 0; i < phase1.numHazardsToSpawn; i++)
                spawnLocationX.push_back(rng::randomRealRange(phase1.hazardStartDistance, phase1.hazardEndDistance));
            std::sort(spawnLocationX.rbegin(), spawnLocationX.rend());

            for (size_t i = 0; i < phase1.numHazardsToSpawn; i++)
            {
                vec3 spawnLocation;
                glm_vec3_copy(phase1.contASpawnPosition, spawnLocation);
                spawnLocation[0] += spawnLocationX[i];
                int8_t hazardType = rng::randomIntegerRange(0, 0);

                DataSerializer ds;
                ds.dumpString("0");  // GUID
                ds.dumpVec3(spawnLocation);
                ds.dumpFloat(hazardType);
                DataSerialized dsd = ds.getSerializedData();

                static_cast<Hazard*>(scene::spinupNewObject(":hazard", &dsd));
            }

            // Activate Date.
            phase1.dateCharacter->activateDate(phase1.dateIdx);  // @NOCHECKIN: is there even anything that this needs to do?
                                                   // @REPLY: I guess what this would do is (CONTEXT: when the date moves to the end of the hallway, it deactivates itself) tell the date to activate and start moving down the hall. There should really only be a `moveDownHallway = true` that's needed.

            // Finished.
            currentPhase = GamePhases::P1_HALLWAY;
            phase1.loadTriggerFlag = false;
        }

        if (phase1.returnFromPhase2TriggerFlag && (phase1.transitionTimer -= deltaTime) < 0.0f)
        {
            // Simple return to phase 1.
            skyboxIsSnapshotImage = false;
            phase2.datingInterface->deactivate();
            phase0.contestants[phase0.playerContestantIdx]->setAsCameraTargetObject();
            phase1.contACharacter->stun(1.5f);  // @HARDCODE.
            currentPhase = GamePhases::P1_HALLWAY;
            phase1.returnFromPhase2TriggerFlag = false;
        }

        if (phase2.loadTriggerFlag)
        {
            bool transitionFinished = ((phase2.transitionTimer -= deltaTime) < 0.0f);
            requestSnapshotBlit = !transitionFinished;  // Take snapshots while transition is happening, but not when scene switches.
            if (transitionFinished)
            {
                // Load phase 2.
                // Activate dating interface.
                skyboxIsSnapshotImage = true;
                phase2.datingInterface->activate(phase2.dateIdx);

                // Finished.
                currentPhase = GamePhases::P2_OTOMEGE;
                phase2.loadTriggerFlag = false;
            }
        }
    }

    void draw3dCrosshair(vec3 position, physengine::DebugVisLineType lineType, float_t extent = 0.5f)
    {
        vec3 p0, p1, p2, p3, p4, p5;
        glm_vec3_add(position, vec3{ -extent, 0.0f, 0.0f }, p0);
        glm_vec3_add(position, vec3{  extent, 0.0f, 0.0f }, p1);
        glm_vec3_add(position, vec3{ 0.0f, -extent, 0.0f }, p2);
        glm_vec3_add(position, vec3{ 0.0f,  extent, 0.0f }, p3);
        glm_vec3_add(position, vec3{ 0.0f, 0.0f, -extent }, p4);
        glm_vec3_add(position, vec3{ 0.0f, 0.0f,  extent }, p5);
        physengine::drawDebugVisLine(p0, p1, lineType);
        physengine::drawDebugVisLine(p2, p3, lineType);
        physengine::drawDebugVisLine(p4, p5, lineType);
    }

    void drawDebugVisualization()
    {
        if (!showDebugVisuals)
            return;
        
        // Draw phase0 spawn bounds.
        {
            vec3 pt0, pt1, pt2, pt3;
            glm_vec3_copy(phase0.spawnBoundsOrigin, pt0);
            glm_vec3_copy(phase0.spawnBoundsOrigin, pt1);
            glm_vec3_copy(phase0.spawnBoundsOrigin, pt2);
            glm_vec3_copy(phase0.spawnBoundsOrigin, pt3);
            pt0[0] += phase0.spawnBoundsExtent[0];
            pt1[0] -= phase0.spawnBoundsExtent[0];
            pt2[0] -= phase0.spawnBoundsExtent[0];
            pt3[0] += phase0.spawnBoundsExtent[0];
            pt0[2] += phase0.spawnBoundsExtent[1];
            pt1[2] += phase0.spawnBoundsExtent[1];
            pt2[2] -= phase0.spawnBoundsExtent[1];
            pt3[2] -= phase0.spawnBoundsExtent[1];
            physengine::drawDebugVisLine(pt0, pt1);
            physengine::drawDebugVisLine(pt1, pt2);
            physengine::drawDebugVisLine(pt2, pt3);
            physengine::drawDebugVisLine(pt3, pt0);
        }

        // Draw phase1 spawn positions and start and finish lines.
        {
            draw3dCrosshair(phase1.contASpawnPosition, physengine::DebugVisLineType::PURPTEAL);
            draw3dCrosshair(phase1.dateSpawnPosition, physengine::DebugVisLineType::VELOCITY);
            vec3 contBPos;
            for (size_t i = 0; i < NUM_CONTESTANTS - 1; i++)
            {
                if (i == 0)
                    glm_vec3_copy(phase1.contBSpawnPosition, contBPos);
                else
                    glm_vec3_add(contBPos, phase1.contBSpawnStride, contBPos);
                draw3dCrosshair(contBPos, physengine::DebugVisLineType::KIKKOARMY);
            }

            constexpr float_t lineExtent = 5.0f;
            vec3 hazardStartLineP1, hazardStartLineP2;
            glm_vec3_add(phase1.contASpawnPosition, vec3{ phase1.hazardStartDistance, 0.0f, -lineExtent }, hazardStartLineP1);
            glm_vec3_add(phase1.contASpawnPosition, vec3{ phase1.hazardStartDistance, 0.0f,  lineExtent }, hazardStartLineP2);
            physengine::drawDebugVisLine(hazardStartLineP1, hazardStartLineP2, physengine::DebugVisLineType::PURPTEAL);

            vec3 hazardEndLineP1, hazardEndLineP2;
            glm_vec3_add(phase1.contASpawnPosition, vec3{ phase1.hazardEndDistance, 0.0f, -lineExtent }, hazardEndLineP1);
            glm_vec3_add(phase1.contASpawnPosition, vec3{ phase1.hazardEndDistance, 0.0f,  lineExtent }, hazardEndLineP2);
            physengine::drawDebugVisLine(hazardEndLineP1, hazardEndLineP2, physengine::DebugVisLineType::PURPTEAL);

            vec3 finishLineP1, finishLineP2;
            glm_vec3_add(phase1.contASpawnPosition, vec3{ phase1.finishLineDistance, 0.0f, -lineExtent }, finishLineP1);
            glm_vec3_add(phase1.contASpawnPosition, vec3{ phase1.finishLineDistance, 0.0f,  lineExtent }, finishLineP2);
            physengine::drawDebugVisLine(finishLineP1, finishLineP2, physengine::DebugVisLineType::YUUJUUFUDAN);
        }
    }

    void cleanupGlobalState()
    {
        launchAsyncWriteTask();  // Run the task one last time before cleanup

        // Lol, no cleanup. Thanks Dmitri!
    }

    HarvestableItemOption* getHarvestableItemByIndex(size_t index)
    {
        return &allHarvestableItems[index];
    }

    uint16_t getInventoryQtyOfHarvestableItemByIndex(size_t harvestableItemId)
    {
        return harvestableItemQuantities[harvestableItemId];
    }

    void changeInventoryItemQtyByIndex(size_t harvestableItemId, int16_t changeInQty)
    {
        // Clamp all item quantities in the range [0-999]
        harvestableItemQuantities[harvestableItemId] = (uint16_t)std::max(0, std::min(999, (int32_t)harvestableItemQuantities[harvestableItemId] + changeInQty));
    }

    size_t getNumHarvestableItemIds()
    {
        return allHarvestableItems.size();
    }
    
    std::string ancientWeaponItemTypeToString(AncientWeaponItemType awit)
    {
        switch (awit)
        {
            case WEAPON: return "weapon";
            case FOOD:   return "food";
            case TOOL:   return "tool";
            default:     return "NO ITEM TYPE TO STRING CONVERSTION AVAILABLE";
        }
    }

    ScannableItemOption* getAncientWeaponItemByIndex(size_t index)
    {
        return &allAncientWeaponItems[index];
    }

    bool getCanMaterializeScannableItemByIndex(size_t scannableItemId)
    {
        return scannableItemCanMaterializeFlags[scannableItemId];
    }

    void flagScannableItemAsCanMaterializeByIndex(size_t scannableItemId, bool flag)
    {
        scannableItemCanMaterializeFlags[scannableItemId] = flag;
    }

    size_t getNumScannableItemIds()
    {
        return allAncientWeaponItems.size();
    }

    size_t getSelectedScannableItemId()
    {
        return selectedScannableItemId;
    }

    void setSelectedScannableItemId(size_t scannableItemId)
    {
        selectedScannableItemId = scannableItemId;
    }

    bool selectNextCanMaterializeScannableItemId()
    {
        size_t originalId = selectedScannableItemId;
        do
        {
            selectedScannableItemId = (selectedScannableItemId + 1) % allAncientWeaponItems.size();
            if (selectedScannableItemId == originalId)
                return false;  // Cycled thru all of them and couldn't find another one that was materializable. So, just keep the original id.
        } while (!getCanMaterializeScannableItemByIndex(selectedScannableItemId));

        return true;  // New id was found and got selected.
    }

    // Camera Rail manager.
    std::vector<CameraRail*> cameraRails;
    void addCameraRail(CameraRail* cameraRail)
    {
        if (std::find(cameraRails.begin(), cameraRails.end(), cameraRail) == cameraRails.end())
            cameraRails.push_back(cameraRail);
    }

    void removeCameraRail(CameraRail* cameraRail)
    {
        cameraRails.erase(
            std::remove(
                cameraRails.begin(),
                cameraRails.end(),
                cameraRail
            ),
            cameraRails.end()
        );
    }

    CameraRail* getNearestCameraRailToDesiredRailDirection(vec3 queryPos, vec3 queryDir)
    {
        float_t closestDistance = std::numeric_limits<float_t>::max();
        size_t closestIdx = (size_t)-1;
        for (size_t i = 0; i < cameraRails.size(); i++)
        {
            float_t currentDistance;
            if ((currentDistance = cameraRails[i]->findDistance2FromRailIfInlineEnough(queryPos, queryDir)) < closestDistance)
            {
                closestDistance = currentDistance;
                closestIdx = i;
            }
        }

        if (closestIdx == (size_t)-1)
            return nullptr;
        return cameraRails[closestIdx];
    }

    bool showCountdown()
    {
        return isGameActive && currentPhase == GamePhases::P0_UNCOVER;
    }

    bool charactersHaveInput()
    {
        return
            isGameActive &&
            (currentPhase == GamePhases::P0_UNCOVER ||
                currentPhase == GamePhases::P1_HALLWAY) &&
            !phase0.loadTriggerFlag &&
            !phase1.loadTriggerFlag &&
            !phase2.loadTriggerFlag &&
            !gameIsOver();
    }

    bool gameIsOver()
    {
        return playTimeRemaining <= 0.0f;
    }
}

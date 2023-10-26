#include "GlobalState.h"

#include "DataSerialization.h"
#include "RandomNumberGenerator.h"
#include "Debug.h"
#include "StringHelper.h"
#include "Camera.h"
#include "CameraRail.h"
#include "EntityManager.h"
#include "RenderObject.h"
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
        glm_vec3_copy(vec3{ 10.0f, 10.0f, 10.0f }, phase0.spawnBoundsOrigin);
        glm_vec2_copy(vec2{ 10.0f, 10.0f }, phase0.spawnBoundsExtent);
        phase0.numCoveredItemsToSpawn = 50;
        glm_vec3_copy(vec3{ 10.0f, 10.0f, 10.0f }, phase1.contASpawnPosition);
        glm_vec3_copy(vec3{ 10.0f, 10.0f, 10.0f }, phase1.dateSpawnPosition);
        glm_vec3_copy(vec3{ 10.0f, 10.0f, 10.0f }, phase1.contBSpawnPosition);
        glm_vec3_copy(vec3{ 0.0f, 0.0f, 2.0f }, phase1.contBSpawnStride);
        dates[0] = {
            .trustLevelIncrement = 1.0f,
            .trustLevelDecrement = 1.0f,
            .harshRejectThreshold = 20.0f,
            .maybeAcceptDateInviteThreshold = 80.0f,
            .acceptDateInviteWaitTimeRange = { 2.0f, 6.0f },
        };
        dates[1] = {
            .trustLevelIncrement = 1.0f,
            .trustLevelDecrement = 1.0f,
            .harshRejectThreshold = 20.0f,
            .maybeAcceptDateInviteThreshold = 80.0f,
            .acceptDateInviteWaitTimeRange = { 2.0f, 6.0f },
        };
        dates[2] = {
            .trustLevelIncrement = 1.0f,
            .trustLevelDecrement = 1.0f,
            .harshRejectThreshold = 20.0f,
            .maybeAcceptDateInviteThreshold = 80.0f,
            .acceptDateInviteWaitTimeRange = { 2.0f, 6.0f },
        };
    }

    void startNewGame()
    {
        // Trigger loading phase 0.
        phase0.loadTriggerFlag = true;
        // phase1.loadTriggerFlag = true;  // @DEBUG
        // phase2.loadTriggerFlag = true;  // @DEBUG
    }

    Phase0 phase0 = Phase0();
    Phase1 phase1 = Phase1();
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
            0.0f,
            rng::randomRealRange(-phase0.spawnBoundsExtent[1], phase0.spawnBoundsExtent[1])
        };
        glm_vec3_add(spawnLocation, phase0.spawnBoundsOrigin, spawnLocation);
        glm_vec3_copy(spawnLocation, outSpawnLocation);
    }

    void update(float_t deltaTime)
    {
        if (showCountdown())
        {
            playTimeRemaining -= deltaTime;
        }

        if (phase0.loadTriggerFlag)
        {
            // Load phase 0.
            // Move contestants to somewhere within play bounds.
            for (size_t i = 0; i < NUM_CONTESTANTS; i++)
            {
                vec3 spawnLocation;
                generateSpawnLocation(spawnLocation);
                phase0.contestants[i]->moreOrLessSpawnAtPosition(spawnLocation);
            }

            // Delete all covered items.
            for (auto& coveredItem : phase0.spawnedCoveredItems)
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
                    static_cast<CoveredItem*>(scene::spinupNewObject("covereditem", &dsd))
                );
            }

            // Assign `NUM_CONTESTANTS` # of covered items to have a date id.
            std::vector<size_t> reservedIndices;
            for (size_t i = 0; i < NUM_CONTESTANTS; i++)
            {
                size_t selectedIdx;
                bool collided = false;
                do
                {
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
                phase0.spawnedCoveredItems[i]->getPosition(position);
                phase0.dateDummyCharacter[i]->moreOrLessSpawnAtPosition(position);
                phase0.dateDummyCharacter[i]->setRenderLayer(RenderLayer::INVISIBLE);
            }

            // Finished.
            phase0.loadTriggerFlag = false;
        }

        if (phase1.loadTriggerFlag)
        {
            // @DEBUG inject certain characters as the ones for phase 1.
            // phase1.clearContestants();
            // phase1.registerContestantA(phase0.contestants[0]);
            // phase1.registerContestantB(phase0.contestants[1]);
            // phase1.registerContestantB(phase0.contestants[2]);
            // phase1.registerDate(0);

            // Load phase 1.
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

            // Reset hazards.
            for (auto& hazard : phase1.hazards)
                hazard->reset();

            // Activate Date.
            phase1.dateCharacter->activateDate(phase1.dateIdx);  // @NOCHECKIN: is there even anything that this needs to do?
                                                   // @REPLY: I guess what this would do is (CONTEXT: when the date moves to the end of the hallway, it deactivates itself) tell the date to activate and start moving down the hall. There should really only be a `moveDownHallway = true` that's needed.

            // Finished.
            phase0.loadTriggerFlag = false;
        }

        if (phase2.loadTriggerFlag)
        {
            // Load phase 2.
            // Activate dating interface.
            phase2.datingInterface->activate(phase2.dateIdx);

            // Finished.
            phase2.loadTriggerFlag = false;
        }

        if (phase2.unloadTriggerFlag)
        {
            // Unload phase 2.
            // Deactivate dating interface.
            phase2.datingInterface->deactivate();

            // Finished.
            phase2.unloadTriggerFlag = false;
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

    bool gameIsOver()
    {
        return playTimeRemaining <= 0.0f;
    }
}

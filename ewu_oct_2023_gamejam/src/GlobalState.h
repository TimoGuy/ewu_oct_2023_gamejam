#pragma once

#include "Imports.h"
struct SceneCamera;
class EntityManager;
class CameraRail;
class Character;
class CoveredItem;
class Hazard;
class DatingInterface;


namespace globalState
{
    //
    // Saved
    //
    extern std::string savedActiveScene;

    extern vec3    savedPlayerPosition;
    extern float_t savedPlayerFacingDirection;

    extern int32_t savedPlayerHealth;
    extern int32_t savedPlayerMaxHealth;

    extern std::string playerGUID;
    extern vec3* playerPositionRef;

    extern float_t timescale;

    extern float_t DOFFocusDepth;
    extern float_t DOFFocusExtent;
    extern float_t DOFBlurExtent;

    constexpr size_t NUM_CONTESTANTS = 3;
    enum class GamePhases
    {
        P0_UNCOVER,
        P1_HALLWAY,
        P2_OTOMEGE,
    };
    extern bool       isGameActive;  // @NOTE: for EWU Game Jam.
    extern GamePhases currentPhase;  // @NOTE: for EWU Game Jam.
    extern float_t    playTimeRemaining;  // @NOTE: for EWU Game Jam.
    extern bool       showDebugVisuals;

    void resetGameState();
    void startNewGame();

    struct Phase0
    {
        bool                      loadTriggerFlag = false;
        vec3                      spawnBoundsOrigin;
        vec2                      spawnBoundsExtent;
        size_t                    numCoveredItemsToSpawn;
        std::vector<CoveredItem*> spawnedCoveredItems;
        Character*                contestants[NUM_CONTESTANTS];
        size_t                    nextContestantIdx = 0;
        Character*                dateDummyCharacter[NUM_CONTESTANTS];  // @NOTE: the dummy character has no collision and just shakes when revealed.
        size_t                    nextDateDummyIdx = 0;

        void registerContestant(Character* c)
        {
            contestants[nextContestantIdx++] = c;
        }
        void registerDateDummy(Character* c)
        {
            dateDummyCharacter[nextDateDummyIdx++] = c;
        }
        void uncoverDateDummy(size_t dateIdx);
    };
    extern Phase0 phase0;

    struct Phase1
    {
        bool                 loadTriggerFlag = false;
        vec3                 contASpawnPosition;
        vec3                 dateSpawnPosition;
        vec3                 contBSpawnPosition;
        vec3                 contBSpawnStride;
        Character*           contACharacter = nullptr;
        Character*           contBCharacters[NUM_CONTESTANTS - 1];
        size_t               nextContBIdx = 0;
        Character*           dateCharacter = nullptr;
        size_t               dateIdx;
        std::vector<Hazard*> hazards;

        void clearContestants()
        {
            contACharacter = nullptr;
            for (size_t i = 0; i < NUM_CONTESTANTS - 1; i++)
                contBCharacters[i] = nullptr;
            nextContBIdx = 0;
        }
        void registerContestantA(Character* c)
        {
            contACharacter = c;
        }
        void registerContestantB(Character* c)
        {
            contBCharacters[nextContBIdx++] = c;
        }
        void registerDateChar(Character* c)
        {
            dateCharacter = c;
        }
        void setDateIndex(size_t dateIdx)
        {
            Phase1::dateIdx = dateIdx;
        }
        void registerHazard(Hazard* h)
        {
            hazards.push_back(h);
        }
    };
    extern Phase1 phase1;

    struct Phase2
    {
        bool             loadTriggerFlag = false;
        bool             unloadTriggerFlag = false;
        size_t           dateIdx;
        DatingInterface* datingInterface = nullptr;

        void registerDatingInterface(DatingInterface* di)
        {
            datingInterface = di;
        }
    };
    extern Phase2 phase2;
    struct DateProps
    {
        bool isFirstPhase1 = true;
        bool isFirstPhase2Question = true;
        float_t trustLevelIncrement;  // Add this for every correct answer.
        float_t trustLevelDecrement;  // Subtract this for every incorrect answer.
        float_t currentTrustLevel = 0.0f;  // @NOTE: Initialize at zero always (but, this value is persistant among all date interactions).

        // This threshold depends on how "nice" the date is (doesn't mutate).
        // If the player selects an incorrect answer and the trust level is <= this #,
        // then date says something, and player is booted from phase2
        // back to phase 1 (and is stunned for x seconds).
        // If the trust level is > this threshold, then the date will say something like
        // "That's weird... umm... I'm fine to talk some more though" for an incorrect answer.
        // For a "reject" for asking for a date, it 
        float_t harshRejectThreshold = 20.0f;

        // Depends on how "nice" the date is (doesn't mutate).
        // If trust level is >= the "maybe" value, then the date waits.
        // Within the "maybe" range, the acceptance rate is 50%. Within
        // the "accept" range, the acceptance rate is 100%.
        float_t maybeAcceptDateInviteThreshold = 80.0f;
        float_t acceptDateInviteThreshold = 100.0f;  // 100% chance of accepting the date, but wait is still required (player shouldn't know that there is a 100% range).
        vec2    acceptDateInviteWaitTimeRange = { 2.0f, 6.0f };  // Choose random value in this range.
    };
    extern DateProps dates[NUM_CONTESTANTS];  // Equal number of dates to number of contestants.

    enum AncientWeaponItemType
    {
        WEAPON,
        FOOD,
        TOOL,
    };

    struct HarvestableItemOption
    {
        std::string name;
        std::string modelName;
    };

    struct HarvestableItemWithQuantity
    {
        size_t harvestableItemId;
        uint32_t quantity;
    };

    struct ScannableItemOption
    {
        std::string name;
        std::string modelName;
        AncientWeaponItemType type;
        std::vector<HarvestableItemWithQuantity> requiredMaterialsToMaterialize;

        struct WeaponStats  // @NOTE: garbage values if this is not a weapon.
        {
            std::string weaponType = "NULL";
            int32_t durability;
            int32_t attackPower;
            int32_t attackPowerWhenDulled;  // This is when durability hits 0.
        } weaponStats;
    };


    void initGlobalState(SceneCamera& sc, EntityManager* em);
    void launchAsyncWriteTask();  // @NOTE: this is simply for things that are marked saved
    void update(float_t deltaTime);
    void drawDebugVisualization();
    void cleanupGlobalState();

    HarvestableItemOption* getHarvestableItemByIndex(size_t index);
    uint16_t getInventoryQtyOfHarvestableItemByIndex(size_t harvestableItemId);
    void changeInventoryItemQtyByIndex(size_t harvestableItemId, int16_t changeInQty);
    size_t getNumHarvestableItemIds();

    std::string ancientWeaponItemTypeToString(AncientWeaponItemType awit);
    ScannableItemOption* getAncientWeaponItemByIndex(size_t index);
    bool getCanMaterializeScannableItemByIndex(size_t scannableItemId);
    void flagScannableItemAsCanMaterializeByIndex(size_t scannableItemId, bool flag);
    size_t getNumScannableItemIds();
    size_t getSelectedScannableItemId();
    void setSelectedScannableItemId(size_t scannableItemId);
    bool selectNextCanMaterializeScannableItemId();

    void addCameraRail(CameraRail* cameraRail);
    void removeCameraRail(CameraRail* cameraRail);
    
    CameraRail* getNearestCameraRailToDesiredRailDirection(vec3 queryPos, vec3 queryDir);

    bool showCountdown();
    bool gameIsOver();
}

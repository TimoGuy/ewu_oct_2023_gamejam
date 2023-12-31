#include "SceneManagement.h"

#include <fstream>
#include <sstream>
#include "DataSerialization.h"
#include "VulkanEngine.h"
#include "PhysicsEngine.h"
#include "Camera.h"
#include "Debug.h"
#include "StringHelper.h"
#include "GlobalState.h"

#include "Character.h"
#include "NoteTaker.h"
#include "VoxelField.h"
#include "HarvestableItem.h"
#include "CameraRail.h"
#include "CoveredItem.h"
#include "Hazard.h"
#include "DatingInterface.h"
#include "MainMenu.h"
#include "GameOverMenu.h"
#include "LogosDisplay.h"


// @PALETTE: where to add serialized names for the entities
const std::vector<std::string> ENTITY_TYPE_NAMES = {
    ":character",
    ":notetaker",
    ":voxelfield",
    ":harvestableitem",
    ":camerarail",
    ":covereditem",
    ":hazard",
    ":datinginterface",
    ":mainmenu",
    ":gameovermenu",
    ":logosdisplay",
};
const std::string Character::TYPE_NAME        = ENTITY_TYPE_NAMES[0];
const std::string NoteTaker::TYPE_NAME        = ENTITY_TYPE_NAMES[1];
const std::string VoxelField::TYPE_NAME       = ENTITY_TYPE_NAMES[2];
const std::string HarvestableItem::TYPE_NAME  = ENTITY_TYPE_NAMES[3];
const std::string CameraRail::TYPE_NAME       = ENTITY_TYPE_NAMES[4];
const std::string CoveredItem::TYPE_NAME      = ENTITY_TYPE_NAMES[5];
const std::string Hazard::TYPE_NAME           = ENTITY_TYPE_NAMES[6];
const std::string DatingInterface::TYPE_NAME  = ENTITY_TYPE_NAMES[7];
const std::string MainMenu::TYPE_NAME         = ENTITY_TYPE_NAMES[8];
const std::string GameOverMenu::TYPE_NAME     = ENTITY_TYPE_NAMES[9];
const std::string LogosDisplay::TYPE_NAME     = ENTITY_TYPE_NAMES[10];


namespace scene
{
    VulkanEngine* engine;
    bool performingDeleteAllLoadSceneProcedure;
    bool performingLoadSceneImmediateLoadSceneProcedure;
    std::string performingLoadSceneImmediateLoadSceneProcedureSavedSceneName;

    void init(VulkanEngine* inEnginePtr)
    {
        engine = inEnginePtr;
        performingDeleteAllLoadSceneProcedure = false;
        performingLoadSceneImmediateLoadSceneProcedure = false;
        performingLoadSceneImmediateLoadSceneProcedureSavedSceneName = "";
    }

    bool loadSceneImmediate(const std::string& name);
    void tick()
    {
        if (performingDeleteAllLoadSceneProcedure)
        {
            // Delete all entities.
            for (auto& ent : engine->_entityManager->_entities)
                engine->_entityManager->destroyEntity(ent);

            performingDeleteAllLoadSceneProcedure = false;
            performingLoadSceneImmediateLoadSceneProcedure = true;
        }
        else if (performingLoadSceneImmediateLoadSceneProcedure)
        {
            loadSceneImmediate(performingLoadSceneImmediateLoadSceneProcedureSavedSceneName);

            performingDeleteAllLoadSceneProcedure = false;
            performingLoadSceneImmediateLoadSceneProcedure = false;
            performingLoadSceneImmediateLoadSceneProcedureSavedSceneName = "";
        }
    }

    std::vector<std::string> getListOfEntityTypes()
    {
        return ENTITY_TYPE_NAMES;
    }

    Entity* spinupNewObject(const std::string& objectName, DataSerialized* ds)
    {
        Entity* ent = nullptr;
        if (objectName == Character::TYPE_NAME)
            ent = new Character(engine->_entityManager, engine->_roManager, engine->_camera, engine, ds);
        if (objectName == NoteTaker::TYPE_NAME)
            ent = new NoteTaker(engine->_entityManager, engine->_roManager, ds);
        if (objectName == VoxelField::TYPE_NAME)
            ent = new VoxelField(engine, engine->_entityManager, engine->_roManager, engine->_camera, ds);
        if (objectName == HarvestableItem::TYPE_NAME)
            ent = new HarvestableItem(engine->_entityManager, engine->_roManager, ds);
        if (objectName == CameraRail::TYPE_NAME)
            ent = new CameraRail(engine->_entityManager, engine->_roManager, ds);
        if (objectName == CoveredItem::TYPE_NAME)
            ent = new CoveredItem(engine->_entityManager, engine->_roManager, engine->_camera, ds);
        if (objectName == Hazard::TYPE_NAME)
            ent = new Hazard(engine->_entityManager, engine->_roManager, ds);
        if (objectName == DatingInterface::TYPE_NAME)
            ent = new DatingInterface(engine->_entityManager, engine->_roManager, engine->_camera, engine, ds);
        if (objectName == MainMenu::TYPE_NAME)
            ent = new MainMenu(engine->_entityManager, engine->_roManager, engine->_camera, engine, ds);
        if (objectName == GameOverMenu::TYPE_NAME)
            ent = new GameOverMenu(engine->_entityManager, engine->_roManager, engine->_camera, engine, ds);
        if (objectName == LogosDisplay::TYPE_NAME)
            ent = new LogosDisplay(engine->_entityManager, engine->_roManager, engine->_camera, engine, ds);

        if (ent == nullptr)
        {
            std::cerr << "[ENTITY CREATION]" << std::endl
                << "ERROR: creating entity \"" << objectName << "\" did not match any creation routines." << std::endl;
        }

        return ent;
    }

    std::vector<std::string> getListOfScenes()
    {
        std::vector<std::string> scenes;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(SCENE_DIRECTORY_PATH))
        {
            const auto& path = entry.path();
            if (std::filesystem::is_directory(path))
                continue;
            if (!path.has_extension() || path.extension().compare(".ssdat") != 0)
                continue;
            auto relativePath = std::filesystem::relative(path, SCENE_DIRECTORY_PATH);
            scenes.push_back(relativePath.string());  // @NOTE: that this line could be dangerous if there are any filenames or directory names that have utf8 chars or wchars in it
        }
        return scenes;
    }

    std::vector<std::string> getListOfPrefabs()
    {
        std::vector<std::string> prefabs;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(PREFAB_DIRECTORY_PATH))
        {
            const auto& path = entry.path();
            if (std::filesystem::is_directory(path))
                continue;
            if (!path.has_extension() || path.extension().compare(".hunk") != 0)
                continue;
            auto relativePath = std::filesystem::relative(path, PREFAB_DIRECTORY_PATH);
            prefabs.push_back(relativePath.string());  // @NOTE: that this line could be dangerous if there are any filenames or directory names that have utf8 chars or wchars in it
        }
        return prefabs;
    }

    bool loadSerializationFull(const std::string& fullFname, bool ownEntities, std::vector<Entity*>& outEntityPtrs)
    {
        bool success = true;

        DataSerializer ds;
        std::string newObjectType = "";

        std::ifstream infile(fullFname);
        std::string line;
        for (size_t lineNum = 1; std::getline(infile, line); lineNum++)
        {
            //
            // Prep line data
            //
            std::string originalLine = line;

            size_t found = line.find('#');
            if (found != std::string::npos)
            {
                line = line.substr(0, found);
            }

            trim(line);
            if (line.empty())
                continue;

            //
            // Process that line
            //
            if (line[0] == ':')
            {
                // Wrap up the previous object if there was one
                if (!newObjectType.empty())
                {
                    auto dsCooked = ds.getSerializedData();
                    Entity* newEntity = spinupNewObject(newObjectType, &dsCooked);
                    newEntity->_isOwned = ownEntities;
                    outEntityPtrs.push_back(newEntity);
                    success &= (newEntity != nullptr);
                }

                // New object
                ds = DataSerializer();
                newObjectType = line;
            }
            else if (line[0] == '~')
            {
                // @INCOMPLETE: still need to implement object count and possibly multithreading?
                //              Note that the object count would be needed to create a buffer to
                //              have the threads write to to avoid any data races when push_back()
                //              onto a vector  -Timo 2022/10/29
            }
            else if (!newObjectType.empty())
            {
                // Concat data to the object
                ds.dumpString(line);
            }
            else
            {
                // ERROR
                std::cerr << "[SCENE MANAGEMENT]" << std::endl
                    << "ERROR (line " << lineNum << ") (file: " << SCENE_DIRECTORY_PATH << fullFname << "): Headless data" << std::endl
                    << "   Trimmed line: " << line << std::endl
                    << "  Original line: " << line << std::endl;
            }
        }
        
        // @COPYPASTA: Wrap up the previous object if there was one
        if (!newObjectType.empty())
        {
            auto dsCooked = ds.getSerializedData();
            Entity* newEntity = spinupNewObject(newObjectType, &dsCooked);
            outEntityPtrs.push_back(newEntity);
            success &= (newEntity != nullptr);
        }

        return success;
    }

    bool loadPrefab(const std::string& name, std::vector<Entity*>& outEntityPtrs)
    {
        return loadSerializationFull(PREFAB_DIRECTORY_PATH + name, true, outEntityPtrs);
    }

    bool loadPrefabNonOwned(const std::string& name)
    {
        std::vector<Entity*> _;
        return loadSerializationFull(PREFAB_DIRECTORY_PATH + name, false, _);
    }
    
    bool loadScene(const std::string& name, bool deleteExistingEntitiesFirst)
    {
        if (deleteExistingEntitiesFirst)
        {
            performingDeleteAllLoadSceneProcedure = true;
            performingLoadSceneImmediateLoadSceneProcedureSavedSceneName = name;
            return true;
        }
        else
            return loadSceneImmediate(name);
    }

    bool loadSceneImmediate(const std::string& name)
    {
        std::vector<Entity*> _;
        bool ret = loadSerializationFull(SCENE_DIRECTORY_PATH + name, false, _);

        globalState::savedActiveScene = name;

        if (ret)
            debug::pushDebugMessage({
			    .message = "Successfully loaded scene \"" + name + "\"",
			    });
        else
            debug::pushDebugMessage({
                .message = "Loaded scene \"" + name + "\" with errors (see console output)",
                .type = 1,
                });

        // @DEBUG: save snapshot of physics frame.
        physengine::savePhysicsWorldSnapshot();

        return true;
    }

    bool saveScene(const std::string& name, const std::vector<Entity*>& entities)
    {
        std::ofstream outfile(SCENE_DIRECTORY_PATH + name);
        if (!outfile.is_open())
        {
            debug::pushDebugMessage({
			    .message = "Could not open file \"" + name + "\" for writing scene data",
                .type = 2,
			    });
            return false;
        }

        for (auto ent : entities)
        {
            if (ent->_isOwned)
                continue;  // Don't save owned entities.

            DataSerializer ds;
            ent->dump(ds);

            outfile << ent->getTypeName() << '\n';

            DataSerialized dsd = ds.getSerializedData();
            size_t count = dsd.getSerializedValuesCount();
            for (size_t i = 0; i < count; i++)
            {
                std::string s;
                dsd.loadString(s);
                outfile << s << '\n';
            }

            outfile << '\n';  // Extra newline for readability
        }
        outfile.close();

        debug::pushDebugMessage({
			.message = "Successfully saved scene \"" + name + "\"",
			});

        return true;
    }
}

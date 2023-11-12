#pragma once

#include "Entity.h"
class EntityManager;
class RenderObjectManager;
struct Camera;
struct GameOverMenu_XData;
class VulkanEngine;


class GameOverMenu : public Entity
{
public:
    static const std::string TYPE_NAME;
    std::string getTypeName() { return TYPE_NAME; };

    GameOverMenu(EntityManager* em, RenderObjectManager* rom, Camera* camera, VulkanEngine* engine, DataSerialized* ds);
    ~GameOverMenu();

    void physicsUpdate(const float_t& physicsDeltaTime);
    void update(const float_t& deltaTime);
    void lateUpdate(const float_t& deltaTime);

    void dump(DataSerializer& ds);
    void load(DataSerialized& ds);
    bool processMessage(DataSerialized& message);

    void reportMoved(mat4* matrixMoved);
    void renderImGui();

private:
    GameOverMenu_XData* _data;
};

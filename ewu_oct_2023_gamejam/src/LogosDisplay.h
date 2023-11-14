#pragma once

#include "Entity.h"
class EntityManager;
class RenderObjectManager;
struct Camera;
class VulkanEngine;
struct LogosDisplay_XData;


class LogosDisplay : public Entity
{
public:
    static const std::string TYPE_NAME;
    std::string getTypeName() { return TYPE_NAME; };

    LogosDisplay(EntityManager* em, RenderObjectManager* rom, Camera* camera, VulkanEngine* engine, DataSerialized* ds);
    ~LogosDisplay();

    void physicsUpdate(const float_t& physicsDeltaTime);
    void update(const float_t& deltaTime);
    void lateUpdate(const float_t& deltaTime);

    void dump(DataSerializer& ds);
    void load(DataSerialized& ds);
    bool processMessage(DataSerialized& message);

    void reportMoved(mat4* matrixMoved);
    void renderImGui();

private:
    LogosDisplay_XData* _data;
};

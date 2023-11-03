#pragma once

#include "Entity.h"
class EntityManager;
class RenderObjectManager;
struct Camera;
struct DatingInterface_XData;
class VulkanEngine;


class DatingInterface : public Entity
{
public:
    static const std::string TYPE_NAME;
    std::string getTypeName() { return TYPE_NAME; };

    DatingInterface(EntityManager* em, RenderObjectManager* rom, Camera* camera, VulkanEngine* engine, DataSerialized* ds);
    ~DatingInterface();

    void physicsUpdate(const float_t& physicsDeltaTime);
    void update(const float_t& deltaTime);
    void lateUpdate(const float_t& deltaTime);

    void dump(DataSerializer& ds);
    void load(DataSerialized& ds);
    bool processMessage(DataSerialized& message);

    void reportMoved(mat4* matrixMoved);
    void renderImGui();

    void activate(size_t dateIdx);
    void deactivate();

private:
    DatingInterface_XData* _data;
};

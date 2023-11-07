#pragma once

#include "Entity.h"
class EntityManager;
class RenderObjectManager;
struct Hazard_XData;


class Hazard : public Entity
{
public:
    static const std::string TYPE_NAME;
    std::string getTypeName() { return TYPE_NAME; };

    Hazard(EntityManager* em, RenderObjectManager* rom, DataSerialized* ds);
    ~Hazard();

    void physicsUpdate(const float_t& physicsDeltaTime);
    void update(const float_t& deltaTime);
    void lateUpdate(const float_t& deltaTime);

    void dump(DataSerializer& ds);
    void load(DataSerialized& ds);
    bool processMessage(DataSerialized& message);

    void reportMoved(mat4* matrixMoved);
    void renderImGui();

    void reportTriggerActivation(const std::string& entityGUID);

private:
    Hazard_XData* _data;
};

#pragma once

#include "Entity.h"
class EntityManager;
class RenderObjectManager;
struct Camera;
struct CoveredItem_XData;


class CoveredItem : public Entity
{
public:
    static const std::string TYPE_NAME;
    std::string getTypeName() { return TYPE_NAME; };

    CoveredItem(EntityManager* em, RenderObjectManager* rom, Camera* camera, DataSerialized* ds);
    ~CoveredItem();

    void physicsUpdate(const float_t& physicsDeltaTime);
    void update(const float_t& deltaTime);
    void lateUpdate(const float_t& deltaTime);

    void dump(DataSerializer& ds);
    void load(DataSerialized& ds);
    bool processMessage(DataSerialized& message);

    void reportMoved(mat4* matrixMoved);
    void renderImGui();

    static size_t numItemTypes();
    void setDateId(size_t dateId);
    void getPosition(vec3& outPosition);

private:
    CoveredItem_XData* _data;
};

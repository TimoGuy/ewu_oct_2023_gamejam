#pragma once

#include "Entity.h"
class EntityManager;
class RenderObjectManager;
struct Camera;
struct CameraRail_XData;


class CameraRail : public Entity
{
public:
    static const std::string TYPE_NAME;
    std::string getTypeName() { return TYPE_NAME; };

    CameraRail(EntityManager* em, RenderObjectManager* rom, DataSerialized* ds);
    ~CameraRail();

    void physicsUpdate(const float_t& physicsDeltaTime);
    void update(const float_t& deltaTime);
    void lateUpdate(const float_t& deltaTime);

    void dump(DataSerializer& ds);
    void load(DataSerialized& ds);
    bool processMessage(DataSerialized& message);

    void reportMoved(mat4* matrixMoved);
    void renderImGui();

    float_t getOrbitY();
    void getPosition(vec3& outPosition);
    float_t findDistance2FromRailIfInlineEnough(vec3 queryPos, vec3 queryDir);
    void projectPositionOnRail(vec3 pos, vec3& outProjectedPosition);

private:
    CameraRail_XData* _data;
};

#pragma once

#include "Entity.h"
class VulkanEngine;
class EntityManager;
class RenderObjectManager;
class Camera;
struct VoxelField_XData;
namespace JPH
{
    class Body;
}


class VoxelField : public Entity
{
public:
    static const std::string TYPE_NAME;
    std::string getTypeName() { return TYPE_NAME; };

    VoxelField(VulkanEngine* engine, EntityManager* em, RenderObjectManager* rom, Camera* camera, DataSerialized* ds);
    ~VoxelField();

    void physicsUpdate(const float_t& physicsDeltaTime);
    void update(const float_t& deltaTime);
    void lateUpdate(const float_t& deltaTime);

    void dump(DataSerializer& ds);
    void load(DataSerialized& ds);

    void reportMoved(mat4* matrixMoved);
    void renderImGui();

    void setBodyKinematic(bool isKinematic);
    void moveBody(vec3 newPosition, versor newRotation, bool immediate, float_t physicsDeltaTime);
    void getSize(vec3& outSize);
    void getTransform(mat4& outTransform);

    void reportPlayerInCameraRailTrigger(const JPH::Body& otherBody, float_t playerFacingDirection);

private:
    VoxelField_XData* _data;
};

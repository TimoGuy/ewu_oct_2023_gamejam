#pragma once

#include "Entity.h"
class EntityManager;
class RenderObjectManager;
struct Camera;
struct Character_XData;
namespace JPH
{
    class Body;
    class ContactManifold;
    class ContactSettings;
}
enum class RenderLayer;
class VulkanEngine;


class Character : public Entity
{
public:
    static const std::string TYPE_NAME;
    std::string getTypeName() { return TYPE_NAME; };

    Character(EntityManager* em, RenderObjectManager* rom, Camera* camera, VulkanEngine* engine, DataSerialized* ds);
    ~Character();

    void physicsUpdate(const float_t& physicsDeltaTime);
    void update(const float_t& deltaTime);
    void lateUpdate(const float_t& deltaTime);

    void dump(DataSerializer& ds);
    void load(DataSerialized& ds);
    bool processMessage(DataSerialized& message);

    void reportMoved(mat4* matrixMoved);
    void renderImGui();

    void setAsCameraTargetObject();
    void reportPhysicsContact(const JPH::Body& otherBody, const JPH::ContactManifold& manifold, JPH::ContactSettings* ioSettings, bool persistedContact);
    bool isPlayer();
    void stun(float_t time, bool playSfx);
    bool isStunned();
    void setFacingRight(bool facingRight);
    float_t getFacingDirection();
    void getPosition(vec3& outPosition);
    void setContestantIndex(size_t contestantId);
    void activateDate(size_t dateId);
    void setDateDummyIndex(size_t dateId);
    void moreOrLessSpawnAtPosition(vec3 position);
    void setRenderLayer(const RenderLayer& renderLayer);

private:
    Character_XData* _data;
};

#include "PhysicsEngine.h"

#ifdef _DEVELOP
#include <array>
#include "VkDataStructures.h"
#include "VulkanEngine.h"
#include "VkDescriptorBuilderUtil.h"
#include "VkPipelineBuilderUtil.h"
#include "VkInitializers.h"
#include "Camera.h"
#endif

#include <SDL2/SDL.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <format>
#include <map>
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/PhysicsScene.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include "PhysUtil.h"
#include "EntityManager.h"
#include "Character.h"
#include "Hazard.h"
#include "VoxelField.h"
#include "GlobalState.h"
#include "imgui/imgui.h"
#include "imgui/implot.h"

JPH_SUPPRESS_WARNINGS
using namespace JPH;
using namespace JPH::literals;


namespace physengine
{
    bool isInitialized = false;

    //
    // Physics engine works
    //
    constexpr float_t physicsDeltaTime = 0.025f;    // 40fps. This seemed to be the sweet spot. 25/30fps would be inconsistent for getting smaller platform jumps with the dash move. 50fps felt like too many physics calculations all at once. 40fps seems right, striking a balance.  -Timo 2023/01/26
    constexpr float_t physicsDeltaTimeInMS = physicsDeltaTime * 1000.0f;
    constexpr float_t oneOverPhysicsDeltaTimeInMS = 1.0f / physicsDeltaTimeInMS;

    constexpr float_t collisionTolerance = 0.05f;  // For physics characters.

    void runPhysicsEngineAsync();
    EntityManager* entityManager;
    bool isAsyncRunnerRunning;
    std::thread* asyncRunner = nullptr;
    uint64_t lastTick;

    PhysicsSystem* physicsSystem = nullptr;
    std::map<uint32_t, std::string> bodyIdToEntityGuidMap;

#ifdef _DEVELOP
    struct DebugStats
    {
        size_t simTimesUSHeadIndex = 0;
        size_t simTimesUSCount = 256;
        float_t simTimesUS[256 * 2];
        float_t highestSimTime = -1.0f;
    } perfStats;

    //
    // Debug visualization
    // @INCOMPLETE: for now just have capsules and raycasts be visualized, since the 3d models for the voxel fields is an accurate visualization of it anyways.  -Timo 2023/06/13
    //
    VulkanEngine* engine;
    struct GPUVisCameraData
    {
        mat4 projectionView;
    };
    AllocatedBuffer visCameraBuffer;

    struct GPUVisInstancePushConst
    {
        vec4 color1;
        vec4 color2;
        vec4 pt1;  // Vec4's for padding.
        vec4 pt2;
        float_t capsuleRadius;
    };

    struct DebugVisVertex
    {
        vec3 pos;
        int32_t pointSpace;  // 0 is pt1 space. 1 is pt2 space.
    };
    AllocatedBuffer capsuleVisVertexBuffer;  // @NOTE: there will be vertex attributes that will set a vertex to pt1's transform or pt2's transform.
    AllocatedBuffer lineVisVertexBuffer;
    uint32_t capsuleVisVertexCount;
    uint32_t lineVisVertexCount;
    bool vertexBuffersInitialized = false;

    struct DebugVisLine
    {
        vec3 pt1, pt2;
        DebugVisLineType type;
    };
    std::vector<DebugVisLine> debugVisLines;
    std::mutex mutateDebugVisLines;

    void initializeAndUploadBuffers()
    {
        static std::vector<DebugVisVertex> capsuleVertices = {
            // Bottom cap, y-plane circle.
            { {  1.000f, 0.0f,  0.000f }, 0 }, { {  0.924f, 0.0f,  0.383f }, 0 },
            { {  0.924f, 0.0f,  0.383f }, 0 }, { {  0.707f, 0.0f,  0.707f }, 0 },
            { {  0.707f, 0.0f,  0.707f }, 0 }, { {  0.383f, 0.0f,  0.924f }, 0 },
            { {  0.383f, 0.0f,  0.924f }, 0 }, { {  0.000f, 0.0f,  1.000f }, 0 },
            { {  0.000f, 0.0f,  1.000f }, 0 }, { { -0.383f, 0.0f,  0.924f }, 0 },
            { { -0.383f, 0.0f,  0.924f }, 0 }, { { -0.707f, 0.0f,  0.707f }, 0 },
            { { -0.707f, 0.0f,  0.707f }, 0 }, { { -0.924f, 0.0f,  0.383f }, 0 },
            { { -0.924f, 0.0f,  0.383f }, 0 }, { { -1.000f, 0.0f,  0.000f }, 0 },
            { { -1.000f, 0.0f,  0.000f }, 0 }, { { -0.924f, 0.0f, -0.383f }, 0 },
            { { -0.924f, 0.0f, -0.383f }, 0 }, { { -0.707f, 0.0f, -0.707f }, 0 },
            { { -0.707f, 0.0f, -0.707f }, 0 }, { { -0.383f, 0.0f, -0.924f }, 0 },
            { { -0.383f, 0.0f, -0.924f }, 0 }, { {  0.000f, 0.0f, -1.000f }, 0 },
            { {  0.000f, 0.0f, -1.000f }, 0 }, { {  0.383f, 0.0f, -0.924f }, 0 },
            { {  0.383f, 0.0f, -0.924f }, 0 }, { {  0.707f, 0.0f, -0.707f }, 0 },
            { {  0.707f, 0.0f, -0.707f }, 0 }, { {  0.924f, 0.0f, -0.383f }, 0 },
            { {  0.924f, 0.0f, -0.383f }, 0 }, { {  1.000f, 0.0f,  0.000f }, 0 },

            // Top cap, y-plane circle.
            { {  1.000f, 0.0f,  0.000f }, 1 }, { {  0.924f, 0.0f,  0.383f }, 1 },
            { {  0.924f, 0.0f,  0.383f }, 1 }, { {  0.707f, 0.0f,  0.707f }, 1 },
            { {  0.707f, 0.0f,  0.707f }, 1 }, { {  0.383f, 0.0f,  0.924f }, 1 },
            { {  0.383f, 0.0f,  0.924f }, 1 }, { {  0.000f, 0.0f,  1.000f }, 1 },
            { {  0.000f, 0.0f,  1.000f }, 1 }, { { -0.383f, 0.0f,  0.924f }, 1 },
            { { -0.383f, 0.0f,  0.924f }, 1 }, { { -0.707f, 0.0f,  0.707f }, 1 },
            { { -0.707f, 0.0f,  0.707f }, 1 }, { { -0.924f, 0.0f,  0.383f }, 1 },
            { { -0.924f, 0.0f,  0.383f }, 1 }, { { -1.000f, 0.0f,  0.000f }, 1 },
            { { -1.000f, 0.0f,  0.000f }, 1 }, { { -0.924f, 0.0f, -0.383f }, 1 },
            { { -0.924f, 0.0f, -0.383f }, 1 }, { { -0.707f, 0.0f, -0.707f }, 1 },
            { { -0.707f, 0.0f, -0.707f }, 1 }, { { -0.383f, 0.0f, -0.924f }, 1 },
            { { -0.383f, 0.0f, -0.924f }, 1 }, { {  0.000f, 0.0f, -1.000f }, 1 },
            { {  0.000f, 0.0f, -1.000f }, 1 }, { {  0.383f, 0.0f, -0.924f }, 1 },
            { {  0.383f, 0.0f, -0.924f }, 1 }, { {  0.707f, 0.0f, -0.707f }, 1 },
            { {  0.707f, 0.0f, -0.707f }, 1 }, { {  0.924f, 0.0f, -0.383f }, 1 },
            { {  0.924f, 0.0f, -0.383f }, 1 }, { {  1.000f, 0.0f,  0.000f }, 1 },

            // X-plane circle.
            { { 0.0f,  1.000f,  0.000f }, 1 }, { { 0.0f,  0.924f,  0.383f }, 1 },
            { { 0.0f,  0.924f,  0.383f }, 1 }, { { 0.0f,  0.707f,  0.707f }, 1 },
            { { 0.0f,  0.707f,  0.707f }, 1 }, { { 0.0f,  0.383f,  0.924f }, 1 },
            { { 0.0f,  0.383f,  0.924f }, 1 }, { { 0.0f,  0.000f,  1.000f }, 1 },
            { { 0.0f,  0.000f,  1.000f }, 0 }, { { 0.0f, -0.383f,  0.924f }, 0 },
            { { 0.0f, -0.383f,  0.924f }, 0 }, { { 0.0f, -0.707f,  0.707f }, 0 },
            { { 0.0f, -0.707f,  0.707f }, 0 }, { { 0.0f, -0.924f,  0.383f }, 0 },
            { { 0.0f, -0.924f,  0.383f }, 0 }, { { 0.0f, -1.000f,  0.000f }, 0 },
            { { 0.0f, -1.000f,  0.000f }, 0 }, { { 0.0f, -0.924f, -0.383f }, 0 },
            { { 0.0f, -0.924f, -0.383f }, 0 }, { { 0.0f, -0.707f, -0.707f }, 0 },
            { { 0.0f, -0.707f, -0.707f }, 0 }, { { 0.0f, -0.383f, -0.924f }, 0 },
            { { 0.0f, -0.383f, -0.924f }, 0 }, { { 0.0f,  0.000f, -1.000f }, 0 },
            { { 0.0f,  0.000f, -1.000f }, 1 }, { { 0.0f,  0.383f, -0.924f }, 1 },
            { { 0.0f,  0.383f, -0.924f }, 1 }, { { 0.0f,  0.707f, -0.707f }, 1 },
            { { 0.0f,  0.707f, -0.707f }, 1 }, { { 0.0f,  0.924f, -0.383f }, 1 },
            { { 0.0f,  0.924f, -0.383f }, 1 }, { { 0.0f,  1.000f,  0.000f }, 1 },

            // Z-plane circle.
            { {  1.000f,  0.000f, 0.0f }, 1 }, { {  0.924f,  0.383f, 0.0f }, 1 },
            { {  0.924f,  0.383f, 0.0f }, 1 }, { {  0.707f,  0.707f, 0.0f }, 1 },
            { {  0.707f,  0.707f, 0.0f }, 1 }, { {  0.383f,  0.924f, 0.0f }, 1 },
            { {  0.383f,  0.924f, 0.0f }, 1 }, { {  0.000f,  1.000f, 0.0f }, 1 },
            { {  0.000f,  1.000f, 0.0f }, 1 }, { { -0.383f,  0.924f, 0.0f }, 1 },
            { { -0.383f,  0.924f, 0.0f }, 1 }, { { -0.707f,  0.707f, 0.0f }, 1 },
            { { -0.707f,  0.707f, 0.0f }, 1 }, { { -0.924f,  0.383f, 0.0f }, 1 },
            { { -0.924f,  0.383f, 0.0f }, 1 }, { { -1.000f,  0.000f, 0.0f }, 1 },
            { { -1.000f,  0.000f, 0.0f }, 0 }, { { -0.924f, -0.383f, 0.0f }, 0 },
            { { -0.924f, -0.383f, 0.0f }, 0 }, { { -0.707f, -0.707f, 0.0f }, 0 },
            { { -0.707f, -0.707f, 0.0f }, 0 }, { { -0.383f, -0.924f, 0.0f }, 0 },
            { { -0.383f, -0.924f, 0.0f }, 0 }, { {  0.000f, -1.000f, 0.0f }, 0 },
            { {  0.000f, -1.000f, 0.0f }, 0 }, { {  0.383f, -0.924f, 0.0f }, 0 },
            { {  0.383f, -0.924f, 0.0f }, 0 }, { {  0.707f, -0.707f, 0.0f }, 0 },
            { {  0.707f, -0.707f, 0.0f }, 0 }, { {  0.924f, -0.383f, 0.0f }, 0 },
            { {  0.924f, -0.383f, 0.0f }, 0 }, { {  1.000f,  0.000f, 0.0f }, 0 },

            // Connecting lines.
            { { -1.0f, 0.0f,  0.0f }, 0 }, { { -1.0f, 0.0f,  0.0f }, 1 },
            { {  1.0f, 0.0f,  0.0f }, 0 }, { {  1.0f, 0.0f,  0.0f }, 1 },
            { {  0.0f, 0.0f, -1.0f }, 0 }, { {  0.0f, 0.0f, -1.0f }, 1 },
            { {  0.0f, 0.0f,  1.0f }, 0 }, { {  0.0f, 0.0f,  1.0f }, 1 },
        };
        size_t capsuleVerticesSize = sizeof(DebugVisVertex) * capsuleVertices.size();
        AllocatedBuffer    cUp = engine->createBuffer(capsuleVerticesSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        capsuleVisVertexBuffer = engine->createBuffer(capsuleVerticesSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
        capsuleVisVertexCount = (uint32_t)capsuleVertices.size();

        static std::vector<DebugVisVertex> lineVertices = {
            { { 0.0f, 0.0f, 0.0f }, 0 }, { { 0.0f, 0.0f, 0.0f }, 1 },
        };
        size_t lineVerticesSize = sizeof(DebugVisVertex) * lineVertices.size();
        AllocatedBuffer lUp = engine->createBuffer(lineVerticesSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        lineVisVertexBuffer = engine->createBuffer(lineVerticesSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
        lineVisVertexCount = (uint32_t)lineVertices.size();

        void* data;
        vmaMapMemory(engine->_allocator, cUp._allocation, &data);
        memcpy(data, capsuleVertices.data(), capsuleVerticesSize);
        vmaUnmapMemory(engine->_allocator, cUp._allocation);
        vmaMapMemory(engine->_allocator, lUp._allocation, &data);
        memcpy(data, lineVertices.data(), lineVerticesSize);
        vmaUnmapMemory(engine->_allocator, lUp._allocation);

        engine->immediateSubmit([&](VkCommandBuffer cmd) {
            VkBufferCopy copyRegion = {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = capsuleVerticesSize,
            };
            vkCmdCopyBuffer(cmd, cUp._buffer, capsuleVisVertexBuffer._buffer, 1, &copyRegion);

            copyRegion.size = lineVerticesSize;
            vkCmdCopyBuffer(cmd, lUp._buffer, lineVisVertexBuffer._buffer, 1, &copyRegion);
            });

        vmaDestroyBuffer(engine->_allocator, cUp._buffer, cUp._allocation);
        vmaDestroyBuffer(engine->_allocator, lUp._buffer, lUp._allocation);

        visCameraBuffer = engine->createBuffer(sizeof(GPUVisCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }

    VkDescriptorSet debugVisDescriptor;
    VkDescriptorSetLayout debugVisDescriptorLayout;

    void initDebugVisDescriptors(VulkanEngine* engineRef)
    {
        engine = engineRef;
        if (!vertexBuffersInitialized)
        {
            initializeAndUploadBuffers();
            vertexBuffersInitialized = true;
        }

        VkDescriptorBufferInfo debugVisCameraInfo = {
            .buffer = visCameraBuffer._buffer,
            .offset = 0,
            .range = sizeof(GPUVisCameraData),
        };

        vkutil::DescriptorBuilder::begin()
            .bindBuffer(0, &debugVisCameraInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .build(debugVisDescriptor, debugVisDescriptorLayout);
    }

    VkPipeline debugVisPipeline = VK_NULL_HANDLE;
    VkPipelineLayout debugVisPipelineLayout;

    void initDebugVisPipelines(VkRenderPass mainRenderPass, VkViewport& screenspaceViewport, VkRect2D& screenspaceScissor, DeletionQueue& deletionQueue)
    {
        // Setup vertex descriptions
        VkVertexInputAttributeDescription posAttribute = {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(DebugVisVertex, pos),
        };
        VkVertexInputAttributeDescription pointSpaceAttribute = {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32_SINT,
            .offset = offsetof(DebugVisVertex, pointSpace),
        };
        std::vector<VkVertexInputAttributeDescription> attributes = { posAttribute, pointSpaceAttribute };

        VkVertexInputBindingDescription mainBinding = {
            .binding = 0,
            .stride = sizeof(DebugVisVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
        std::vector<VkVertexInputBindingDescription> bindings = { mainBinding };

        // Build pipeline
        vkutil::pipelinebuilder::build(
            {
                VkPushConstantRange{
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset = 0,
                    .size = sizeof(GPUVisInstancePushConst)
                }
            },
            { debugVisDescriptorLayout },
            {
                { VK_SHADER_STAGE_VERTEX_BIT, "shader/physengineDebugVis.vert.spv" },
                { VK_SHADER_STAGE_FRAGMENT_BIT, "shader/physengineDebugVis.frag.spv" },
            },
            attributes,
            bindings,
            vkinit::inputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST),
            screenspaceViewport,
            screenspaceScissor,
            vkinit::rasterizationStateCreateInfo(VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE),
            { vkinit::colorBlendAttachmentState() },
            vkinit::multisamplingStateCreateInfo(),
            vkinit::depthStencilCreateInfo(false, false, VK_COMPARE_OP_NEVER),
            {},
            mainRenderPass,
            1,
            debugVisPipeline,
            debugVisPipelineLayout,
            deletionQueue
        );
    }

    void savePhysicsWorldSnapshot()
    {
        // Convert physics system to scene
        Ref<PhysicsScene> scene = new PhysicsScene();
        scene->FromPhysicsSystem(physicsSystem);

        // Save scene
        std::ofstream stream("physics_world_snapshot.bin", std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
        StreamOutWrapper wrapper(stream);
        if (stream.is_open())
            scene->SaveBinaryState(wrapper, true, true);
    }
#endif

    void start(EntityManager* em)
    {
        entityManager = em;
        isAsyncRunnerRunning = true;
        asyncRunner = new std::thread(runPhysicsEngineAsync);
    }

    void haltAsyncRunner()
    {
        isAsyncRunnerRunning = false;
        asyncRunner->join();
    }

    void cleanup()
    {
        delete asyncRunner;
        delete physicsSystem;

        UnregisterTypes();

        delete Factory::sInstance;
        Factory::sInstance = nullptr;

#ifdef _DEVELOP
        vmaDestroyBuffer(engine->_allocator, visCameraBuffer._buffer, visCameraBuffer._allocation);
        vmaDestroyBuffer(engine->_allocator, capsuleVisVertexBuffer._buffer, capsuleVisVertexBuffer._allocation);
        vmaDestroyBuffer(engine->_allocator, lineVisVertexBuffer._buffer, lineVisVertexBuffer._allocation);
#endif
    }

    float_t getPhysicsAlpha()
    {
        return (SDL_GetTicks64() - lastTick) * oneOverPhysicsDeltaTimeInMS * globalState::timescale;
    }

    void getWorldTransform(BodyID bodyId, mat4& outWorldTransform)
    {
        JPH::RMat44 trans = physicsSystem->GetBodyInterface().GetWorldTransform(bodyId);
        // Copy to cglm style.
        JPH::Vec4 c0 = trans.GetColumn4(0);
        JPH::Vec4 c1 = trans.GetColumn4(1);
        JPH::Vec4 c2 = trans.GetColumn4(2);
        JPH::Vec4 c3 = trans.GetColumn4(3);
        outWorldTransform[0][0] = c0.GetX();
        outWorldTransform[0][1] = c0.GetY();
        outWorldTransform[0][2] = c0.GetZ();
        outWorldTransform[0][3] = c0.GetW();
        outWorldTransform[1][0] = c1.GetX();
        outWorldTransform[1][1] = c1.GetY();
        outWorldTransform[1][2] = c1.GetZ();
        outWorldTransform[1][3] = c1.GetW();
        outWorldTransform[2][0] = c2.GetX();
        outWorldTransform[2][1] = c2.GetY();
        outWorldTransform[2][2] = c2.GetZ();
        outWorldTransform[2][3] = c2.GetW();
        outWorldTransform[3][0] = c3.GetX();
        outWorldTransform[3][1] = c3.GetY();
        outWorldTransform[3][2] = c3.GetZ();
        outWorldTransform[3][3] = c3.GetW();
    }

    void tick();
    void tock();

    static void TraceImpl(const char* inFMT, ...)  // Callback for traces, connect this to your own trace function if you have one
    {
        // Format the message
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);

        // Print to the TTY
        std::cout << buffer << std::endl;
    }

#ifdef JPH_ENABLE_ASSERTS

    // Callback for asserts, connect this to your own assert handler if you have one
    static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint32_t inLine)
    {
        // Print to the TTY
        std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;

        // Breakpoint
        return true;
    };

#endif // JPH_ENABLE_ASSERTS

    /// Class that determines if two object layers can collide
    class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING; // Non moving only collides with moving
            case Layers::MOVING:
                return true; // Moving collides with everything
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

    // BroadPhaseLayerInterface implementation
    // This defines a mapping between object and broadphase layers.
    class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            // Create a mapping table from object to broad phase layer
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        virtual uint32_t GetNumBroadPhaseLayers() const override
        {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
        {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
        {
            switch ((BroadPhaseLayer::Type)inLayer)
            {
            case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
            case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
            default:													JPH_ASSERT(false); return "INVALID";
            }
        }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

    private:
        BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };

    /// Class that determines if an object layer can collide with a broadphase layer
    class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
        {
            switch (inLayer1)
            {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

    // An example contact listener
    class MyContactListener : public ContactListener
    {
    public:
        // See: ContactListener
        virtual ValidateResult OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult) override
        {
            //std::cout << "Contact validate callback" << std::endl;

            // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
            return ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        void processUserDataMeaning(const Body& thisBody, const Body& otherBody, const ContactManifold& manifold, ContactSettings& ioSettings, bool persistedContact)
        {
            switch (UserDataMeaning(thisBody.GetUserData()))
            {
                case UserDataMeaning::NOTHING:
                    return;
                
                case UserDataMeaning::IS_CHARACTER:
                {
                    uint32_t id = thisBody.GetID().GetIndex();
                    ::Character* entityAsChar;
                    if (entityAsChar = dynamic_cast<::Character*>(entityManager->getEntityViaGUID(bodyIdToEntityGuidMap[id])))
                        entityAsChar->reportPhysicsContact(otherBody, manifold, &ioSettings, persistedContact);
                } return;

                case UserDataMeaning::IS_NS_CAMERA_RAIL_TRIGGER:  // @NOTE: these are supposed to be processed in the character's `reportPhysicsContact`.
                case UserDataMeaning::IS_EW_CAMERA_RAIL_TRIGGER:
                {
                    if (UserDataMeaning(otherBody.GetUserData()) == UserDataMeaning::IS_CHARACTER)
                    {
                        ::Character* entityAsChar = dynamic_cast<::Character*>(entityManager->getEntityViaGUID(bodyIdToEntityGuidMap[otherBody.GetID().GetIndex()]));
                        if (entityAsChar && entityAsChar->isPlayer())
                        {
                            uint32_t id = thisBody.GetID().GetIndex();
                            VoxelField* entityAsVF;
                            if (entityAsVF = dynamic_cast<VoxelField*>(entityManager->getEntityViaGUID(bodyIdToEntityGuidMap[id])))
                                entityAsVF->reportPlayerInCameraRailTrigger(otherBody, entityAsChar->getFacingDirection());
                        }
                    }
                } return;

                case UserDataMeaning::IS_TRIGGER:
                {
                    uint32_t id = thisBody.GetID().GetIndex();
                    uint32_t otherId = otherBody.GetID().GetIndex();
                    Hazard* entityAsHazard;
                    if (entityAsHazard = dynamic_cast<Hazard*>(entityManager->getEntityViaGUID(bodyIdToEntityGuidMap[id])))
                        entityAsHazard->reportTriggerActivation(bodyIdToEntityGuidMap[otherId]);
                } return;
            }
        }

        virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
        {
            //std::cout << "A contact was added" << std::endl;
            processUserDataMeaning(inBody1, inBody2, inManifold, ioSettings, false);
            processUserDataMeaning(inBody2, inBody1, inManifold.SwapShapes(), ioSettings, false);
        }

        virtual void OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
        {
            //std::cout << "A contact was persisted" << std::endl;
            processUserDataMeaning(inBody1, inBody2, inManifold, ioSettings, true);
            processUserDataMeaning(inBody2, inBody1, inManifold.SwapShapes(), ioSettings, true);
        }

        virtual void OnContactRemoved(const SubShapeIDPair& inSubShapePair) override
        {
            //std::cout << "A contact was removed" << std::endl;
        }
    } contactListener;

    // An example activation listener
    class MyBodyActivationListener : public BodyActivationListener
    {
    public:
        virtual void OnBodyActivated(const BodyID& inBodyID, uint64_t inBodyUserData) override
        {
            //std::cout << "A body got activated" << std::endl;
        }

        virtual void OnBodyDeactivated(const BodyID& inBodyID, uint64_t inBodyUserData) override
        {
            //std::cout << "A body went to sleep" << std::endl;
        }
    } bodyActivationListener;

    void runPhysicsEngineAsync()
    {
        //
        // Init Physics World.
        // REFERENCE: https://github.com/jrouwe/JoltPhysics/blob/master/HelloWorld/HelloWorld.cpp
        //
        RegisterDefaultAllocator();

        Trace = TraceImpl;
        JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl);

        Factory::sInstance = new Factory();
        RegisterTypes();

        TempAllocatorImpl tempAllocator(10 * 1024 * 1024);

        constexpr int32_t maxPhysicsJobs = 2048;
        constexpr int32_t maxPhysicsBarriers = 8;
        JobSystemThreadPool jobSystem(maxPhysicsJobs, maxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

        const uint32_t maxBodies = 65536;
        const uint32_t numBodyMutexes = 0;  // Default settings is no mutexes to protect bodies from concurrent access.
        const uint32_t maxBodyPairs = 65536;
        const uint32_t maxContactConstraints = 10240;

        BPLayerInterfaceImpl broadphaseLayerInterface;
        ObjectVsBroadPhaseLayerFilterImpl objectVsBroadphaseLayerFilter;
        ObjectLayerPairFilterImpl objectVsObjectLayerFilter;

        physicsSystem = new PhysicsSystem;
        physicsSystem->Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints, broadphaseLayerInterface, objectVsBroadphaseLayerFilter, objectVsObjectLayerFilter);
        physicsSystem->SetBodyActivationListener(&bodyActivationListener);
        physicsSystem->SetContactListener(&contactListener);
        setWorldGravity(vec3{ 0.0f, -37.5f, 0.0f });  // @NOTE: This is tuned to be the original.  -Timo 2023/09/29

        physicsSystem->OptimizeBroadPhase();

        isInitialized = true;  // Initialization finished.

        //
        // Run Physics Simulation until no more.
        //
        while (isAsyncRunnerRunning)
        {
            lastTick = SDL_GetTicks64();

#ifdef _DEVELOP
            uint64_t perfTime = SDL_GetPerformanceCounter();

            {   // Reset all the debug vis lines.
                std::lock_guard<std::mutex> lg(mutateDebugVisLines);
                debugVisLines.clear();
            }
#endif
            // @NOTE: this is the only place where `timeScale` is used. That's
            //        because this system is designed to be running at 40fps constantly
            //        in real time, so it doesn't slow down or speed up with time scale.
            // @REPLY: I thought that the system should just run in a constant 40fps. As in,
            //         if the timescale slows down, then the tick rate should also slow down
            //         proportionate to the timescale.  -Timo 2023/06/10
            tick();
            entityManager->INTERNALphysicsUpdate(physicsDeltaTime);  // @NOTE: if timescale changes, then the system just waits longer/shorter.
            globalState::drawDebugVisualization();
            physicsSystem->Update(physicsDeltaTime, 1, 1, &tempAllocator, &jobSystem);
            tock();

#ifdef _DEVELOP
            {
                //
                // Update performance metrics
                // @COPYPASTA
                //
                perfTime = SDL_GetPerformanceCounter() - perfTime;
                perfStats.simTimesUSHeadIndex = (size_t)std::fmodf((float_t)perfStats.simTimesUSHeadIndex + 1, (float_t)perfStats.simTimesUSCount);

                // Find what the highest simulation time is
                if (perfTime > perfStats.highestSimTime)
                    perfStats.highestSimTime = perfTime;
                else if (perfStats.simTimesUS[perfStats.simTimesUSHeadIndex] == perfStats.highestSimTime)
                {
                    // Former highest sim time is getting overwritten; recalculate the 2nd highest sim time.
                    float_t nextHighestsimTime = perfTime;
                    for (size_t i = perfStats.simTimesUSHeadIndex + 1; i < perfStats.simTimesUSHeadIndex + perfStats.simTimesUSCount; i++)
                        nextHighestsimTime = std::max(nextHighestsimTime, perfStats.simTimesUS[i]);
                    perfStats.highestSimTime = nextHighestsimTime;
                }

                // Apply simulation time to buffer
                perfStats.simTimesUS[perfStats.simTimesUSHeadIndex] =
                    perfStats.simTimesUS[perfStats.simTimesUSHeadIndex + perfStats.simTimesUSCount] =
                    perfTime;
            }
#endif

            // Wait for remaining time
            uint64_t endingTime = SDL_GetTicks64();
            uint64_t timeDiff = endingTime - lastTick;
            uint64_t physicsDeltaTimeInMSScaled = physicsDeltaTimeInMS / globalState::timescale;
            if (timeDiff > physicsDeltaTimeInMSScaled)
            {
                std::cerr << "ERROR: physics engine is running too slowly. (" << (timeDiff - physicsDeltaTimeInMSScaled) << "ns behind)" << std::endl;
            }
            else
            {
                SDL_Delay((uint32_t)(physicsDeltaTimeInMSScaled - timeDiff));
            }
        }
    }

    //
    // Voxel field pool
    //
    constexpr size_t PHYSICS_OBJECTS_MAX_CAPACITY = 10000;

    VoxelFieldPhysicsData voxelFieldPool[PHYSICS_OBJECTS_MAX_CAPACITY];
    size_t voxelFieldIndices[PHYSICS_OBJECTS_MAX_CAPACITY];
    size_t numVFsCreated = 0;

    VoxelFieldPhysicsData* createVoxelField(const std::string& entityGuid, mat4 transform, const size_t& sizeX, const size_t& sizeY, const size_t& sizeZ, uint8_t* voxelData)
    {
        if (numVFsCreated < PHYSICS_OBJECTS_MAX_CAPACITY)
        {
            // Pull a voxel field from the pool
            size_t index = 0;
            if (numVFsCreated > 0)
            {
                index = (voxelFieldIndices[numVFsCreated - 1] + 1) % PHYSICS_OBJECTS_MAX_CAPACITY;
                voxelFieldIndices[numVFsCreated] = index;
            }
            VoxelFieldPhysicsData& vfpd = voxelFieldPool[index];
            numVFsCreated++;

            // Insert in the data
            vfpd.entityGuid = entityGuid;
            glm_mat4_copy(transform, vfpd.transform);
            vfpd.sizeX = sizeX;
            vfpd.sizeY = sizeY;
            vfpd.sizeZ = sizeZ;
            vfpd.voxelData = voxelData;
            vfpd.bodyId = JPH::BodyID();
            vfpd.nsTriggerBodyId = JPH::BodyID();
            vfpd.ewTriggerBodyId = JPH::BodyID();

            return &vfpd;
        }
        else
        {
            std::cerr << "ERROR: voxel field creation has reached its limit" << std::endl;
            return nullptr;
        }
    }

    bool destroyVoxelField(VoxelFieldPhysicsData* vfpd)
    {
        for (size_t i = 0; i < numVFsCreated; i++)
        {
            size_t& index = voxelFieldIndices[i];
            if (&voxelFieldPool[index] == vfpd)
            {
                if (numVFsCreated > 1)
                {
                    // Overwrite the index with the back index,
                    // effectively deleting the index
                    index = voxelFieldIndices[numVFsCreated - 1];
                }
                numVFsCreated--;

                // Remove and delete the voxel field body.
                BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
                bodyInterface.RemoveBody(vfpd->bodyId);
                bodyInterface.DestroyBody(vfpd->bodyId);

                return true;
            }
        }
        return false;
    }

    uint8_t getVoxelDataAtPosition(const VoxelFieldPhysicsData& vfpd, const int32_t& x, const int32_t& y, const int32_t& z)
    {
        if (x < 0 || y < 0 || z < 0 ||
            x >= vfpd.sizeX || y >= vfpd.sizeY || z >= vfpd.sizeZ)
            return 0;
        return vfpd.voxelData[(size_t)x * vfpd.sizeY * vfpd.sizeZ + (size_t)y * vfpd.sizeZ + (size_t)z];
    }

    bool setVoxelDataAtPosition(const VoxelFieldPhysicsData& vfpd, const int32_t& x, const int32_t& y, const int32_t& z, uint8_t data)
    {
        if (x < 0 || y < 0 || z < 0 ||
            x >= vfpd.sizeX || y >= vfpd.sizeY || z >= vfpd.sizeZ)
            return false;
        vfpd.voxelData[(size_t)x * vfpd.sizeY * vfpd.sizeZ + (size_t)y * vfpd.sizeZ + (size_t)z] = data;
        return true;
    }

    void expandVoxelFieldBounds(VoxelFieldPhysicsData& vfpd, ivec3 boundsMin, ivec3 boundsMax, ivec3& outOffset)
    {
        ivec3 newSize;
        glm_ivec3_maxv(ivec3{ (int32_t)vfpd.sizeX, (int32_t)vfpd.sizeY, (int32_t)vfpd.sizeZ }, ivec3{ boundsMax[0] + 1, boundsMax[1] + 1, boundsMax[2] + 1 }, newSize);

        glm_ivec3_minv(ivec3{ 0, 0, 0 }, boundsMin, outOffset);
        glm_ivec3_mul(outOffset, ivec3{ -1, -1, -1 }, outOffset);
        glm_ivec3_add(newSize, outOffset, newSize);  // Adds on the offset.

        // Create a new data grid with new bounds.
        size_t arraySize = newSize[0] * newSize[1] * newSize[2];
        uint8_t* newVD = new uint8_t[arraySize];
        for (size_t i = 0; i < arraySize; i++)
            newVD[i] = 0;  // Init the value to be empty.

        for (size_t i = 0; i < vfpd.sizeX; i++)
        for (size_t j = 0; j < vfpd.sizeY; j++)
        for (size_t k = 0; k < vfpd.sizeZ; k++)
        {
            uint8_t data = vfpd.voxelData[i * vfpd.sizeY * vfpd.sizeZ + j * vfpd.sizeZ + k];
            ivec3 newIJK;
            glm_ivec3_add(ivec3{ (int32_t)i, (int32_t)j, (int32_t)k }, outOffset, newIJK);
            newVD[newIJK[0] * newSize[1] * newSize[2] + newIJK[1] * newSize[2] + newIJK[2]] = data;
        }
        delete[] vfpd.voxelData;
        vfpd.voxelData = newVD;

        // Update size for voxel data structure.
        vfpd.sizeX = (size_t)newSize[0];
        vfpd.sizeY = (size_t)newSize[1];
        vfpd.sizeZ = (size_t)newSize[2];

        // Offset the transform.
        glm_translate(vfpd.transform, vec3{ -(float_t)outOffset[0], -(float_t)outOffset[1], -(float_t)outOffset[2] });
    }

    void shrinkVoxelFieldBoundsAuto(VoxelFieldPhysicsData& vfpd, ivec3& outOffset)
    {
        ivec3 boundsMin = { vfpd.sizeX, vfpd.sizeY, vfpd.sizeZ };
        ivec3 boundsMax = { 0, 0, 0 };
        for (size_t i = 0; i < vfpd.sizeX; i++)
        for (size_t j = 0; j < vfpd.sizeY; j++)
        for (size_t k = 0; k < vfpd.sizeZ; k++)
            if (getVoxelDataAtPosition(vfpd, i, j, k) != 0)
            {
                ivec3 ijk = { i, j, k };
                glm_ivec3_minv(boundsMin, ijk, boundsMin);
                glm_ivec3_maxv(boundsMax, ijk, boundsMax);
            }
        glm_ivec3_mul(boundsMin, ivec3{ -1, -1, -1 }, outOffset);

        // Set the new bounds to the smaller amount.
        ivec3 newSize;
        glm_ivec3_add(boundsMax, ivec3{ 1, 1, 1 }, newSize);
        glm_ivec3_sub(newSize, boundsMin, newSize);

        // @COPYPASTA
        // Create a new data grid with new bounds.
        size_t arraySize = newSize[0] * newSize[1] * newSize[2];
        uint8_t* newVD = new uint8_t[arraySize];
        for (size_t i = 0; i < arraySize; i++)
            newVD[i] = 0;  // Init the value to be empty.

        for (size_t i = 0; i < vfpd.sizeX; i++)
        for (size_t j = 0; j < vfpd.sizeY; j++)
        for (size_t k = 0; k < vfpd.sizeZ; k++)
        {
            uint8_t data = vfpd.voxelData[i * vfpd.sizeY * vfpd.sizeZ + j * vfpd.sizeZ + k];
            if (data == 0)
                continue;  // Skip empty spaces (to also prevent inserting into out of bounds if shrinking the array).

            ivec3 newIJK;
            glm_ivec3_add(ivec3{ (int32_t)i, (int32_t)j, (int32_t)k }, outOffset, newIJK);
            newVD[newIJK[0] * newSize[1] * newSize[2] + newIJK[1] * newSize[2] + newIJK[2]] = data;
        }
        delete[] vfpd.voxelData;
        vfpd.voxelData = newVD;

        // Update size for voxel data structure.
        vfpd.sizeX = (size_t)newSize[0];
        vfpd.sizeY = (size_t)newSize[1];
        vfpd.sizeZ = (size_t)newSize[2];

        // Offset the transform.
        glm_translate(vfpd.transform, vec3{ -(float_t)outOffset[0], -(float_t)outOffset[1], -(float_t)outOffset[2] });

        std::cout << "Shurnk to { " << vfpd.sizeX << ", " << vfpd.sizeY << ", " << vfpd.sizeZ << " }" << std::endl;
    }

    void cookVoxelDataIntoShape(VoxelFieldPhysicsData& vfpd, const std::string& entityGuid, std::vector<VoxelFieldCollisionShape>& outShapes, std::vector<VoxelFieldCollisionShape>& outTriggers)
    {
        BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();

        // Recreate shape from scratch (property/feature of static compound shape).
        if (!vfpd.bodyId.IsInvalid())
        {
            bodyInterface.RemoveBody(vfpd.bodyId);
            bodyInterface.DestroyBody(vfpd.bodyId);
            vfpd.bodyId = BodyID();
        }

        if (!vfpd.nsTriggerBodyId.IsInvalid())
        {
            bodyInterface.RemoveBody(vfpd.nsTriggerBodyId);
            bodyInterface.DestroyBody(vfpd.nsTriggerBodyId);
            vfpd.nsTriggerBodyId = BodyID();
        }

        if (!vfpd.ewTriggerBodyId.IsInvalid())
        {
            bodyInterface.RemoveBody(vfpd.ewTriggerBodyId);
            bodyInterface.DestroyBody(vfpd.ewTriggerBodyId);
            vfpd.ewTriggerBodyId = BodyID();
        }

        // Create shape for each voxel.
        // (Simple greedy algorithm that pushes as far as possible in one dimension, then in another while throwing away portions that don't fit)
        // (Actually..... right now it's not a greedy algorithm and it's just a simple depth first flood that's good enough for now)  -Timo 2023/09/27
        Ref<StaticCompoundShapeSettings> compoundShape = new StaticCompoundShapeSettings;
        Ref<StaticCompoundShapeSettings> compoundNSTriggerShape = new StaticCompoundShapeSettings;
        Ref<StaticCompoundShapeSettings> compoundEWTriggerShape = new StaticCompoundShapeSettings;

        bool* processed = new bool[vfpd.sizeX * vfpd.sizeY * vfpd.sizeZ];  // Init processing datastructure
        for (size_t i = 0; i < vfpd.sizeX * vfpd.sizeY * vfpd.sizeZ; i++)
            processed[i] = false;

        for (size_t i = 0; i < vfpd.sizeX; i++)
        for (size_t j = 0; j < vfpd.sizeY; j++)
        for (size_t k = 0; k < vfpd.sizeZ; k++)
        {
            size_t idx = i * vfpd.sizeY * vfpd.sizeZ + j * vfpd.sizeZ + k;
            if (vfpd.voxelData[idx] == 0 || processed[idx])
                continue;
            
            // Start greedy search.
            if (vfpd.voxelData[idx] == 1 || (6 <= vfpd.voxelData[idx] && vfpd.voxelData[idx] <= 7))
            {
                uint8_t myType = vfpd.voxelData[idx];

                // Filled space search.
                size_t encX = 1,  // Encapsulation sizes. Multiply it all together to get the count of encapsulation.
                    encY = 1,
                    encZ = 1;
                for (size_t x = i + 1; x < vfpd.sizeX; x++)
                {
                    // Test whether next position is viable.
                    size_t idx = x * vfpd.sizeY * vfpd.sizeZ + j * vfpd.sizeZ + k;
                    bool viable = (vfpd.voxelData[idx] == myType && !processed[idx]);
                    if (!viable)
                        break;  // Exit if not viable.
                    
                    encX++; // March forward.
                }
                for (size_t y = j + 1; y < vfpd.sizeY; y++)
                {
                    // Test whether next row of positions are viable.
                    bool viable = true;
                    for (size_t x = i; x < i + encX; x++)
                    {
                        size_t idx = x * vfpd.sizeY * vfpd.sizeZ + y * vfpd.sizeZ + k;
                        viable &= (vfpd.voxelData[idx] == myType && !processed[idx]);
                        if (!viable)
                            break;
                    }

                    if (!viable)
                        break;  // Exit if not viable.
                    
                    encY++; // March forward.
                }
                for (size_t z = k + 1; z < vfpd.sizeZ; z++)
                {
                    // Test whether next sheet of positions are viable.
                    bool viable = true;
                    for (size_t x = i; x < i + encX; x++)
                    for (size_t y = j; y < j + encY; y++)
                    {
                        size_t idx = x * vfpd.sizeY * vfpd.sizeZ + y * vfpd.sizeZ + z;
                        viable &= (vfpd.voxelData[idx] == myType && !processed[idx]);
                        if (!viable)
                            break;
                    }

                    if (!viable)
                        break;  // Exit if not viable.
                    
                    encZ++; // March forward.
                }

                // Mark all claimed as processed.
                for (size_t x = 0; x < encX; x++)
                for (size_t y = 0; y < encY; y++)
                for (size_t z = 0; z < encZ; z++)
                {
                    size_t idx = (x + i) * vfpd.sizeY * vfpd.sizeZ + (y + j) * vfpd.sizeZ + (z + k);
                    processed[idx] = true;
                }

                // Create shape.
                Vec3 extent((float_t)encX * 0.5f, (float_t)encY * 0.5f, (float_t)encZ * 0.5f);
                Vec3 origin((float_t)i + extent.GetX(), (float_t)j + extent.GetY(), (float_t)k + extent.GetZ());
                Quat rotation = Quat::sIdentity();
                if (myType == 1)
                    compoundShape->AddShape(origin, rotation, new BoxShape(extent));
                else if (myType == 6)
                    compoundNSTriggerShape->AddShape(origin, rotation, new BoxShape(extent));
                else if (myType == 7)
                    compoundEWTriggerShape->AddShape(origin, rotation, new BoxShape(extent));


                // Add shape props to `outShapes`.
                VoxelFieldCollisionShape vfcs = {};
                glm_vec3_copy(vec3{ origin.GetX(), origin.GetY(), origin.GetZ() }, vfcs.origin);
                glm_quat_copy(versor{ rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW() }, vfcs.rotation);
                glm_vec3_copy(vec3{ extent.GetX(), extent.GetY(), extent.GetZ() }, vfcs.extent);
                if (myType == 1)
                    outShapes.push_back(vfcs);
                else if (myType == 6 || myType == 7)
                    outTriggers.push_back(vfcs);
            }
            else if (2 <= vfpd.voxelData[idx] && vfpd.voxelData[idx] <= 5)
            {
                uint8_t myType = vfpd.voxelData[idx];
                bool even = (myType == 2 || myType == 4);

                // Slope space search.
                size_t length = 1;   // Amount slope takes to go down 1 level.
                size_t width = 1;    // # spaces wide the same slope pattern goes.
                size_t repeats = 1;  // # times this pattern repeats downward.

                // Get length dimension.
                for (size_t l = (even ? k : i) + 1; l < (even ? vfpd.sizeZ : vfpd.sizeX); l++)
                {
                    // Test whether next position is viable.
                    size_t idx;
                    if (even)
                        idx = i * vfpd.sizeY * vfpd.sizeZ + j * vfpd.sizeZ + l;
                    else
                        idx = l * vfpd.sizeY * vfpd.sizeZ + j * vfpd.sizeZ + k;

                    bool viable = (vfpd.voxelData[idx] == myType && !processed[idx]);
                    if (!viable)
                        break;  // Exit if not viable.
                    
                    length++; // March forward.
                }

                // Get width dimension.
                for (size_t w = (even ? i : k) + 1; w < (even ? vfpd.sizeX : vfpd.sizeZ); w++)
                {
                    // Test whether next row of positions are viable.
                    bool viable = true;
                    for (size_t l = (even ? k : i); l < (even ? k : i) + length; l++)
                    {
                        size_t idx;
                        if (even)
                            idx = w * vfpd.sizeY * vfpd.sizeZ + j * vfpd.sizeZ + l;
                        else
                            idx = l * vfpd.sizeY * vfpd.sizeZ + j * vfpd.sizeZ + w;

                        viable &= (vfpd.voxelData[idx] == myType && !processed[idx]);
                        if (!viable)
                            break;
                    }

                    if (!viable)
                        break;  // Exit if not viable.
                    
                    width++; // March forward.
                }

                // @TODO: get repeats.

                // Mark all claimed as processed.
                for (size_t x = 0; x < (even ? width : length); x++)
                for (size_t z = 0; z < (even ? length : width); z++)
                {
                    size_t idx = (x + i) * vfpd.sizeY * vfpd.sizeZ + j * vfpd.sizeZ + (z + k);
                    processed[idx] = true;
                }

                // Create shape.
                float_t realLength = std::sqrtf(1.0f + (float_t)length * (float_t)length);
                float_t angle      = std::asinf(1.0f / realLength);
                float_t realHeight = std::sinf(90.0f - angle);

                float_t yoff = 0.0f;
                Quat rotation;
                if (myType == 2)
                    rotation = Quat::sEulerAngles(Vec3(-angle, 0.0f, 0.0f));
                else if (myType == 3)
                    rotation = Quat::sEulerAngles(Vec3(0.0f, 0.0f, angle));
                else if (myType == 4)
                {
                    rotation = Quat::sEulerAngles(Vec3(angle, 0.0f, 0.0f));
                    yoff = 1.0f;
                }
                else if (myType == 5)
                {
                    rotation = Quat::sEulerAngles(Vec3(0.0f, 0.0f, -angle));
                    yoff = 1.0f;
                }
                else
                    std::cerr << "[COOKING VOXEL SHAPES]" << std::endl
                        << "WARNING: voxel type " << myType << " was not recognized." << std::endl;
                
                Vec3 extent((float_t)(even ? width : realLength) * 0.5f, (float_t)realHeight * 0.5f, (float_t)(even ? realLength : width) * 0.5f);

                Vec3 origin = Vec3{ (float_t)i, (float_t)j + yoff, (float_t)k } + rotation * (extent + Vec3(0.0f, -realHeight, 0.0f));

                compoundShape->AddShape(origin, rotation, new BoxShape(extent));

                // Add shape props to `outShapes`.
                // @COPYPASTA
                VoxelFieldCollisionShape vfcs = {};
                glm_vec3_copy(vec3{ origin.GetX(), origin.GetY(), origin.GetZ() }, vfcs.origin);
                glm_quat_copy(versor{ rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW() }, vfcs.rotation);
                glm_vec3_copy(vec3{ extent.GetX(), extent.GetY(), extent.GetZ() }, vfcs.extent);
                outShapes.push_back(vfcs);
            }
        }

        // Get body creation transform.
        vec4 pos;
        mat4 rot;
        vec3 sca;
        glm_decompose(vfpd.transform, pos, rot, sca);
        versor rotV;
        glm_mat4_quat(rot, rotV);

        if (compoundShape->mSubShapes.size() > 0)
        {
            // Create collision body.
            // DYNAMIC is set so that voxel field can move around with the influence of other physics objects.
            vfpd.bodyId = bodyInterface.CreateBody(BodyCreationSettings(compoundShape, RVec3(pos[0], pos[1], pos[2]), Quat(rotV[0], rotV[1], rotV[2], rotV[3]), EMotionType::Static, Layers::NON_MOVING))->GetID();
            // bodyInterface.SetGravityFactor(vfpd.bodyId, 0.0f);
            bodyInterface.AddBody(vfpd.bodyId, EActivation::DontActivate);

            // Add guid into references.
            bodyIdToEntityGuidMap[vfpd.bodyId.GetIndex()] = entityGuid;
        }

        if (compoundNSTriggerShape->mSubShapes.size() > 0)
        {
            // Create NS trigger.
            BodyCreationSettings bcs(compoundNSTriggerShape, RVec3(pos[0], pos[1], pos[2]), Quat(rotV[0], rotV[1], rotV[2], rotV[3]), EMotionType::Static, Layers::NON_MOVING);
            bcs.mIsSensor = true;
            bcs.mUserData = (uint64_t)physengine::UserDataMeaning::IS_NS_CAMERA_RAIL_TRIGGER;
            vfpd.nsTriggerBodyId = bodyInterface.CreateBody(bcs)->GetID();
            bodyInterface.AddBody(vfpd.nsTriggerBodyId, EActivation::DontActivate);

            // Add guid into references.
            bodyIdToEntityGuidMap[vfpd.nsTriggerBodyId.GetIndex()] = entityGuid;
        }

        if (compoundEWTriggerShape->mSubShapes.size() > 0)
        {
            // Create NS trigger.
            BodyCreationSettings bcs(compoundEWTriggerShape, RVec3(pos[0], pos[1], pos[2]), Quat(rotV[0], rotV[1], rotV[2], rotV[3]), EMotionType::Static, Layers::NON_MOVING);
            bcs.mIsSensor = true;
            bcs.mUserData = (uint64_t)physengine::UserDataMeaning::IS_EW_CAMERA_RAIL_TRIGGER;
            vfpd.ewTriggerBodyId = bodyInterface.CreateBody(bcs)->GetID();
            bodyInterface.AddBody(vfpd.ewTriggerBodyId, EActivation::DontActivate);
            
            // Add guid into references.
            bodyIdToEntityGuidMap[vfpd.ewTriggerBodyId.GetIndex()] = entityGuid;
        }
    }

    void setVoxelFieldBodyTransform(VoxelFieldPhysicsData& vfpd, vec3 newPosition, versor newRotation)
    {
        setBodyTransform(vfpd.bodyId, newPosition, newRotation);
    }

    void moveVoxelFieldBodyKinematic(VoxelFieldPhysicsData& vfpd, vec3 newPosition, versor newRotation, const float_t& physicsDeltaTime)
    {
        RVec3 newPositionReal(newPosition[0], newPosition[1], newPosition[2]);
        Quat newRotationJolt(newRotation[0], newRotation[1], newRotation[2], newRotation[3]);
        physicsSystem->GetBodyInterface().MoveKinematic(vfpd.bodyId, newPositionReal, newRotationJolt, physicsDeltaTime);
    }

    void setVoxelFieldBodyKinematic(VoxelFieldPhysicsData& vfpd, bool isKinematic)
    {
        physicsSystem->GetBodyInterface().SetMotionType(vfpd.bodyId, (isKinematic ? EMotionType::Kinematic : EMotionType::Dynamic), EActivation::DontActivate);
    }

    //
    // Capsule pool
    //
    CapsulePhysicsData capsulePool[PHYSICS_OBJECTS_MAX_CAPACITY];
    size_t capsuleIndices[PHYSICS_OBJECTS_MAX_CAPACITY];
    size_t numCapsCreated = 0;

    CapsulePhysicsData* createCharacter(const std::string& entityGuid, vec3 position, const float_t& radius, const float_t& height, bool enableCCD)
    {
        if (numCapsCreated < PHYSICS_OBJECTS_MAX_CAPACITY)
        {
            // Pull a capsule from the pool
            size_t index = 0;
            if (numCapsCreated > 0)
            {
                index = (capsuleIndices[numCapsCreated - 1] + 1) % PHYSICS_OBJECTS_MAX_CAPACITY;
                capsuleIndices[numCapsCreated] = index;
            }
            CapsulePhysicsData& cpd = capsulePool[index];
            numCapsCreated++;

            // Insert in the data
            cpd.entityGuid = entityGuid;
            glm_vec3_copy(position, cpd.currentCOMPosition);
            glm_vec3_copy(position, cpd.prevCOMPosition);
            cpd.radius = radius;
            cpd.height = height;

            // Create physics capsule.
            ShapeRefC capsuleShape = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * height + radius, 0), Quat::sIdentity(), new CapsuleShape(0.5f * height, radius)).Create().Get();

            Ref<CharacterSettings> settings = new CharacterSettings;
            settings->mMaxSlopeAngle = glm_rad(45.0f);
            settings->mLayer = Layers::MOVING;
            settings->mShape = capsuleShape;

            // @NOTE: this was in the past 0.0f, but after introducing the slightest slope, the character starts sliding down.
            //        This gives everything a bit of a tacky feel, but I feel like that makes the physics for the characters
            //        feel real (gives character lol). Plus, the characters can hold up to a rotating moving platform.  -Timo 2023/09/30
            settings->mFriction = 0.0f;

            settings->mSupportingVolume = Plane(Vec3::sAxisY(), -(0.5f * height));
            cpd.character = new JPH::Character(settings, RVec3(position[0], position[1], position[2]), Quat::sIdentity(), (int64_t)UserDataMeaning::IS_CHARACTER, physicsSystem);
            if (enableCCD)
                physicsSystem->GetBodyInterface().SetMotionQuality(cpd.character->GetBodyID(), EMotionQuality::LinearCast);
            cpd.character->AddToPhysicsSystem(EActivation::Activate);

            // Add guid into references.
            bodyIdToEntityGuidMap[cpd.character->GetBodyID().GetIndex()] = entityGuid;

            return &cpd;
        }
        else
        {
            std::cerr << "ERROR: capsule creation has reached its limit" << std::endl;
            return nullptr;
        }
    }

    bool destroyCapsule(CapsulePhysicsData* cpd)
    {
        for (size_t i = 0; i < numCapsCreated; i++)
        {
            size_t& index = capsuleIndices[i];
            if (&capsulePool[index] == cpd)
            {
                if (numCapsCreated > 1)
                {
                    // Overwrite the index with the back index,
                    // effectively deleting the index
                    index = capsuleIndices[numCapsCreated - 1];
                }
                numCapsCreated--;

                // Remove and delete the physics capsule.
                cpd->character->RemoveFromPhysicsSystem();

                return true;
            }
        }
        return false;
    }

    size_t getNumCapsules()
    {
        return numCapsCreated;
    }

    CapsulePhysicsData* getCapsuleByIndex(size_t index)
    {
        return &capsulePool[capsuleIndices[index]];
    }

    float_t getLengthOffsetToBase(const CapsulePhysicsData& cpd)
    {
        return cpd.height * 0.5f + cpd.radius - collisionTolerance * 0.5f;
    }

    void setCharacterPosition(CapsulePhysicsData& cpd, vec3 position)
    {
        cpd.character->SetPosition(RVec3(position[0], position[1], position[2]));
    }

    void moveCharacter(CapsulePhysicsData& cpd, vec3 velocity)
    {
        cpd.character->SetLinearVelocity(Vec3(velocity[0], velocity[1], velocity[2]));
    }

    void setGravityFactor(CapsulePhysicsData& cpd, float_t newGravityFactor)
    {
        BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        bodyInterface.SetGravityFactor(cpd.character->GetBodyID(), newGravityFactor);
    }

    void getLinearVelocity(const CapsulePhysicsData& cpd, vec3& outVelocity)
    {
        Vec3 velo = cpd.character->GetLinearVelocity();
        outVelocity[0] = velo.GetX();
        outVelocity[1] = velo.GetY();
        outVelocity[2] = velo.GetZ();
    }

    bool isGrounded(const CapsulePhysicsData& cpd)
    {
        return (cpd.character->GetGroundState() == CharacterBase::EGroundState::OnGround);
    }

    bool isSlopeTooSteepForCharacter(const CapsulePhysicsData& cpd, Vec3Arg normal)
    {
        return cpd.character->IsSlopeTooSteep(normal);
    }

    //
    // Tick
    //
    void tick()
    {
        auto& bodyInterface = physicsSystem->GetBodyInterface();

        // Set previous transform
        for (size_t i = 0; i < numVFsCreated; i++)
        {
            VoxelFieldPhysicsData& vfpd = voxelFieldPool[voxelFieldIndices[i]];
            glm_mat4_copy(vfpd.transform, vfpd.prevTransform);
        }
        for (size_t i = 0; i < numCapsCreated; i++)
        {
            CapsulePhysicsData& cpd = capsulePool[capsuleIndices[i]];
            glm_vec3_copy(cpd.currentCOMPosition, cpd.prevCOMPosition);
        }

        // @NOTE: @FIXME: @INCORRECT: just setting the prev transform and then *sometime* the new transform is gonna come in `tock()` means that at bad times the interpolated transform will be lerping between two COMs that are identical. The evaluated COM in the `tock()` needs to be set in the queue as the next current, then during this `tick()` the positions will get set as the different COMs.  -Timo 2023/11/16
    }

    void tock()
    {
        auto& bodyInterface = physicsSystem->GetBodyInterface();

        // Set current transform.
        for (size_t i = 0; i < numVFsCreated; i++)
        {
            VoxelFieldPhysicsData& vfpd = voxelFieldPool[voxelFieldIndices[i]];
            // vfpd.COMPositionDifferent = false;  // @TODO: implement this!

            if (vfpd.bodyId.IsInvalid() || !bodyInterface.IsActive(vfpd.bodyId))
                continue;

            getWorldTransform(vfpd.bodyId, vfpd.transform);
        }
        for (size_t i = 0; i < numCapsCreated; i++)
        {
            CapsulePhysicsData& cpd = capsulePool[capsuleIndices[i]];
            cpd.COMPositionDifferent = false;

            if (cpd.character == nullptr)
                continue;
            
            cpd.character->PostSimulation(collisionTolerance);

            // Copy to cglm style.
            RVec3 pos = cpd.character->GetCenterOfMassPosition();  // @NOTE: I thought that `GetPosition` would be quicker/lighter than `GetCenterOfMassPosition`, but getting the position negates the center of mass, thus causing an extra subtract operation.
            cpd.currentCOMPosition[0] = pos.GetX();
            cpd.currentCOMPosition[1] = pos.GetY();
            cpd.currentCOMPosition[2] = pos.GetZ();
            //////////////////////
            cpd.COMPositionDifferent = (glm_vec3_distance2(cpd.currentCOMPosition, cpd.prevCOMPosition) > 0.000001f);
        }
    }

#if 0
    //
    // Collision algorithms
    //
    void closestPointToLineSegment(vec3& pt, vec3& a, vec3& b, vec3& outPt)
    {
        // https://arrowinmyknee.com/2021/03/15/some-math-about-capsule-collision/
        vec3 ab;
        glm_vec3_sub(b, a, ab);

        // Project pt onto ab, but deferring divide by Dot(ab, ab)
        vec3 pt_a;
        glm_vec3_sub(pt, a, pt_a);
        float_t t = glm_vec3_dot(pt_a, ab);
        if (t <= 0.0f)
        {
            // pt projects outside the [a,b] interval, on the a side; clamp to a
            t = 0.0f;
            glm_vec3_copy(a, outPt);
        }
        else
        {
            float_t denom = glm_vec3_dot(ab, ab); // Always nonnegative since denom = ||ab||∧2
            if (t >= denom)
            {
                // pt projects outside the [a,b] interval, on the b side; clamp to b
                t = 1.0f;
                glm_vec3_copy(b, outPt);
            }
            else
            {
                // pt projects inside the [a,b] interval; must do deferred divide now
                t = t / denom;
                glm_vec3_scale(ab, t, ab);
                glm_vec3_add(a, ab, outPt);
            }
        }
    }

    bool checkCapsuleCollidingWithVoxelField(VoxelFieldPhysicsData& vfpd, CapsulePhysicsData& cpd, vec3& collisionNormal, float_t& penetrationDepth)
    {
        //
        // Broad phase: turn both objects into AABB and do collision
        //
        auto broadPhaseTimingStart = std::chrono::high_resolution_clock::now();

        vec3 capsulePtATransformed;
        vec3 capsulePtBTransformed;
        mat4 vfpdTransInv;
        glm_mat4_inv(vfpd.transform, vfpdTransInv);
        glm_vec3_copy(cpd.basePosition, capsulePtATransformed);
       COM_vec3_copy(cpd.basePosition, capsulePtBTransformed);
        capsulePtATransformed[1] += cpd.radius + cpd.height;
        capsulePtBTransformed[1] += cpd.radius;
        glm_mat4_mulv3(vfpdTransInv, capsulePtATransformed, 1.0f, capsulePtATransformed);
        glm_mat4_mulv3(vfpdTransInv, capsulePtBTransformed, 1.0f, capsulePtBTransformed);
        vec3 capsuleAABBMinMax[2] = {
            {
                std::min(capsulePtATransformed[0], capsulePtBTransformed[0]) - cpd.radius,  // @NOTE: add/subtract the radius while in voxel field transform space.
                std::min(capsulePtATransformed[1], capsulePtBTransformed[1]) - cpd.radius,
                std::min(capsulePtATransformed[2], capsulePtBTransformed[2]) - cpd.radius
            },
            {
                std::max(capsulePtATransformed[0], capsulePtBTransformed[0]) + cpd.radius,
                std::max(capsulePtATransformed[1], capsulePtBTransformed[1]) + cpd.radius,
                std::max(capsulePtATransformed[2], capsulePtBTransformed[2]) + cpd.radius
            },
        };
        vec3 voxelFieldAABBMinMax[2] = {
            { 0.0f, 0.0f, 0.0f },
            { vfpd.sizeX, vfpd.sizeY, vfpd.sizeZ },
        };
        if (capsuleAABBMinMax[0][0] > voxelFieldAABBMinMax[1][0] ||
            capsuleAABBMinMax[1][0] < voxelFieldAABBMinMax[0][0] ||
            capsuleAABBMinMax[0][1] > voxelFieldAABBMinMax[1][1] ||
            capsuleAABBMinMax[1][1] < voxelFieldAABBMinMax[0][1] ||
            capsuleAABBMinMax[0][2] > voxelFieldAABBMinMax[1][2] ||
            capsuleAABBMinMax[1][2] < voxelFieldAABBMinMax[0][2])
            return false;

        auto broadPhaseTimingDiff = std::chrono::high_resolution_clock::now() - broadPhaseTimingStart;

        //
        // Narrow phase: check all filled voxels within the capsule AABB
        //
        auto narrowPhaseTimingStart = std::chrono::high_resolution_clock::now();
        ivec3 searchMin = {
            std::max(floor(capsuleAABBMinMax[0][0]), voxelFieldAABBMinMax[0][0]),
            std::max(floor(capsuleAABBMinMax[0][1]), voxelFieldAABBMinMax[0][1]),
            std::max(floor(capsuleAABBMinMax[0][2]), voxelFieldAABBMinMax[0][2])
        };
        ivec3 searchMax = {
            std::min(floor(capsuleAABBMinMax[1][0]), voxelFieldAABBMinMax[1][0] - 1),
            std::min(floor(capsuleAABBMinMax[1][1]), voxelFieldAABBMinMax[1][1] - 1),
            std::min(floor(capsuleAABBMinMax[1][2]), voxelFieldAABBMinMax[1][2] - 1)
        };

        bool collisionSuccessful = false;
        float_t lowestDpSqrDist = std::numeric_limits<float_t>::max();
        size_t lkjlkj = 0;
        size_t succs = 0;
        for (size_t i = searchMin[0]; i <= searchMax[0]; i++)
            for (size_t j = searchMin[1]; j <= searchMax[1]; j++)
                for (size_t k = searchMin[2]; k <= searchMax[2]; k++)
                {
                    lkjlkj++;
                    uint8_t vd = vfpd.voxelData[i * vfpd.sizeY * vfpd.sizeZ + j * vfpd.sizeZ + k];

                    switch (vd)
                    {
                        // Empty space
                    case 0:
                        continue;

                        // Filled space
                    case 1:
                    {
                        //
                        // Test collision with this voxel
                        //
                        vec3 voxelCenterPt = { i + 0.5f, j + 0.5f, k + 0.5f };
                        vec3 point;
                        closestPointToLineSegment(voxelCenterPt, capsulePtATransformed, capsulePtBTransformed, point);

                        vec3 boundedPoint;
                        glm_vec3_copy(point, boundedPoint);
                        boundedPoint[0] = glm_clamp(boundedPoint[0], i, i + 1.0f);
                        boundedPoint[1] = glm_clamp(boundedPoint[1], j, j + 1.0f);
                        boundedPoint[2] = glm_clamp(boundedPoint[2], k, k + 1.0f);
                        if (point == boundedPoint)
                        {
                            // Collider is stuck inside
                            collisionNormal[0] = 0.0f;
                            collisionNormal[1] = 1.0f;
                            collisionNormal[2] = 0.0f;
                            penetrationDepth = 1.0f;
                            return true;
                        }
                        else
                        {
                            // Get more accurate point with the bounded point
                            vec3 betterPoint;
                            closestPointToLineSegment(boundedPoint, capsulePtATransformed, capsulePtBTransformed, betterPoint);

                            vec3 deltaPoint;
                            glm_vec3_sub(betterPoint, boundedPoint, deltaPoint);
                            float_t dpSqrDist = glm_vec3_norm2(deltaPoint);
                            if (dpSqrDist < cpd.radius * cpd.radius && dpSqrDist < lowestDpSqrDist)
                            {
                                // Collision successful
                                succs++;
                                collisionSuccessful = true;
                                lowestDpSqrDist = dpSqrDist;
                                glm_normalize(deltaPoint);
                                mat4 transformCopy;
                                glm_mat4_copy(vfpd.transform, transformCopy);
                                glm_mat4_mulv3(transformCopy, deltaPoint, 0.0f, collisionNormal);
                                penetrationDepth = cpd.radius - std::sqrt(dpSqrDist);
                            }
                        }
                    } break;
                    }
                }

        auto narrowPhaseTimingDiff = std::chrono::high_resolution_clock::now() - narrowPhaseTimingStart;
        // std::cout << "collided: checks: " << lkjlkj << "\tsuccs: " << succs << "\ttime (broad): " << broadPhaseTimingDiff  << "\ttime (narrow): " << narrowPhaseTimingDiff << "\tisGround: " << (collisionNormal[1] >= 0.707106665647) << "\tnormal: " << collisionNormal[0] << ", " << collisionNormal[1] << ", " << collisionNormal[2] << "\tdepth: " << penetrationDepth << std::endl;

        return collisionSuccessful;
    }

    bool debugCheckCapsuleColliding(CapsulePhysicsData& cpd, vec3& collisionNormal, float_t& penetrationDepth)
    {
        vec3 normal;
        float_t penDepth;

        for (size_t i = 0; i < numVFsCreated; i++)
        {
            size_t& index = voxelFieldIndices[i];
            if (checkCapsuleCollidingWithVoxelField(voxelFieldPool[index], cpd, normal, penDepth))
            {
                glm_vec3_copy(normal, collisionNormal);
                penetrationDepth = penDepth;
                return true;
            }
        }
        return false;
    }

    void moveCapsuleAccountingForCollision(CapsulePhysicsData& cpd, vec3 deltaPosition, bool stickToGround, vec3& outNormal, float_t ccdDistance)
    {
        glm_vec3_zero(outNormal);  // In case if no collision happens, normal is zero'd!

        do
        {
            // // @NOTE: keep this code here. It works sometimes (if the edge capsule walked up to is flat) and is useful for reference.
            // vec3 originalPosition;
            // glm_vec3_copy(cpd.basePosition, originalPosition);

            vec3 deltaPositionCCD;
            glm_vec3_copy(deltaPosition, deltaPositionCCD);
            if (glm_vec3_norm2(deltaPosition) > ccdDistance * ccdDistance) // Move at a max of the ccdDistance
                glm_vec3_scale_as(deltaPosition, ccdDistance, deltaPositionCCD);
            glm_vec3_sub(deltaPosition, deltaPositionCCD, deltaPosition);

            // Move and check for collision
            glm_vec3_zero(outNormal);
            float_t numNormals = 0.0f;

            glm_vec3_add(cpd.basePosition, deltaPositionCCD, cpd.basePosition);

            for (size_t iterations = 0; iterations < 6; iterations++)
            {
                vec3 normal;
                float_t penetrationDepth;
                bool collided = physengine::debugCheckCapsuleColliding(cpd, normal, penetrationDepth);

                // @NOTE: this was to stick the capsule to the ground for high humps, but caused the capsule
                //        to fly off at twice the speed sometimes bc of nicking the side of the capsule with the
                //        collision resolution. Without sticking (and if voxels are the way), then there is no need
                //        to keep the capsule stuck onto the ground.  -Timo 2023/08/08
                // // Subsequent iterations of collision are just to resolve until sitting in empty space,
                // // so only double check 1st iteration if expecting to stick to the ground.
                // if (iterations == 0 && !collided && stickToGround)
                // {
                //     vec3 oldPosition;
                //     glm_vec3_copy(cpd.basePosition, oldPosition);

                //     cpd.basePosition[1] += -ccdDistance;  // Just push straight down maximum amount to see if collides
                //     collided = physengine::debugCheckCapsuleColliding(cpd, normal, penetrationDepth);
                //     if (!collided)
                //         glm_vec3_copy(oldPosition, cpd.basePosition);  // I guess this empty space was where the capsule was supposed to go to after all!
                // }

                // Resolved into empty space.
                // Do not proceed to do collision resolution.
                if (!collided)
                    break;

                // Collided!
                glm_vec3_add(outNormal, normal, outNormal);
                penetrationDepth += 0.0001f;
                if (normal[1] >= 0.707106781187)  // >=45 degrees
                {
                    // Don't slide on "level-enough" ground
                    cpd.basePosition[1] += penetrationDepth / normal[1];
                }
                else
                {
                    vec3 penetrationDepthV3 = { penetrationDepth, penetrationDepth, penetrationDepth };
                    glm_vec3_muladd(normal, penetrationDepthV3, cpd.basePosition);
                }
            }

            if (numNormals != 0.0f)
                glm_vec3_scale(outNormal, 1.0f / numNormals, outNormal);

            // // Keep capsule from falling off the edge!
            // // @NOTE: keep this code here. It works sometimes (if the edge capsule walked up to is flat) and is useful for reference.
            // if (stickToGround && (glm_vec3_norm2(outNormal) < 0.000001f || outNormal[1] < 0.707106781187))
            // {
            //     if (glm_vec3_norm2(outNormal) > 0.000001f && glm_vec3_norm2(deltaPosition) > 0.000001f)
            //     {
            //         // Redirect rest of ccd movement along the cross of up and the bad normal
            //         vec3 upXBadNormal;
            //         glm_cross(vec3{ 0.0f, 1.0f, 0.0f }, outNormal, upXBadNormal);
            //         glm_normalize(upXBadNormal);

            //         vec3 deltaPositionFlat = {
            //             deltaPosition[0] + deltaPositionCCD[0],
            //             0.0f,
            //             deltaPosition[2] + deltaPositionCCD[2],
            //         };
            //         float_t deltaPositionY = deltaPosition[1] + deltaPositionCCD[1];
            //         float_t deltaPositionFlatLength = glm_vec3_norm(deltaPositionFlat);
            //         glm_normalize(deltaPositionFlat);

            //         float_t slideSca = glm_dot(upXBadNormal, deltaPositionFlat);
            //         glm_vec3_scale(upXBadNormal, slideSca * deltaPositionFlatLength, deltaPosition);
            //         deltaPosition[1] = deltaPositionY;
            //     }


            //     std::cout << "DONT JUMP!\tX: " << outNormal[0] << "\tY: " << outNormal[1] << "\tZ: " << outNormal[2] << std::endl;
            //     outNormal[0] = outNormal[2] = 0.0f;
            //     outNormal[1] = 1.0f;

            //     glm_vec3_copy(originalPosition, cpd.basePosition);
            // }
        } while (glm_vec3_norm2(deltaPosition) > 0.000001f);
    }
#endif

    JPH::BodyID createBoxColliderBody(const std::string& entityGuid, vec3 origin, versor rotation, vec3 extent, bool isTrigger)
    {
        BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        Vec3 extentJ(extent[0], extent[1], extent[2]);
        Vec3 originJ(origin[0], origin[1], origin[2]);
        Quat rotationJ(rotation[0], rotation[1], rotation[2], rotation[3]);

        // @NOTE: if you set a trigger to Kinematic/dynamic and then activate it, it will detect sleeping physics objects. Albeit at a higher cost.
        BoxShape* bs = new BoxShape(extentJ);
        bs->SetDensity(200.0f);
        BodyCreationSettings bcs(bs, originJ, rotationJ, isTrigger ? EMotionType::Kinematic : EMotionType::Dynamic, isTrigger ? Layers::NON_MOVING : Layers::MOVING);
        bcs.mIsSensor = isTrigger;
        if (isTrigger)
        {
            bcs.mAllowSleeping = false;
            bcs.mUserData = (uint64_t)UserDataMeaning::IS_TRIGGER;
        }
        JPH::BodyID bodyId = bodyInterface.CreateBody(bcs)->GetID();
        bodyInterface.AddBody(bodyId, EActivation::Activate);

        // Add guid into references.
        bodyIdToEntityGuidMap[bodyId.GetIndex()] = entityGuid;
        return bodyId;
    }

    void destroyBody(JPH::BodyID bodyId)
    {
        BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        bodyInterface.RemoveBody(bodyId);
        bodyInterface.DestroyBody(bodyId);
    }

    void setBodyTransform(JPH::BodyID bodyId, vec3 origin, versor rotation)
    {
        RVec3 newPositionReal(origin[0], origin[1], origin[2]);
        Quat newRotationJolt(rotation[0], rotation[1], rotation[2], rotation[3]);

        BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
        EActivation activation = EActivation::DontActivate;
        if (bodyInterface.GetMotionType(bodyId) == EMotionType::Dynamic)
            activation = EActivation::Activate;
        bodyInterface.SetPositionAndRotation(bodyId, newPositionReal, newRotationJolt, activation);
    }

    void setPhysicsObjectInterpolation(const float_t& physicsAlpha)
    {
        auto& bodyInterface = physicsSystem->GetBodyInterface();

        //
        // Set interpolated transform
        //
        for (size_t i = 0; i < numVFsCreated; i++)
        {
            VoxelFieldPhysicsData& vfpd = voxelFieldPool[voxelFieldIndices[i]];
            if (vfpd.prevTransform != vfpd.transform)
            {
                vec4   prevPositionV4, positionV4;
                vec3   prevPosition, position;
                mat4   prevRotationM4, rotationM4;
                versor prevRotation, rotation;
                vec3   prevScale, scale;
                glm_decompose(vfpd.prevTransform, prevPositionV4, prevRotationM4, prevScale);
                glm_decompose(vfpd.transform, positionV4, rotationM4, scale);
                glm_vec4_copy3(prevPositionV4, prevPosition);
                glm_vec4_copy3(positionV4, position);
                glm_mat4_quat(prevRotationM4, prevRotation);
                glm_mat4_quat(rotationM4, rotation);

                vec3 interpolPos;
                glm_vec3_lerp(prevPosition, position, physicsAlpha, interpolPos);
                versor interpolRot;
                glm_quat_nlerp(prevRotation, rotation, physicsAlpha, interpolRot);
                vec3 interpolSca;
                glm_vec3_lerp(prevScale, scale, physicsAlpha, interpolSca);

                mat4 transform = GLM_MAT4_IDENTITY_INIT;
                glm_translate(transform, interpolPos);
                glm_quat_rotate(transform, interpolRot, transform);
                glm_scale(transform, interpolSca);
                glm_mat4_copy(transform, vfpd.interpolTransform);
            }
        }
        for (size_t i = 0; i < numCapsCreated; i++)
        {
            CapsulePhysicsData& cpd = capsulePool[capsuleIndices[i]];
            if (cpd.COMPositionDifferent)
                glm_vec3_lerp(cpd.prevCOMPosition, cpd.currentCOMPosition, physicsAlpha, cpd.interpolCOMPosition);
            else
                glm_vec3_copy(cpd.currentCOMPosition, cpd.interpolCOMPosition);
        }
    }

    void setWorldGravity(vec3 newGravity)
    {
        physicsSystem->SetGravity(Vec3(newGravity[0], newGravity[1], newGravity[2]));
    }

    size_t getCollisionLayer(const std::string& layerName)
    {
        return 0;  // @INCOMPLETE: for now, just ignore the collision layers and check everything.
    }

    bool raycastForEntity(vec3 origin, vec3 directionAndMagnitude, std::string& outHitGuid)
    {
        RayCastResult result;
        if (raycast(origin, directionAndMagnitude, SpecifiedBroadPhaseLayerFilter(BroadPhaseLayers::MOVING), SpecifiedObjectLayerFilter(Layers::MOVING), result))
        {
            const uint32_t bodyIdIdx = result.mBodyID.GetIndex();
            if (bodyIdToEntityGuidMap.find(bodyIdIdx) == bodyIdToEntityGuidMap.end())
            {
                std::cout << "[RAYCAST]" << std::endl
                    << "WARNING: body ID " << bodyIdIdx << " didn\'t match any entity GUIDs." << std::endl;
            }
            else
            {
                outHitGuid = bodyIdToEntityGuidMap[bodyIdIdx];
            }
            return true;
        }
        return false;
    }

    bool raycast(vec3 origin, vec3 directionAndMagnitude, SpecifiedBroadPhaseLayerFilter layerFilter, SpecifiedObjectLayerFilter objectFilter, RayCastResult& outResult)
    {
        RRayCast ray{
            Vec3(origin[0], origin[1], origin[2]),
            Vec3(directionAndMagnitude[0], directionAndMagnitude[1], directionAndMagnitude[2])
        };
        bool success = physicsSystem->GetNarrowPhaseQuery().CastRay(ray, outResult, layerFilter, objectFilter);
#ifdef _DEVELOP
        if (engine->generateCollisionDebugVisualization)
        {
            if (success)
                glm_vec3_scale(directionAndMagnitude, outResult.mFraction, directionAndMagnitude);
            vec3 pt2;
            glm_vec3_add(origin, directionAndMagnitude, pt2);
            drawDebugVisLine(origin, pt2);
        }
#endif
        return success;
    }

#if 0
    bool checkLineSegmentIntersectingCapsule(CapsulePhysicsData& cpd, vec3& pt1, vec3& pt2, std::string& outHitGuid)
    {
#ifdef _DEVELOP
        if (engine->generateCollisionDebugVisualization)
            drawDebugVisLine(pt1, pt2);
#endif

        vec3 a_A, a_B;
        glm_vec3_add(cpd.basePosition, vec3{ 0.0f, cpd.radius, 0.0f }, a_A);
        glm_vec3_add(cpd.basePosition, vec3{ 0.0f, cpd.radius + cpd.height, 0.0f }, a_B);

        vec3 v0, v1, v2, v3;
        glm_vec3_sub(pt1, a_A, v0);
        glm_vec3_sub(pt2, a_A, v1);
        glm_vec3_sub(pt1, a_B, v2);
        glm_vec3_sub(pt2, a_B, v3);

        float_t d0 = glm_vec3_norm2(v0);
        float_t d1 = glm_vec3_norm2(v1);
        float_t d2 = glm_vec3_norm2(v2);
        float_t d3 = glm_vec3_norm2(v3);

        vec3 bestA;
        if (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1)
            glm_vec3_copy(a_B, bestA);
        else
            glm_vec3_copy(a_A, bestA);

        vec3 bestB;
        closestPointToLineSegment(bestA, pt1, pt2, bestB);
        closestPointToLineSegment(bestB, a_A, a_B, bestA);

        // Use best points to test collision
        outHitGuid = cpd.entityGuid;
        return (glm_vec3_distance2(bestA, bestB) <= cpd.radius * cpd.radius);
    }

    bool lineSegmentCast(vec3& pt1, vec3& pt2, size_t collisionLayer, bool getAllGuids, std::vector<std::string>& outHitGuids)
    {
        collisionLayer;  // @INCOMPLETE: note that this is unused.
        bool success = false;

        // Check capsules
        for (size_t i = 0; i < numCapsCreated; i++)
        {
            size_t& index = capsuleIndices[i];
            std::string outHitGuid;
            if (checkLineSegmentIntersectingCapsule(capsulePool[index], pt1, pt2, outHitGuid))
            {
                outHitGuids.push_back(outHitGuid);
                success = true;

                if (!getAllGuids)
                    return true;
            }
        }

        // Check Voxel Fields
        for (size_t i = 0; i < numVFsCreated; i++)
        {
            // @INCOMPLETE: just ignore voxel fields for now.
        }

        return success;
    }
#endif

#ifdef _DEVELOP
    void drawDebugVisLine(vec3 pt1, vec3 pt2, DebugVisLineType type)
    {
        DebugVisLine dvl = {};
        glm_vec3_copy(pt1, dvl.pt1);
        glm_vec3_copy(pt2, dvl.pt2);
        dvl.type = type;

        std::lock_guard<std::mutex> lg(mutateDebugVisLines);
        debugVisLines.push_back(dvl);
    }

    void renderImguiPerformanceStats()
    {
        static const float_t perfTimeToMS = 1000.0f / (float_t)SDL_GetPerformanceFrequency();
        ImGui::Text("Physics Times");
        ImGui::Text((std::format("{:.2f}", perfStats.simTimesUS[perfStats.simTimesUSHeadIndex] * perfTimeToMS) + "ms").c_str());
        ImGui::PlotHistogram("##Physics Times Histogram", perfStats.simTimesUS, (int32_t)perfStats.simTimesUSCount, (int32_t)perfStats.simTimesUSHeadIndex, "", 0.0f, perfStats.highestSimTime, ImVec2(256, 24.0f));
        ImGui::SameLine();
        ImGui::Text(("[0, " + std::format("{:.2f}", perfStats.highestSimTime * perfTimeToMS) + "]").c_str());
    }

    void renderDebugVisualization(VkCommandBuffer cmd)
    {
        GPUVisCameraData cd = {};
        glm_mat4_copy(engine->_camera->sceneCamera.gpuCameraData.projectionView, cd.projectionView);

        void* data;
        vmaMapMemory(engine->_allocator, visCameraBuffer._allocation, &data);
        memcpy(data, &cd, sizeof(GPUVisCameraData));
        vmaUnmapMemory(engine->_allocator, visCameraBuffer._allocation);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, debugVisPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, debugVisPipelineLayout, 0, 1, &debugVisDescriptor, 0, nullptr);

        const VkDeviceSize offsets[1] = { 0 };

        if (engine->generateCollisionDebugVisualization)
        {
            // Draw capsules
            vkCmdBindVertexBuffers(cmd, 0, 1, &capsuleVisVertexBuffer._buffer, offsets);
            for (size_t i = 0; i < numCapsCreated; i++)
            {
                CapsulePhysicsData& cpd = capsulePool[capsuleIndices[i]];
                GPUVisInstancePushConst pc = {};
                glm_vec4_copy(vec4{ 0.25f, 1.0f, 0.0f, 1.0f }, pc.color1);
                glm_vec4_copy(pc.color1, pc.color2);
                glm_vec4(cpd.currentCOMPosition, 0.0f, pc.pt1);
                glm_vec4_add(pc.pt1, vec4{ 0.0f, -cpd.height * 0.5f, 0.0f, 0.0f }, pc.pt1);
                glm_vec4(cpd.currentCOMPosition, 0.0f, pc.pt2);
                glm_vec4_add(pc.pt2, vec4{ 0.0f, cpd.height * 0.5f, 0.0f, 0.0f }, pc.pt2);
                pc.capsuleRadius = cpd.radius;
                vkCmdPushConstants(cmd, debugVisPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUVisInstancePushConst), &pc);

                vkCmdDraw(cmd, capsuleVisVertexCount, 1, 0, 0);
            }

            // Draw lines
            // @NOTE: draw all lines all the time, bc `generateCollisionDebugVisualization` controls creation of the lines (when doing a raycast only), not the drawing.
            std::vector<DebugVisLine> visLinesCopy;
            {
                // Copy debug vis lines so locking time is minimal.
                std::lock_guard<std::mutex> lg(mutateDebugVisLines);
                visLinesCopy = debugVisLines;
            }
            vkCmdBindVertexBuffers(cmd, 0, 1, &lineVisVertexBuffer._buffer, offsets);
            for (DebugVisLine& dvl : visLinesCopy)
            {
                GPUVisInstancePushConst pc = {};
                switch (dvl.type)
                {
                    case PURPTEAL:
                        glm_vec4_copy(vec4{ 0.75f, 0.0f, 1.0f, 1.0f }, pc.color1);
                        glm_vec4_copy(vec4{ 0.0f, 0.75f, 1.0f, 1.0f }, pc.color2);
                        break;

                    case AUDACITY:
                        glm_vec4_copy(vec4{ 0.0f, 0.1f, 0.5f, 1.0f }, pc.color1);
                        glm_vec4_copy(vec4{ 0.0f, 0.25f, 1.0f, 1.0f }, pc.color2);
                        break;

                    case SUCCESS:
                        glm_vec4_copy(vec4{ 0.1f, 0.1f, 0.1f, 1.0f }, pc.color1);
                        glm_vec4_copy(vec4{ 0.0f, 1.0f, 0.7f, 1.0f }, pc.color2);
                        break;

                    case VELOCITY:
                        glm_vec4_copy(vec4{ 0.75f, 0.2f, 0.1f, 1.0f }, pc.color1);
                        glm_vec4_copy(vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, pc.color2);
                        break;

                    case KIKKOARMY:
                        glm_vec4_copy(vec4{ 0.0f, 0.0f, 0.0f, 1.0f }, pc.color1);
                        glm_vec4_copy(vec4{ 0.0f, 0.25f, 0.0f, 1.0f }, pc.color2);
                        break;

                    case YUUJUUFUDAN:
                        glm_vec4_copy(vec4{ 0.69f, 0.69f, 0.69f, 1.0f }, pc.color1);
                        glm_vec4_copy(vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, pc.color2);
                        break;
                }
                glm_vec4(dvl.pt1, 0.0f, pc.pt1);
                glm_vec4(dvl.pt2, 0.0f, pc.pt2);
                vkCmdPushConstants(cmd, debugVisPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUVisInstancePushConst), &pc);

                vkCmdDraw(cmd, lineVisVertexCount, 1, 0, 0);
            }
        }
    }
#endif
}
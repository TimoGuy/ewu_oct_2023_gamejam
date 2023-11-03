#pragma once

#include <string>
#include <vector>
#include <functional>
#include <vulkan/vulkan.h>
#include "VkDataStructures.h"
#include "ImportGLM.h"

class VulkanEngine;
struct DeletionQueue;


namespace ui
{
    struct UIQuad
    {
        bool visible     = true;
        Texture* texture = nullptr;
        vec4 tint        = { 1.0f, 1.0f, 1.0f, 1.0f };
        mat4 transform   = GLM_MAT4_IDENTITY_INIT;
        float_t renderOrder = 0.0f;
        VkDescriptorSet builtTextureSet;
    };

    void init(VulkanEngine* engine);
    void initPipeline(VkViewport& screenspaceViewport, VkRect2D& screenspaceScissor, DeletionQueue& deletionQueue);
    void cleanup();

    UIQuad* registerUIQuad(Texture* texture);
    void unregisterUIQuad(UIQuad* toDelete);
    void renderUIQuads(VkCommandBuffer cmd);
}
#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstddef>
#include <vector>
#include <glm/glm.hpp>

namespace graphics {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    void computeVertexNormals(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

    uint32_t buildConeVertices(const glm::vec3& apex,
        const glm::vec3& direction,
        float outer_angle_deg,
        float distance,
        Vertex* out);

    struct GpuBuffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        void* mapped = nullptr;
        size_t size = 0;
    };

    GpuBuffer createBuffer(size_t size, VkBufferUsageFlags usage);

    void destroyBuffer(GpuBuffer& buf);

    template<typename T>
    GpuBuffer createUniformBuffer() {
        return createBuffer(sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    template<typename T>
    GpuBuffer createStorageBuffer(uint32_t count) {
        return createBuffer(sizeof(T) * count, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }

    GpuBuffer createVertexBuffer(size_t size);
    GpuBuffer createIndexBuffer(size_t size);

    VkShaderModule loadShader(const char* path);

    VkDescriptorSetLayout createDescriptorSetLayout(
        const std::vector<VkDescriptorSetLayoutBinding>& bindings);

    VkDescriptorPool createDescriptorPool(
        const std::vector<VkDescriptorPoolSize>& sizes,
        uint32_t maxSets);

    VkDescriptorSet allocateDescriptorSet(
        VkDescriptorPool pool,
        VkDescriptorSetLayout layout);

    void allocateDescriptorSets(
        VkDescriptorPool pool,
        const VkDescriptorSetLayout* layouts,
        uint32_t count,
        VkDescriptorSet* out_sets);

    void writeUniformBuffer(
        VkDescriptorSet set,
        VkBuffer buffer,
        size_t size,
        uint32_t binding = 0);

    void writeStorageBuffer(
        VkDescriptorSet set,
        VkBuffer buffer,
        uint32_t binding);

    struct PipelineConfig {
        VkShaderModule vertexShader = VK_NULL_HANDLE;
        VkShaderModule fragmentShader = VK_NULL_HANDLE;

        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;

        bool depthTest = true;
        bool depthWrite = true;

        const VkVertexInputBindingDescription* vertexBindings = nullptr;
        uint32_t vertexBindingCount = 0;
        const VkVertexInputAttributeDescription* vertexAttributes = nullptr;
        uint32_t vertexAttributeCount = 0;

        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        uint32_t subpass = 0;
    };

    VkPipeline createGraphicsPipeline(const PipelineConfig& config);

    GpuBuffer createDeviceBuffer(size_t size, VkBufferUsageFlags usage);

    GpuBuffer createBufferWithData(const void* data, size_t size, VkBufferUsageFlags usage);

    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

    void writeDynamicUniformBuffer(VkDescriptorSet set, VkBuffer buffer, size_t size, uint32_t binding = 0);

    struct GpuTexture {
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
    };

    GpuTexture loadTexture(const char* path);

    void destroyTexture(GpuTexture& tex);
} // namespace graphics
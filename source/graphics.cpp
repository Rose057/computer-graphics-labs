#include "graphics.hpp"
#include "graphics_internal.hpp"

#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>
#include "lodepng.h"

namespace graphics {
    GpuBuffer createBuffer(size_t size, VkBufferUsageFlags usage) {
        GpuBuffer result;
        result.size = size;

        const VkBufferCreateInfo buffer_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        const VmaAllocationCreateInfo alloc_info = {
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
                   | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };

        VmaAllocationInfo allocation_info{};

        if (vmaCreateBuffer(internal::context.allocator,
            &buffer_info, &alloc_info,
            &result.buffer, &result.allocation,
            &allocation_info) != VK_SUCCESS) {
            std::cerr << "Failed to create Vulkan buffer\n";
            return {};
        }

        result.mapped = allocation_info.pMappedData;
        return result;
    }

    void destroyBuffer(GpuBuffer& buf) {
        if (buf.buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(internal::context.allocator, buf.buffer, buf.allocation);
            buf = {};
        }
    }

    GpuBuffer createVertexBuffer(size_t size) {
        return createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }

    GpuBuffer createIndexBuffer(size_t size) {
        return createBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }

    VkShaderModule loadShader(const char* path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            std::cerr << "Failed to open shader file: " << path << '\n';
            return VK_NULL_HANDLE;
        }

        const size_t size = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(size);

        file.seekg(0);
        file.read(buffer.data(), size);
        file.close();

        const VkShaderModuleCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = size,
            .pCode = reinterpret_cast<const uint32_t*>(buffer.data()),
        };

        VkShaderModule result = VK_NULL_HANDLE;
        if (vkCreateShaderModule(internal::context.device,
            &info, nullptr, &result) != VK_SUCCESS) {
            std::cerr << "Failed to create shader module: " << path << '\n';
            return VK_NULL_HANDLE;
        }
        return result;
    }

    VkDescriptorSetLayout createDescriptorSetLayout(
        const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
        const VkDescriptorSetLayoutCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
        };

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (vkCreateDescriptorSetLayout(internal::context.device, &info, nullptr, &layout) != VK_SUCCESS) {
            std::cerr << "Failed to create descriptor set layout\n";
            return VK_NULL_HANDLE;
        }
        return layout;
    }

    VkDescriptorPool createDescriptorPool(
        const std::vector<VkDescriptorPoolSize>& sizes,
        uint32_t maxSets) {
        const VkDescriptorPoolCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = maxSets,
            .poolSizeCount = static_cast<uint32_t>(sizes.size()),
            .pPoolSizes = sizes.data(),
        };

        VkDescriptorPool pool = VK_NULL_HANDLE;
        if (vkCreateDescriptorPool(internal::context.device, &info, nullptr, &pool) != VK_SUCCESS) {
            std::cerr << "Failed to create descriptor pool\n";
            return VK_NULL_HANDLE;
        }
        return pool;
    }

    VkDescriptorSet allocateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout) {
        const VkDescriptorSetAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &layout,
        };

        VkDescriptorSet set = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(internal::context.device, &info, &set) != VK_SUCCESS) {
            std::cerr << "Failed to allocate descriptor set\n";
            return VK_NULL_HANDLE;
        }
        return set;
    }

    void allocateDescriptorSets(
        VkDescriptorPool pool,
        const VkDescriptorSetLayout* layouts,
        uint32_t count,
        VkDescriptorSet* out_sets) {
        const VkDescriptorSetAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = pool,
            .descriptorSetCount = count,
            .pSetLayouts = layouts,
        };

        if (vkAllocateDescriptorSets(internal::context.device, &info, out_sets) != VK_SUCCESS) {
            std::cerr << "Failed to allocate descriptor sets\n";
        }
    }

    void writeUniformBuffer(VkDescriptorSet set, VkBuffer buffer, size_t size, uint32_t binding) {
        const VkDescriptorBufferInfo buffer_info = {
            .buffer = buffer,
            .offset = 0,
            .range = size,
        };

        const VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &buffer_info,
        };

        vkUpdateDescriptorSets(internal::context.device, 1, &write, 0, nullptr);
    }

    void writeStorageBuffer(VkDescriptorSet set, VkBuffer buffer, uint32_t binding) {
        const VkDescriptorBufferInfo buffer_info = {
            .buffer = buffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE,
        };

        const VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &buffer_info,
        };

        vkUpdateDescriptorSets(internal::context.device, 1, &write, 0, nullptr);
    }

    VkPipeline createGraphicsPipeline(const PipelineConfig& config) {
        const VkPipelineShaderStageCreateInfo stages[] = {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = config.vertexShader,
                .pName = "main",
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = config.fragmentShader,
                .pName = "main",
            },
        };

        const VkPipelineVertexInputStateCreateInfo vertex_input = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = config.vertexBindingCount,
            .pVertexBindingDescriptions = config.vertexBindings,
            .vertexAttributeDescriptionCount = config.vertexAttributeCount,
            .pVertexAttributeDescriptions = config.vertexAttributes,
        };

        const VkPipelineInputAssemblyStateCreateInfo input_assembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = config.topology,
            .primitiveRestartEnable = VK_FALSE,
        };

        const VkPipelineViewportStateCreateInfo viewport_state = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1,
        };

        const VkPipelineRasterizationStateCreateInfo rasterization = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = config.polygonMode,
            .cullMode = config.cullMode,
            .frontFace = config.frontFace,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f,
        };

        const VkPipelineMultisampleStateCreateInfo multisample = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
        };

        const VkPipelineDepthStencilStateCreateInfo depth_stencil = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = config.depthTest ? VK_TRUE : VK_FALSE,
            .depthWriteEnable = config.depthWrite ? VK_TRUE : VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
        };

        const VkPipelineColorBlendAttachmentState color_attachment = {
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                            | VK_COLOR_COMPONENT_G_BIT
                            | VK_COLOR_COMPONENT_B_BIT
                            | VK_COLOR_COMPONENT_A_BIT,
        };

        const VkPipelineColorBlendStateCreateInfo color_blend = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments = &color_attachment,
        };

        const VkDynamicState dynamic_states[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        const VkPipelineDynamicStateCreateInfo dynamic_state = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = dynamic_states,
        };

        const VkGraphicsPipelineCreateInfo pipeline_info = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = stages,
            .pVertexInputState = &vertex_input,
            .pInputAssemblyState = &input_assembly,
            .pViewportState = &viewport_state,
            .pRasterizationState = &rasterization,
            .pMultisampleState = &multisample,
            .pDepthStencilState = &depth_stencil,
            .pColorBlendState = &color_blend,
            .pDynamicState = &dynamic_state,
            .layout = config.layout,
            .renderPass = config.renderPass,
            .subpass = config.subpass,
        };

        VkPipeline pipeline = VK_NULL_HANDLE;
        if (vkCreateGraphicsPipelines(internal::context.device,
            VK_NULL_HANDLE, 1, &pipeline_info,
            nullptr, &pipeline) != VK_SUCCESS) {
            std::cerr << "Failed to create graphics pipeline\n";
            return VK_NULL_HANDLE;
        }
        return pipeline;
    }
    GpuBuffer createDeviceBuffer(size_t size, VkBufferUsageFlags usage) {
        GpuBuffer result;
        result.size = size;

        const VkBufferCreateInfo buffer_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        const VmaAllocationCreateInfo alloc_info = {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        };

        VmaAllocationInfo allocation_info{};

        if (vmaCreateBuffer(internal::context.allocator,
            &buffer_info, &alloc_info,
            &result.buffer, &result.allocation,
            &allocation_info) != VK_SUCCESS) {
            std::cerr << "Failed to create device-local buffer\n";
            return {};
        }

        return result;
    }

    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
        const VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = internal::context.graphics_queue_index,
        };

        VkCommandPool pool = VK_NULL_HANDLE;
        if (vkCreateCommandPool(internal::context.device, &pool_info, nullptr, &pool) != VK_SUCCESS) {
            std::cerr << "Failed to create one-shot command pool\n";
            return;
        }

        const VkCommandBufferAllocateInfo cmd_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer cmd = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(internal::context.device, &cmd_info, &cmd) != VK_SUCCESS) {
            std::cerr << "Failed to allocate one-shot command buffer\n";
            vkDestroyCommandPool(internal::context.device, pool, nullptr);
            return;
        }

        const VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(cmd, &begin_info);

        const VkBufferCopy region = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size,
        };
        vkCmdCopyBuffer(cmd, src, dst, 1, &region);

        vkEndCommandBuffer(cmd);

        const VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        };
        VkFence fence = VK_NULL_HANDLE;
        if (vkCreateFence(internal::context.device, &fence_info, nullptr, &fence) != VK_SUCCESS) {
            std::cerr << "Failed to create fence\n";
            vkDestroyCommandPool(internal::context.device, pool, nullptr);
            return;
        }

        const VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
        };

        vkQueueSubmit(internal::context.graphics_queue, 1, &submit_info, fence);
        vkWaitForFences(internal::context.device, 1, &fence, VK_TRUE, UINT64_MAX);

        vkDestroyFence(internal::context.device, fence, nullptr);
        vkDestroyCommandPool(internal::context.device, pool, nullptr);
    }

    GpuBuffer createBufferWithData(const void* data, size_t size, VkBufferUsageFlags usage) {
        GpuBuffer staging = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        if (staging.buffer == VK_NULL_HANDLE) return {};
        std::memcpy(staging.mapped, data, size);

        GpuBuffer result = createDeviceBuffer(size, usage);
        if (result.buffer == VK_NULL_HANDLE) {
            destroyBuffer(staging);
            return {};
        }

        copyBuffer(staging.buffer, result.buffer, size);

        destroyBuffer(staging);

        return result;
    }

    void writeDynamicUniformBuffer(VkDescriptorSet set, VkBuffer buffer, size_t size, uint32_t binding) {
        const VkDescriptorBufferInfo buffer_info = {
            .buffer = buffer,
            .offset = 0,
            .range = size,
        };

        const VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .pBufferInfo = &buffer_info,
        };

        vkUpdateDescriptorSets(internal::context.device, 1, &write, 0, nullptr);
    }

    GpuTexture loadTexture(const char* path) {
        GpuTexture result;

        std::vector<uint8_t> pixels;
        unsigned width = 0, height = 0;
        if (lodepng::decode(pixels, width, height, path) != 0) {
            std::cerr << "Failed to decode PNG: " << path << '\n';
            return {};
        }

        const VkDeviceSize image_size = width * height * 4;

        GpuBuffer staging = createBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        if (staging.buffer == VK_NULL_HANDLE) return {};
        std::memcpy(staging.mapped, pixels.data(), image_size);

        const VkImageCreateInfo image_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .extent = { width, height, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        const VmaAllocationCreateInfo alloc_info = {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        };

        VmaAllocationInfo allocation_info{};

        if (vmaCreateImage(internal::context.allocator,
            &image_info, &alloc_info,
            &result.image, &result.allocation,
            &allocation_info) != VK_SUCCESS) {
            std::cerr << "Failed to create texture image\n";
            destroyBuffer(staging);
            return {};
        }

        const VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = internal::context.graphics_queue_index,
        };
        VkCommandPool pool = VK_NULL_HANDLE;
        vkCreateCommandPool(internal::context.device, &pool_info, nullptr, &pool);

        const VkCommandBufferAllocateInfo cmd_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(internal::context.device, &cmd_info, &cmd);

        const VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(cmd, &begin_info);

        const VkImageMemoryBarrier to_transfer = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = result.image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &to_transfer);

        const VkBufferImageCopy region = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { width, height, 1 },
        };
        vkCmdCopyBufferToImage(cmd, staging.buffer, result.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        const VkImageMemoryBarrier to_shader = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = result.image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &to_shader);

        vkEndCommandBuffer(cmd);

        const VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        };
        VkFence fence = VK_NULL_HANDLE;
        vkCreateFence(internal::context.device, &fence_info, nullptr, &fence);

        const VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
        };
        vkQueueSubmit(internal::context.graphics_queue, 1, &submit_info, fence);
        vkWaitForFences(internal::context.device, 1, &fence, VK_TRUE, UINT64_MAX);

        vkDestroyFence(internal::context.device, fence, nullptr);
        vkDestroyCommandPool(internal::context.device, pool, nullptr);

        destroyBuffer(staging);

        const VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = result.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        if (vkCreateImageView(internal::context.device, &view_info, nullptr, &result.view) != VK_SUCCESS) {
            std::cerr << "Failed to create image view\n";
            vmaDestroyImage(internal::context.allocator, result.image, result.allocation);
            return {};
        }

        const VkSamplerCreateInfo sampler_info = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 1.0f,
            .compareEnable = VK_FALSE,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };
        if (vkCreateSampler(internal::context.device, &sampler_info, nullptr, &result.sampler) != VK_SUCCESS) {
            std::cerr << "Failed to create sampler\n";
            vkDestroyImageView(internal::context.device, result.view, nullptr);
            vmaDestroyImage(internal::context.allocator, result.image, result.allocation);
            return {};
        }

        return result;
    }

    void destroyTexture(GpuTexture& tex) {
        if (tex.sampler != VK_NULL_HANDLE) {
            vkDestroySampler(internal::context.device, tex.sampler, nullptr);
        }
        if (tex.view != VK_NULL_HANDLE) {
            vkDestroyImageView(internal::context.device, tex.view, nullptr);
        }
        if (tex.image != VK_NULL_HANDLE) {
            vmaDestroyImage(internal::context.allocator, tex.image, tex.allocation);
        }
        tex = {};
    }
} // namespace graphics
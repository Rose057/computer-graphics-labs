#include "graphics.hpp"
#include "graphics_internal.hpp"

#include <fstream>
#include <vector>
#include <iostream>

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
} // namespace graphics
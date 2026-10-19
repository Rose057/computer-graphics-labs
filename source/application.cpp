#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "application.hpp"
#include "graphics.hpp"

#include <cstring>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>

#include <vk_mem_alloc.h>

#include <fstream>
#include <vector>

#include <cmath>

namespace application {

    struct SceneUniforms {
        glm::mat4 view_projection;

        glm::vec3 camera_position;
        float _padding0;

        glm::vec3 directional_light_direction;
        float _padding1;

        glm::vec3 directional_light_color;
        float directional_light_intensity;

        glm::vec3 ambient_light;
        float _padding2;

        uint32_t point_light_count;
        uint32_t spot_light_count;
        float _padding3;
        float _padding4;
    };

    struct PointLight {
        glm::vec3 position;
        float _padding0;
        glm::vec3 color;
        float intensity;
    };

    struct SpotLight {
        glm::vec3 position;
        float _padding0;
        glm::vec3 direction;
        float _padding1;
        glm::vec3 color;
        float intensity;
        float inner_cosine;
        float outer_cosine;
        float inner_angle_deg;
        float outer_angle_deg;
    };

    struct ModelUniform {
        glm::mat4 model;
        glm::vec3 albedo;
        float _padding0;
        glm::vec3 specular;
        float alpha;
        float is_gizmo;
        float _padding1;
        float _padding2;
        float _padding3;
    };

    namespace {
        std::vector<Vertex> pyramid_vertices = {
            { {  0.0f,  0.7f,  0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
            { { -0.5f,  0.0f, -0.5f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
            { {  0.5f,  0.0f, -0.5f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
            { {  0.5f,  0.0f,  0.5f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
            { { -0.5f,  0.0f,  0.5f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
        };

        std::vector<uint32_t> pyramid_indices = {
            0, 1, 2,
            0, 2, 3,
            0, 3, 4,
            0, 4, 1,
            1, 3, 2,
            1, 4, 3,
        };

        std::vector<Vertex> cube_vertices = {
            { {-0.5f, -0.5f,  0.5f}, {1,1,1}, { 0,  0,  1} },
            { { 0.5f, -0.5f,  0.5f}, {1,1,1}, { 0,  0,  1} },
            { { 0.5f,  0.5f,  0.5f}, {1,1,1}, { 0,  0,  1} },
            { {-0.5f, -0.5f,  0.5f}, {1,1,1}, { 0,  0,  1} },
            { { 0.5f,  0.5f,  0.5f}, {1,1,1}, { 0,  0,  1} },
            { {-0.5f,  0.5f,  0.5f}, {1,1,1}, { 0,  0,  1} },

            { { 0.5f, -0.5f, -0.5f}, {1,1,1}, { 0,  0, -1} },
            { {-0.5f, -0.5f, -0.5f}, {1,1,1}, { 0,  0, -1} },
            { {-0.5f,  0.5f, -0.5f}, {1,1,1}, { 0,  0, -1} },
            { { 0.5f, -0.5f, -0.5f}, {1,1,1}, { 0,  0, -1} },
            { {-0.5f,  0.5f, -0.5f}, {1,1,1}, { 0,  0, -1} },
            { { 0.5f,  0.5f, -0.5f}, {1,1,1}, { 0,  0, -1} },

            { {-0.5f, -0.5f, -0.5f}, {1,1,1}, {-1,  0,  0} },
            { {-0.5f, -0.5f,  0.5f}, {1,1,1}, {-1,  0,  0} },
            { {-0.5f,  0.5f,  0.5f}, {1,1,1}, {-1,  0,  0} },
            { {-0.5f, -0.5f, -0.5f}, {1,1,1}, {-1,  0,  0} },
            { {-0.5f,  0.5f,  0.5f}, {1,1,1}, {-1,  0,  0} },
            { {-0.5f,  0.5f, -0.5f}, {1,1,1}, {-1,  0,  0} },

            { { 0.5f, -0.5f,  0.5f}, {1,1,1}, { 1,  0,  0} },
            { { 0.5f, -0.5f, -0.5f}, {1,1,1}, { 1,  0,  0} },
            { { 0.5f,  0.5f, -0.5f}, {1,1,1}, { 1,  0,  0} },
            { { 0.5f, -0.5f,  0.5f}, {1,1,1}, { 1,  0,  0} },
            { { 0.5f,  0.5f, -0.5f}, {1,1,1}, { 1,  0,  0} },
            { { 0.5f,  0.5f,  0.5f}, {1,1,1}, { 1,  0,  0} },

            { {-0.5f,  0.5f,  0.5f}, {1,1,1}, { 0,  1,  0} },
            { { 0.5f,  0.5f,  0.5f}, {1,1,1}, { 0,  1,  0} },
            { { 0.5f,  0.5f, -0.5f}, {1,1,1}, { 0,  1,  0} },
            { {-0.5f,  0.5f,  0.5f}, {1,1,1}, { 0,  1,  0} },
            { { 0.5f,  0.5f, -0.5f}, {1,1,1}, { 0,  1,  0} },
            { {-0.5f,  0.5f, -0.5f}, {1,1,1}, { 0,  1,  0} },

            { {-0.5f, -0.5f, -0.5f}, {1,1,1}, { 0, -1,  0} },
            { { 0.5f, -0.5f, -0.5f}, {1,1,1}, { 0, -1,  0} },
            { { 0.5f, -0.5f,  0.5f}, {1,1,1}, { 0, -1,  0} },
            { {-0.5f, -0.5f, -0.5f}, {1,1,1}, { 0, -1,  0} },
            { { 0.5f, -0.5f,  0.5f}, {1,1,1}, { 0, -1,  0} },
            { {-0.5f, -0.5f,  0.5f}, {1,1,1}, { 0, -1,  0} },
        };

        std::vector<uint32_t> cube_indices = {
            0, 1, 2, 3, 4, 5,
            6, 7, 8, 9, 10, 11,
            12, 13, 14, 15, 16, 17,
            18, 19, 20, 21, 22, 23,
            24, 25, 26, 27, 28, 29,
            30, 31, 32, 33, 34, 35,
        };

        constexpr uint32_t object_count = 3;

        constexpr uint32_t max_cone_vertices = 512;

        bool show_gizmos = true;

        constexpr uint32_t max_point_lights = 8;
        uint32_t point_light_count = 0;
        PointLight cpu_point_lights[max_point_lights];
        PointLight* vk_point_lights_buffer_mapped = nullptr;

        constexpr uint32_t max_spot_lights = 4;
        uint32_t spot_light_count = 0;
        SpotLight cpu_spot_lights[max_spot_lights];
        SpotLight* vk_spot_lights_buffer_mapped = nullptr;

        graphics::GpuBuffer pyramid_vb;
        graphics::GpuBuffer pyramid_ib;

        graphics::GpuBuffer cone_vb;
        graphics::GpuBuffer cone_ub;
        ModelUniform* vk_cone_uniform_buffer_mapped = nullptr;
        Vertex* cone_vertices_mapped = nullptr;

        graphics::GpuBuffer cube_vb;
        graphics::GpuBuffer cube_ib;

        graphics::GpuBuffer scene_ub;
        SceneUniforms* vk_scene_uniform_buffer_mapped = nullptr;

        graphics::GpuBuffer model_ub[object_count];
        ModelUniform* vk_model_uniform_buffers_mapped[object_count] = {};

        graphics::GpuBuffer point_gizmo_ub[max_point_lights];
        ModelUniform* vk_point_gizmo_uniform_buffers_mapped[max_point_lights] = {};

        graphics::GpuBuffer spot_gizmo_ub[max_spot_lights];
        ModelUniform* vk_spot_gizmo_uniform_buffers_mapped[max_spot_lights] = {};

        graphics::GpuBuffer point_lights_sb;
        graphics::GpuBuffer spot_lights_sb;

        void computeVertexNormals(std::vector<Vertex>& vertices,
            std::vector<uint32_t>& indices) {

            const std::vector<Vertex> original_vertices = vertices;
            const std::vector<uint32_t> original_indices = indices;

            vertices.clear();
            indices.clear();

            for (size_t i = 0; i < original_indices.size(); i += 3) {
                const uint32_t i0 = original_indices[i];
                const uint32_t i1 = original_indices[i + 1];
                const uint32_t i2 = original_indices[i + 2];

                const Vertex& v0 = original_vertices[i0];
                const Vertex& v1 = original_vertices[i1];
                const Vertex& v2 = original_vertices[i2];

                const glm::vec3 edge1 = v2.position - v0.position;
                const glm::vec3 edge2 = v1.position - v0.position;
                const glm::vec3 faceNormal =
                    glm::normalize(glm::cross(edge1, edge2));

                const uint32_t firstIndex =
                    static_cast<uint32_t>(vertices.size());

                Vertex newV0 = v0;
                Vertex newV1 = v1;
                Vertex newV2 = v2;

                newV0.normal = faceNormal;
                newV1.normal = faceNormal;
                newV2.normal = faceNormal;

                vertices.push_back(newV0);
                vertices.push_back(newV1);
                vertices.push_back(newV2);

                indices.push_back(firstIndex);
                indices.push_back(firstIndex + 1);
                indices.push_back(firstIndex + 2);
            }
        }

        bool createPyramidBuffers() {
            pyramid_vb = graphics::createVertexBuffer(pyramid_vertices.size() * sizeof(Vertex));
            if (pyramid_vb.buffer == VK_NULL_HANDLE) return false;
            std::memcpy(pyramid_vb.mapped, pyramid_vertices.data(), pyramid_vb.size);

            pyramid_ib = graphics::createIndexBuffer(pyramid_indices.size() * sizeof(uint32_t));
            if (pyramid_ib.buffer == VK_NULL_HANDLE) return false;
            std::memcpy(pyramid_ib.mapped, pyramid_indices.data(), pyramid_ib.size);

            return true;
        }


        uint32_t buildConeVertices(const SpotLight& light, float distance, Vertex* out) {
            glm::vec3 dir = glm::normalize(light.direction);

            glm::vec3 up = (std::abs(dir.y) < 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
            glm::vec3 right = glm::normalize(glm::cross(dir, up));
            glm::vec3 realUp = glm::normalize(glm::cross(right, dir));

            glm::vec3 apex = light.position;

            glm::vec3 ring_center = apex + dir * distance;
            float ring_radius = distance * std::tan(glm::radians(light.outer_angle_deg));

            const uint32_t segments = 8;
            uint32_t vertex_count = 0;

            glm::vec3 prev_point{};

            for (uint32_t i = 0; i <= segments; ++i) {
                float angle = 2.0f * 3.14159265f * float(i) / float(segments);
                glm::vec3 offset = right * std::cos(angle) * ring_radius
                    + realUp * std::sin(angle) * ring_radius;
                glm::vec3 point = ring_center + offset;

                out[vertex_count++] = { apex,  {1, 1, 1}, {0, 0, 1} };
                out[vertex_count++] = { point, {1, 1, 1}, {0, 0, 1} };

                if (i > 0) {
                    out[vertex_count++] = { prev_point, {1, 1, 1}, {0, 0, 1} };
                    out[vertex_count++] = { point,      {1, 1, 1}, {0, 0, 1} };
                }

                prev_point = point;
            }

            return vertex_count;
        }

        bool createConeBuffer() {
            cone_vb = graphics::createVertexBuffer(sizeof(Vertex) * max_cone_vertices);
            if (cone_vb.buffer == VK_NULL_HANDLE) return false;

            cone_vertices_mapped = static_cast<Vertex*>(cone_vb.mapped);
            return true;
        }

        bool createCubeBuffers() {
            cube_vb = graphics::createVertexBuffer(cube_vertices.size() * sizeof(Vertex));
            if (cube_vb.buffer == VK_NULL_HANDLE) return false;
            std::memcpy(cube_vb.mapped, cube_vertices.data(), cube_vb.size);

            cube_ib = graphics::createIndexBuffer(cube_indices.size() * sizeof(uint32_t));
            if (cube_ib.buffer == VK_NULL_HANDLE) return false;
            std::memcpy(cube_ib.mapped, cube_indices.data(), cube_ib.size);

            return true;
        }

        bool createUniformBuffers() {
            scene_ub = graphics::createUniformBuffer<SceneUniforms>();
            if (scene_ub.buffer == VK_NULL_HANDLE) return false;
            vk_scene_uniform_buffer_mapped = static_cast<SceneUniforms*>(scene_ub.mapped);

            vk_scene_uniform_buffer_mapped->view_projection = glm::mat4(1.0f);
            vk_scene_uniform_buffer_mapped->camera_position = glm::vec3(0.0f);
            vk_scene_uniform_buffer_mapped->_padding0 = 0.0f;
            vk_scene_uniform_buffer_mapped->directional_light_direction = glm::vec3(0.0f, -1.0f, -0.3f);
            vk_scene_uniform_buffer_mapped->_padding1 = 0.0f;
            vk_scene_uniform_buffer_mapped->directional_light_color = glm::vec3(1.0f);
            vk_scene_uniform_buffer_mapped->directional_light_intensity = 1.0f;
            vk_scene_uniform_buffer_mapped->ambient_light = glm::vec3(0.1f);
            vk_scene_uniform_buffer_mapped->_padding2 = 0.0f;
            vk_scene_uniform_buffer_mapped->point_light_count = 0;
            vk_scene_uniform_buffer_mapped->spot_light_count = 0;
            vk_scene_uniform_buffer_mapped->_padding3 = 0.0f;
            vk_scene_uniform_buffer_mapped->_padding4 = 0.0f;


            for (uint32_t i = 0; i < object_count; ++i) {
                model_ub[i] = graphics::createUniformBuffer<ModelUniform>();
                if (model_ub[i].buffer == VK_NULL_HANDLE) return false;
                vk_model_uniform_buffers_mapped[i] = static_cast<ModelUniform*>(model_ub[i].mapped);

                vk_model_uniform_buffers_mapped[i]->model = glm::mat4(1.0f);
                vk_model_uniform_buffers_mapped[i]->albedo = glm::vec3(1.0f);
                vk_model_uniform_buffers_mapped[i]->_padding0 = 0.0f;
                vk_model_uniform_buffers_mapped[i]->specular = glm::vec3(1.0f);
                vk_model_uniform_buffers_mapped[i]->alpha = 32.0f;
            }


            auto create_marker = [](graphics::GpuBuffer& buf, ModelUniform*& mapped) -> bool {
                buf = graphics::createUniformBuffer<ModelUniform>();
                if (buf.buffer == VK_NULL_HANDLE) return false;
                mapped = static_cast<ModelUniform*>(buf.mapped);
                mapped->model = glm::mat4(1.0f);
                mapped->albedo = glm::vec3(1.0f);
                mapped->_padding0 = 0.0f;
                mapped->specular = glm::vec3(0.0f);
                mapped->alpha = 1.0f;
                mapped->is_gizmo = 1.0f;
                mapped->_padding1 = mapped->_padding2 = mapped->_padding3 = 0.0f;
                return true;
                };

            for (uint32_t i = 0; i < max_point_lights; ++i)
                if (!create_marker(point_gizmo_ub[i], vk_point_gizmo_uniform_buffers_mapped[i])) return false;

            for (uint32_t i = 0; i < max_spot_lights; ++i)
                if (!create_marker(spot_gizmo_ub[i], vk_spot_gizmo_uniform_buffers_mapped[i])) return false;

            if (!create_marker(cone_ub, vk_cone_uniform_buffer_mapped)) return false;

            return true;
        }


        bool createPointLightsBuffer() {
            point_lights_sb = graphics::createStorageBuffer<PointLight>(max_point_lights);
            if (point_lights_sb.buffer == VK_NULL_HANDLE) return false;

            vk_point_lights_buffer_mapped = static_cast<PointLight*>(point_lights_sb.mapped);

            for (uint32_t i = 0; i < max_point_lights; ++i) {
                vk_point_lights_buffer_mapped[i].position = glm::vec3(0.0f);
                vk_point_lights_buffer_mapped[i].color = glm::vec3(0.0f);
                vk_point_lights_buffer_mapped[i]._padding0 = 0.0f;
                vk_point_lights_buffer_mapped[i].intensity = 1.0f;
            }
            return true;
        }

        bool createSpotLightsBuffer() {
            spot_lights_sb = graphics::createStorageBuffer<SpotLight>(max_spot_lights);
            if (spot_lights_sb.buffer == VK_NULL_HANDLE) return false;

            vk_spot_lights_buffer_mapped = static_cast<SpotLight*>(spot_lights_sb.mapped);

            for (uint32_t i = 0; i < max_spot_lights; ++i) {
                vk_spot_lights_buffer_mapped[i].position = glm::vec3(0.0f);
                vk_spot_lights_buffer_mapped[i]._padding0 = 0.0f;
                vk_spot_lights_buffer_mapped[i].direction = glm::vec3(0.0f, -1.0f, 0.0f);
                vk_spot_lights_buffer_mapped[i]._padding1 = 0.0f;
                vk_spot_lights_buffer_mapped[i].color = glm::vec3(0.0f);
                vk_spot_lights_buffer_mapped[i].intensity = 1.0f;
                vk_spot_lights_buffer_mapped[i].inner_cosine = 0.0f;
                vk_spot_lights_buffer_mapped[i].outer_cosine = 0.0f;
                vk_spot_lights_buffer_mapped[i].inner_angle_deg = 0.0f;
                vk_spot_lights_buffer_mapped[i].outer_angle_deg = 0.0f;
            }

            return true;
        }


        VkDescriptorSetLayout vk_scene_descriptor_set_layout = VK_NULL_HANDLE;
        VkDescriptorSetLayout vk_model_descriptor_set_layout = VK_NULL_HANDLE;
        VkDescriptorPool vk_descriptor_pool = VK_NULL_HANDLE;
        VkDescriptorSet vk_scene_descriptor_set = VK_NULL_HANDLE;
        VkDescriptorSet vk_model_descriptor_sets[object_count] = {};
        VkDescriptorSet vk_point_gizmo_descriptor_sets[max_point_lights] = {};
        VkDescriptorSet vk_spot_gizmo_descriptor_sets[max_spot_lights] = {};
        VkDescriptorSet vk_cone_descriptor_set = VK_NULL_HANDLE;
        VkPipelineLayout vk_pipeline_layout = VK_NULL_HANDLE;
        VkPipeline vk_pipeline = VK_NULL_HANDLE;
        VkPipeline vk_line_pipeline = VK_NULL_HANDLE;

        bool createDescriptors() {
            vk_scene_descriptor_set_layout = graphics::createDescriptorSetLayout({
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                },
                {
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                },
                {
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                },
                });

            vk_model_descriptor_set_layout = graphics::createDescriptorSetLayout({
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                },
                });

            const VkDescriptorSetLayout set_layouts[] = {
                vk_scene_descriptor_set_layout,
                vk_model_descriptor_set_layout,
            };

            const VkPipelineLayoutCreateInfo pipeline_layout_info = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 2,
                .pSetLayouts = set_layouts,
            };

            if (vkCreatePipelineLayout(graphics::internal::context.device,
                &pipeline_layout_info, nullptr,
                &vk_pipeline_layout) != VK_SUCCESS) {
                std::cerr << "Failed to create pipeline layout\n";
                return false;
            }

            vk_descriptor_pool = graphics::createDescriptorPool({
                {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 2 + object_count + max_point_lights + max_spot_lights,
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 2,
                },
                }, 2 + object_count + max_point_lights + max_spot_lights);

            vk_scene_descriptor_set = graphics::allocateDescriptorSet(vk_descriptor_pool, vk_scene_descriptor_set_layout);

            {
                VkDescriptorSetLayout model_layouts[object_count];
                for (uint32_t i = 0; i < object_count; ++i) model_layouts[i] = vk_model_descriptor_set_layout;
                graphics::allocateDescriptorSets(vk_descriptor_pool, model_layouts, object_count, vk_model_descriptor_sets);
            }

            {
                VkDescriptorSetLayout layouts[max_point_lights];
                for (uint32_t i = 0; i < max_point_lights; ++i) layouts[i] = vk_model_descriptor_set_layout;
                graphics::allocateDescriptorSets(vk_descriptor_pool, layouts, max_point_lights, vk_point_gizmo_descriptor_sets);
            }

            {
                VkDescriptorSetLayout layouts[max_spot_lights];
                for (uint32_t i = 0; i < max_spot_lights; ++i) layouts[i] = vk_model_descriptor_set_layout;
                graphics::allocateDescriptorSets(vk_descriptor_pool, layouts, max_spot_lights, vk_spot_gizmo_descriptor_sets);
            }

            vk_cone_descriptor_set = graphics::allocateDescriptorSet(vk_descriptor_pool, vk_model_descriptor_set_layout);

            graphics::writeUniformBuffer(vk_scene_descriptor_set, scene_ub.buffer, sizeof(SceneUniforms), 0);
            graphics::writeStorageBuffer(vk_scene_descriptor_set, point_lights_sb.buffer, 1);
            graphics::writeStorageBuffer(vk_scene_descriptor_set, spot_lights_sb.buffer, 2);

            for (uint32_t i = 0; i < object_count; ++i) {
                graphics::writeUniformBuffer(vk_model_descriptor_sets[i], model_ub[i].buffer, sizeof(ModelUniform), 0);
            }

            for (uint32_t i = 0; i < max_point_lights; ++i)
                graphics::writeUniformBuffer(vk_point_gizmo_descriptor_sets[i], point_gizmo_ub[i].buffer, sizeof(ModelUniform), 0);

            for (uint32_t i = 0; i < max_spot_lights; ++i)
                graphics::writeUniformBuffer(vk_spot_gizmo_descriptor_sets[i], spot_gizmo_ub[i].buffer, sizeof(ModelUniform), 0);

            graphics::writeUniformBuffer(vk_cone_descriptor_set, cone_ub.buffer, sizeof(ModelUniform), 0);

            return true;
        }

        bool createPipeline() {
            VkShaderModule vert_module = graphics::loadShader("shaders/shader.vert.spv");
            VkShaderModule frag_module = graphics::loadShader("shaders/shader.frag.spv");
            if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
                return false;
            }

            const VkVertexInputBindingDescription vertex_binding = {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            };

            const VkVertexInputAttributeDescription vertex_attributes[] = {
                {
                    .location = 0,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, position),
                },
                {
                    .location = 1,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, color),
                },
                {
                    .location = 2,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, normal),
                },
            };

            graphics::PipelineConfig config = {};
            config.vertexShader = vert_module;
            config.fragmentShader = frag_module;
            config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            config.cullMode = VK_CULL_MODE_BACK_BIT;
            config.vertexBindings = &vertex_binding;
            config.vertexBindingCount = 1;
            config.vertexAttributes = vertex_attributes;
            config.vertexAttributeCount = 3;
            config.layout = vk_pipeline_layout;
            config.renderPass = graphics::internal::context.render_pass;

            vk_pipeline = graphics::createGraphicsPipeline(config);

            config.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            config.cullMode = VK_CULL_MODE_NONE;

            vk_line_pipeline = graphics::createGraphicsPipeline(config);

            vkDestroyShaderModule(graphics::internal::context.device, vert_module, nullptr);
            vkDestroyShaderModule(graphics::internal::context.device, frag_module, nullptr);

            return vk_pipeline != VK_NULL_HANDLE && vk_line_pipeline != VK_NULL_HANDLE;
        }
    } // namespace

    bool initialize() {
        computeVertexNormals(pyramid_vertices, pyramid_indices);

        if (!createConeBuffer()) {
            return false;
        }
        if (!createPointLightsBuffer()) {
            return false;
        }
        if (!createSpotLightsBuffer()) {
            return false;
        }
        if (!createPyramidBuffers()) {
            return false;
        }
        if (!createCubeBuffers()) {
            return false;
        }
        if (!createUniformBuffers()) {
            return false;
        }
        if (!createDescriptors()) {
            return false;
        }
        if (!createPipeline()) {
            return false;
        }
        return true;
    }

    void shutdown() {
        auto& context = graphics::internal::context;
        vkQueueWaitIdle(context.graphics_queue);

        if (vk_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(context.device, vk_pipeline, nullptr);
            vk_pipeline = VK_NULL_HANDLE;
        }
        if (vk_line_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(context.device, vk_line_pipeline, nullptr);
            vk_line_pipeline = VK_NULL_HANDLE;
        }
        if (vk_pipeline_layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(context.device, vk_pipeline_layout, nullptr);
            vk_pipeline_layout = VK_NULL_HANDLE;
        }
        if (vk_descriptor_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(context.device, vk_descriptor_pool, nullptr);
            vk_descriptor_pool = VK_NULL_HANDLE;
            vk_scene_descriptor_set = VK_NULL_HANDLE;
            for (uint32_t i = 0; i < object_count; ++i) {
                vk_model_descriptor_sets[i] = VK_NULL_HANDLE;
            }
            for (uint32_t i = 0; i < max_point_lights; ++i) vk_point_gizmo_descriptor_sets[i] = VK_NULL_HANDLE;
            for (uint32_t i = 0; i < max_spot_lights; ++i) vk_spot_gizmo_descriptor_sets[i] = VK_NULL_HANDLE;
            vk_cone_descriptor_set = VK_NULL_HANDLE;
        }
        if (vk_scene_descriptor_set_layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(context.device, vk_scene_descriptor_set_layout, nullptr);
            vk_scene_descriptor_set_layout = VK_NULL_HANDLE;
        }
        if (vk_model_descriptor_set_layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(context.device, vk_model_descriptor_set_layout, nullptr);
            vk_model_descriptor_set_layout = VK_NULL_HANDLE;
        }
        graphics::destroyBuffer(pyramid_vb);
        graphics::destroyBuffer(pyramid_ib);
        graphics::destroyBuffer(cube_vb);
        graphics::destroyBuffer(cube_ib);
        graphics::destroyBuffer(cone_vb);
        graphics::destroyBuffer(scene_ub);

        for (auto& buf : model_ub) graphics::destroyBuffer(buf);
        for (auto& buf : point_gizmo_ub) graphics::destroyBuffer(buf);
        for (auto& buf : spot_gizmo_ub) graphics::destroyBuffer(buf);
        graphics::destroyBuffer(cone_ub);

        graphics::destroyBuffer(point_lights_sb);
        graphics::destroyBuffer(spot_lights_sb);
    }

    void update(double time) {
        const auto& extent = graphics::internal::context.swapchain_extent;
        const float aspect = float(extent.width) / float(extent.height);

        static glm::vec3 camera_position = glm::vec3(5.0f, 2.5f, 5.0f);
        static float camera_yaw = -135.0f;

        static float camera_pitch = -20.0f;
        static float camera_fov = 40.0f;

        static glm::vec3 directional_light_direction = glm::vec3(-0.5f, -1.0f, -0.3f);
        static glm::vec3 directional_light_color = glm::vec3(1.0f, 0.95f, 0.8f);
        static float directional_light_intensity = 1.0f;
        static glm::vec3 ambient_light = glm::vec3(0.1f, 0.1f, 0.12f);

        static glm::vec3 albedo = glm::vec3(1.0f, 1.0f, 1.0f);
        static glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
        static float alpha = 32.0f;

        static int projection_mode = 0;

        static int selected_object = 0;

        static glm::vec3 manual_position[object_count] = {
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(-2.0f, 0.0f, 0.0f),
            glm::vec3(2.0f, 0.0f, 0.0f)
        };

        static glm::vec3 manual_rotation[object_count] = {
            glm::vec3(0.0f),
            glm::vec3(0.0f, 45.0f, 0.0f),
            glm::vec3(0.0f, -30.0f, 0.0f)
        };

        static glm::vec3 scale[object_count] = {
            glm::vec3(1.0f),
            glm::vec3(0.8f),
            glm::vec3(0.6f)
        };

        static glm::vec3 color[object_count] = {
            glm::vec3(0.6f, 0.2f, 0.2f),
            glm::vec3(1.0f, 0.5f, 0.2f),
            glm::vec3(0.2f, 0.5f, 1.0f)
        };

        static bool  is_playing = true;
        static float anim_speed = 1.0f;
        static float anim_radius = 0.6f;
        static float anim_height = 0.0f;
        static float anim_angle = 0.0f;

        static double last_time = time;
        const double delta = time - last_time;
        last_time = time;

        if (is_playing) {
            anim_angle += float(delta) * anim_speed;
        }

        ImGui::Begin("Controls");

        ImGui::Separator();
        ImGui::Text("Camera:");
        ImGui::SliderFloat3("Position", &camera_position.x, -20.0f, 20.0f);
        ImGui::SliderFloat("Yaw", &camera_yaw, -180.0f, 180.0f, "%.1f deg");
        ImGui::SliderFloat("Pitch", &camera_pitch, -89.0f, 89.0f, "%.1f deg");
        ImGui::SliderFloat("FOV", &camera_fov, 25.0f, 100.0f, "%.1f deg");

        ImGui::Separator();
        ImGui::Text("Directional Light:");
        ImGui::SliderFloat3("Light Direction", &directional_light_direction.x, -1.0f, 1.0f);
        ImGui::ColorEdit3("Light Color", &directional_light_color.x);
        ImGui::SliderFloat("Light Intensity", &directional_light_intensity, 0.0f, 5.0f);
        ImGui::ColorEdit3("Ambient", &ambient_light.x);

        ImGui::Separator();
        ImGui::Text("Material:");
        ImGui::ColorEdit3("Albedo", &albedo.x);
        ImGui::ColorEdit3("Specular", &specular.x);
        ImGui::SliderFloat("Alpha", &alpha, 1.0f, 256.0f);

        ImGui::Separator();
        ImGui::Text("Point Lights (%u / %u):", point_light_count, max_point_lights);

        if (ImGui::Button("Add Point Light") && point_light_count < max_point_lights) {
            PointLight& light = cpu_point_lights[point_light_count];
            light.position = glm::vec3(2.0f, 2.0f, 2.0f);
            light.color = glm::vec3(1.0f, 1.0f, 1.0f);
            light.intensity = 1.0f;
            point_light_count++;
        }

        ImGui::SameLine();

        if (ImGui::Button("Remove Point Light") && point_light_count > 0) {
            point_light_count--;
        }

        for (uint32_t i = 0; i < point_light_count; ++i) {
            ImGui::PushID(i);

            ImGui::Text("Light %u", i);
            ImGui::SliderFloat3("Position##light", &cpu_point_lights[i].position.x, -10.0f, 10.0f);
            ImGui::ColorEdit3("Color##light", &cpu_point_lights[i].color.x);
            ImGui::SliderFloat("Intensity##light", &cpu_point_lights[i].intensity, 0.0f, 20.0f);

            ImGui::PopID();
        }

        ImGui::Separator();
        ImGui::Text("Spot Lights (%u / %u):", spot_light_count, max_spot_lights);

        if (ImGui::Button("Add Spot Light") && spot_light_count < max_spot_lights) {
            SpotLight& light = cpu_spot_lights[spot_light_count];
            light.position = glm::vec3(0.0f, 3.0f, 2.0f);
            light.direction = glm::vec3(0.0f, -1.0f, -0.5f);
            light.color = glm::vec3(1.0f, 1.0f, 1.0f);
            light.intensity = 1.0f;
            light.inner_angle_deg = 20.0f;
            light.outer_angle_deg = 40.0f;
            light.inner_cosine = std::cos(glm::radians(light.inner_angle_deg));
            light.outer_cosine = std::cos(glm::radians(light.outer_angle_deg));
            spot_light_count++;
        }

        ImGui::SameLine();

        if (ImGui::Button("Remove Spot Light") && spot_light_count > 0) {
            spot_light_count--;
        }

        for (uint32_t i = 0; i < spot_light_count; ++i) {
            ImGui::PushID(int(i) + 1000);

            ImGui::Text("Spot %u", i);
            ImGui::SliderFloat3("Position##spot", &cpu_spot_lights[i].position.x, -10.0f, 10.0f);
            ImGui::SliderFloat3("Direction##spot", &cpu_spot_lights[i].direction.x, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color##spot", &cpu_spot_lights[i].color.x);
            ImGui::SliderFloat("Intensity##spot", &cpu_spot_lights[i].intensity, 0.0f, 20.0f);

            if (ImGui::SliderFloat("Outer Angle##spot", &cpu_spot_lights[i].outer_angle_deg,
                cpu_spot_lights[i].inner_angle_deg + 1.0f, 60.0f, "%.1f deg")) {
                cpu_spot_lights[i].outer_cosine = std::cos(glm::radians(cpu_spot_lights[i].outer_angle_deg));
            }
            if (ImGui::SliderFloat("Inner Angle##spot", &cpu_spot_lights[i].inner_angle_deg,
                1.0f, cpu_spot_lights[i].outer_angle_deg - 1.0f, "%.1f deg")) {
                cpu_spot_lights[i].inner_cosine = std::cos(glm::radians(cpu_spot_lights[i].inner_angle_deg));
            }
            cpu_spot_lights[i].inner_cosine = std::cos(glm::radians(cpu_spot_lights[i].inner_angle_deg));
            cpu_spot_lights[i].outer_cosine = std::cos(glm::radians(cpu_spot_lights[i].outer_angle_deg));

            ImGui::PopID();
        }

        ImGui::Separator();
        ImGui::Checkbox("Show Light Gizmos", &show_gizmos);

        ImGui::Text("Select Pyramid:");
        ImGui::RadioButton("Pyramid 0", &selected_object, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Pyramid 1", &selected_object, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Pyramid 2", &selected_object, 2);

        ImGui::Separator();

        ImGui::Text("Projection:");
        ImGui::RadioButton("Perspective", &projection_mode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Orthographic", &projection_mode, 1);

        ImGui::Separator();

        ImGui::Text("Manual transforms (Pyramid %d):", selected_object);
        ImGui::SliderFloat3("Obj Position", &manual_position[selected_object].x, -5.0f, 5.0f);
        ImGui::SliderFloat3("Rotation", &manual_rotation[selected_object].x, -180.0f, 180.0f);
        ImGui::SliderFloat3("Scale", &scale[selected_object].x, 0.1f, 3.0f);

        ImGui::Separator();

        ImGui::Text("Color:");
        ImGui::ColorEdit3("Base color", &color[selected_object].x);

        ImGui::Separator();
        ImGui::Text("Animation:");

        if (is_playing) {
            if (ImGui::Button("||  Pause")) {
                is_playing = false;
            }
        }
        else {
            if (ImGui::Button(">  Play")) {
                is_playing = true;
            }
        }

        ImGui::SliderFloat("Speed", &anim_speed, -3.0f, 3.0f);
        ImGui::SliderFloat("Radius", &anim_radius, 0.0f, 3.0f);
        ImGui::SliderFloat("Height", &anim_height, -1.0f, 1.0f);

        ImGui::End();

        glm::vec3 camera_front;
        camera_front.x = std::cos(glm::radians(camera_yaw)) * std::cos(glm::radians(camera_pitch));
        camera_front.y = std::sin(glm::radians(camera_pitch));
        camera_front.z = std::sin(glm::radians(camera_yaw)) * std::cos(glm::radians(camera_pitch));
        camera_front = glm::normalize(camera_front);

        glm::mat4 view = glm::lookAt(
            camera_position,
            camera_position + camera_front,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        glm::mat4 proj;
        if (projection_mode == 0) {
            proj = glm::perspective(glm::radians(camera_fov), aspect, 0.1f, 100.0f);
        }
        else {
            float ortho_size = 3.0f;
            proj = glm::ortho(
                -ortho_size * aspect, ortho_size * aspect,
                -ortho_size, ortho_size,
                0.1f, 100.0f
            );
        }
        proj[1][1] *= -1.0f;

        glm::mat4 view_projection = proj * view;

        for (uint32_t i = 0; i < point_light_count; ++i) {
            vk_point_lights_buffer_mapped[i] = cpu_point_lights[i];
        }

        for (uint32_t i = 0; i < spot_light_count; ++i) {
            if (glm::length(cpu_spot_lights[i].direction) < 0.001f) {
                cpu_spot_lights[i].direction = glm::vec3(0.0f, -1.0f, 0.0f);
            }
            else {
                cpu_spot_lights[i].direction = glm::normalize(cpu_spot_lights[i].direction);
            }
            vk_spot_lights_buffer_mapped[i] = cpu_spot_lights[i];
        }

        vk_scene_uniform_buffer_mapped->view_projection = view_projection;
        vk_scene_uniform_buffer_mapped->camera_position = camera_position;
        if (glm::length(directional_light_direction) < 0.001f) {
            vk_scene_uniform_buffer_mapped->directional_light_direction = glm::vec3(0.0f, -1.0f, 0.0f);
        }
        else {
            vk_scene_uniform_buffer_mapped->directional_light_direction = glm::normalize(directional_light_direction);
        }
        vk_scene_uniform_buffer_mapped->directional_light_color = directional_light_color;
        vk_scene_uniform_buffer_mapped->directional_light_intensity = directional_light_intensity;
        vk_scene_uniform_buffer_mapped->ambient_light = ambient_light;
        vk_scene_uniform_buffer_mapped->point_light_count = point_light_count;
        vk_scene_uniform_buffer_mapped->spot_light_count = spot_light_count;

        for (int i = 0; i < object_count; ++i) {
            glm::mat4 model = glm::mat4(1.0f);

            glm::vec3 final_position = manual_position[i];

            float phase_angle = anim_angle + float(i) * 2.094f;
            final_position.x += anim_radius * std::cos(phase_angle);
            final_position.z += anim_radius * std::sin(phase_angle);
            float wave_amplitude = 0.3f;
            final_position.y += anim_height + wave_amplitude * std::sin(phase_angle * 2.0f);

            model = glm::translate(model, final_position);

            model = glm::rotate(model, glm::radians(manual_rotation[i].x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(manual_rotation[i].y) + anim_angle * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(manual_rotation[i].z), glm::vec3(0.0f, 0.0f, 1.0f));

            model = glm::scale(model, scale[i]);


            vk_model_uniform_buffers_mapped[i]->model = model;
            vk_model_uniform_buffers_mapped[i]->albedo = albedo * color[i];
            vk_model_uniform_buffers_mapped[i]->specular = specular;
            vk_model_uniform_buffers_mapped[i]->alpha = alpha;
            vk_model_uniform_buffers_mapped[i]->is_gizmo = 0.0f;
            vk_model_uniform_buffers_mapped[i]->_padding1 = 0.0f;
            vk_model_uniform_buffers_mapped[i]->_padding2 = 0.0f;
            vk_model_uniform_buffers_mapped[i]->_padding3 = 0.0f;
        }
    }


    void render(const graphics::internal::FrameData& fd) {
        vkResetCommandBuffer(fd.command_buffer, 0);

        const VkCommandBufferBeginInfo command_buffer_begin = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        vkBeginCommandBuffer(fd.command_buffer, &command_buffer_begin);

        const VkClearValue clear_values[] = {
            {.color = {.float32 = { 0.1f, 0.1f, 0.1f, 1.0f } } },
            {.depthStencil = { 1.0f, 0 } },
        };

        const VkRenderPassBeginInfo render_pass_begin = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = graphics::internal::context.render_pass,
            .framebuffer = fd.framebuffer,
            .renderArea = {.extent = graphics::internal::context.swapchain_extent },
            .clearValueCount = sizeof(clear_values) / sizeof(clear_values[0]),
            .pClearValues = clear_values,
        };

        vkCmdBeginRenderPass(fd.command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

        const VkViewport viewport = {
            .x = 0.0f, .y = 0.0f,
            .width = float(graphics::internal::context.swapchain_extent.width),
            .height = float(graphics::internal::context.swapchain_extent.height),
            .minDepth = 0.0f, .maxDepth = 1.0f,
        };

        const VkRect2D scissor = {
            .extent = graphics::internal::context.swapchain_extent,
        };

        vkCmdSetViewport(fd.command_buffer, 0, 1, &viewport);
        vkCmdSetScissor(fd.command_buffer, 0, 1, &scissor);

        vkCmdBindPipeline(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(fd.command_buffer, 0, 1, &pyramid_vb.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(fd.command_buffer, pyramid_ib.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            vk_pipeline_layout,
            0,
            1,
            &vk_scene_descriptor_set,
            0, nullptr);

        for (uint32_t i = 0; i < object_count; ++i) {
            vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                vk_pipeline_layout, 1, 1, &vk_model_descriptor_sets[i],
                0, nullptr);

            vkCmdDrawIndexed(fd.command_buffer,
                static_cast<uint32_t>(pyramid_indices.size()),
                1, 0, 0, 0);
        }


        if (show_gizmos) {
            const VkDeviceSize offset = 0;

            vkCmdBindVertexBuffers(fd.command_buffer, 0, 1, &cube_vb.buffer, &offset);
            vkCmdBindIndexBuffer(fd.command_buffer, cube_ib.buffer, 0, VK_INDEX_TYPE_UINT32);

            for (uint32_t i = 0; i < point_light_count; ++i) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), cpu_point_lights[i].position);
                model = glm::scale(model, glm::vec3(0.15f));

                ModelUniform* marker = vk_point_gizmo_uniform_buffers_mapped[i];
                marker->model = model;
                marker->albedo = cpu_point_lights[i].color;
                marker->is_gizmo = 1.0f;

                vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    vk_pipeline_layout, 1, 1, &vk_point_gizmo_descriptor_sets[i], 0, nullptr);
                vkCmdDrawIndexed(fd.command_buffer, static_cast<uint32_t>(cube_indices.size()), 1, 0, 0, 0);
            }

            for (uint32_t i = 0; i < spot_light_count; ++i) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), cpu_spot_lights[i].position);
                model = glm::scale(model, glm::vec3(0.15f));

                ModelUniform* marker = vk_spot_gizmo_uniform_buffers_mapped[i];
                marker->model = model;
                marker->albedo = cpu_spot_lights[i].color;
                marker->is_gizmo = 1.0f;

                vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    vk_pipeline_layout, 1, 1, &vk_spot_gizmo_descriptor_sets[i], 0, nullptr);
                vkCmdDrawIndexed(fd.command_buffer, static_cast<uint32_t>(cube_indices.size()), 1, 0, 0, 0);
            }

            if (spot_light_count > 0) {
                uint32_t total_vertices = 0;
                constexpr uint32_t vertices_per_cone = 66;
                for (uint32_t i = 0; i < spot_light_count; ++i) {
                    if (total_vertices + vertices_per_cone > max_cone_vertices) break;
                    const uint32_t count = buildConeVertices(
                        cpu_spot_lights[i], 1.5f, cone_vertices_mapped + total_vertices);
                    total_vertices += count;
                }

                if (total_vertices > 0) {
                    vk_cone_uniform_buffer_mapped->model = glm::mat4(1.0f);
                    vk_cone_uniform_buffer_mapped->albedo = glm::vec3(1.0f);
                    vk_cone_uniform_buffer_mapped->is_gizmo = 1.0f;

                    vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        vk_pipeline_layout, 1, 1, &vk_cone_descriptor_set, 0, nullptr);

                    vkCmdBindPipeline(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_line_pipeline);

                    const VkDeviceSize cone_offset = 0;
                    vkCmdBindVertexBuffers(fd.command_buffer, 0, 1, &cone_vb.buffer, &cone_offset);

                    vkCmdDraw(fd.command_buffer, total_vertices, 1, 0, 0);
                }
            }
        }

        vkCmdEndRenderPass(fd.command_buffer);
        vkEndCommandBuffer(fd.command_buffer);
    }

} // namespace application
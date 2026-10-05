#define GLM_FORCE_DEPTH_ZERO_TO_ONE // для того, чтобы ortho-проекция не отсекала все, что ниже Z=0

#include "application.hpp"

#include <cstring>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>

#include <vk_mem_alloc.h>

#include <fstream>
#include <vector>

#include <cmath>

// загрузка шейдера
// загружает скомпилированный .spv файл и создает VkShaderModule
VkShaderModule loadShaderModule(const char* path) {
    // открытие файла в бинарном режиме
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    // сообщение об ошибке, если файл не открылся
    if (!file) {
        std::cerr << "Failed to open shader file: " << path << '\n';
        return VK_NULL_HANDLE;
    }

    // возвращение текущей позиции (конец файла - размер)
    const size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(size); // выделение буфера ровно под содержимое файла

    // перемещение курсора в начало и чтение файла
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();

    // описание создаваемого VkShaderModule
    const VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, // тег типа
        .codeSize = size,                                     // размер в байтах
        .pCode = reinterpret_cast<const uint32_t*>(buffer.data()), // приведение char* к uint32_t*
    };

    VkShaderModule result = VK_NULL_HANDLE;
    // создание модуля на устройстве
    if (vkCreateShaderModule(graphics::internal::context.device,
        &info, nullptr, &result) != VK_SUCCESS) {
        std::cerr << "Failed to create shader module: " << path << '\n';
        return VK_NULL_HANDLE;
    }
    return result;
}

// данные пирамиды
namespace application {

    namespace {
        const Vertex pyramid_vertices[] = {
            // 5 вершин пирамиды (1 верхушка и 4 в основании)
            // цвета не используются, т к прцедурный цвет вычисляется в шейдере

            { {  0.0f,  0.7f,  0.0f }, { 1.0f, 1.0f, 1.0f } }, // верхушка
            { { -0.5f,  0.0f, -0.5f }, { 1.0f, 1.0f, 1.0f } }, // левый-задний
            { {  0.5f,  0.0f, -0.5f }, { 1.0f, 1.0f, 1.0f } }, // правый-задний
            { {  0.5f,  0.0f,  0.5f }, { 1.0f, 1.0f, 1.0f } }, // правый-передний
            { { -0.5f,  0.0f,  0.5f }, { 1.0f, 1.0f, 1.0f } }, // левый-передний
        };

        // индексы вершин. Описывают, из каких вершин строятся треугольники
        // рисутются треугольники с обходом по часовой стрелке
        const uint32_t pyramid_indices[] = {
            // 4 боковые грани (у каждой треугольник из верхушки и 2 точки основания)
            0, 1, 2,
            0, 2, 3,
            0, 3, 4,
            0, 4, 1,
            // основание (квадрат разбит на два треугольника)
            1, 3, 2,
            1, 4, 3,
        };

        // вершинный буфер
        // VkBuffer
        VkBuffer vk_vertex_buffer = VK_NULL_HANDLE;
        // VMA объединяет операции узнавая требования каждого буфера, выбора подходящего типа памяти, выделение памяти во VRAM,
        // привязка к нему буферов/изображений и распределяет множество ресурсов внути крупных блоков видеопамяти.
        // запись о том, где именно лежит в памяти
        VmaAllocation vk_vertex_buffer_allocation = VK_NULL_HANDLE;

        // то же самое для индексного буфера
        VkBuffer vk_index_buffer = VK_NULL_HANDLE;
        VmaAllocation vk_index_buffer_allocation = VK_NULL_HANDLE;

        // вершинный буфер
        bool createVertexBuffer() {
            // описание создаваемого буфера
            const VkBufferCreateInfo buffer_info = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // тег типа
                .size = sizeof(pyramid_vertices),              // размер в байтах
                .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,    // Vulkan по usage понимает, как размещать данные в памяти
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,      // использование одним семейством очередей за раз
            };

            // описание того, как VMA (библиотека для выделения памяти) должна выделить память под буфер
            const VmaAllocationCreateInfo alloc_info = {
                // память доступна с CPU через pMappedData (указатель на начало блока памяти,
                // который был отображен из памяти устройства в память хоста (CPU))
                // последовательная запись в память с CPU
                .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
                       | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, 
                .usage = VMA_MEMORY_USAGE_AUTO, // VMA решает, какой тип памяти использовать
            };

            // место, куда VMA положит информацию о выделенной памяти
            VmaAllocationInfo allocation_info{};

            // создание буфера и аллокации (процесс выделения памяти под буферы и др данные под использвание GPU)
            if (vmaCreateBuffer(graphics::internal::context.allocator,
                &buffer_info, &alloc_info,
                &vk_vertex_buffer, &vk_vertex_buffer_allocation,
                &allocation_info) != VK_SUCCESS) {
                std::cerr << "Failed to create vertex buffer\n";
                return false;
            }

            // копирование данных пирамиды в замапленную память (память, к которой CPU имеет прямой доступ через указатель)
            // GPU видит эти данные
            std::memcpy(allocation_info.pMappedData,
                pyramid_vertices,
                sizeof(pyramid_vertices));

            return true;
        }

        // индексный буфер
        bool createIndexBuffer() {
            // описание создаваемого буфера
            const VkBufferCreateInfo buffer_info = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // тег типа
                .size = sizeof(pyramid_indices),               // размер в байтах
                // индексный буфер
                .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,     // Vulkan по usage понимает, как размещать данные в памяти
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,      // использование одним семейством очередей за раз
            };

            // описание того, как VMA (библиотека для выделения памяти) должна выделить память под буфер
            const VmaAllocationCreateInfo alloc_info = {
                // память доступна с CPU через pMappedData (указатель на начало блока памяти,
                // который был отображен из памяти устройства в память хоста (CPU))
                // последовательная запись в память с CPU
                .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
                       | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO, // VMA решает, какой тип памяти использовать
            };

            // место, куда VMA положит информацию о выделенной памяти
            VmaAllocationInfo allocation_info{};

            // создание буфера и аллокации (процесс выделения памяти под буферы и др данные под использвание GPU)
            if (vmaCreateBuffer(graphics::internal::context.allocator,
                &buffer_info, &alloc_info,
                &vk_index_buffer, &vk_index_buffer_allocation,
                &allocation_info) != VK_SUCCESS) {
                std::cerr << "Failed to create index buffer\n";
                return false;
            }

            // копирование данных пирамиды в замапленную память
            // GPU видит эти данные
            std::memcpy(allocation_info.pMappedData,
                pyramid_indices,
                sizeof(pyramid_indices));

            return true;
        }

        // Scene Uniforms. Данные, общие для всей сцены, один буфер на кадр
        // view и proj одинаковы для всех объектов
        struct SceneUniforms {
            glm::mat4 view;   // из мировых координат в координаты камеры
            glm::mat4 proj;   // из координат камеры в NDC (Normalized Device Coordinates, то, что видит GPU)
        };

        VkBuffer vk_scene_uniform_buffer = VK_NULL_HANDLE;
        VmaAllocation vk_scene_uniform_buffer_allocation = VK_NULL_HANDLE;
        SceneUniforms* vk_scene_uniform_buffer_mapped = nullptr;

        // Model Uniforms. Данные конкретного объекта, свой буфер для каждого объекта
        // у каждого объекта своя model-матрица и свой цвет
        struct ModelUniform {
            glm::mat4 model;  // из локальных координат в мировые
            glm::vec3 color;  // UI-цвет
            float _padding;   // выравнивание до 16 байт
        };

        constexpr uint32_t object_count = 3; // количество объектов на сцене

        VkBuffer vk_model_uniform_buffers[object_count] = {};
        VmaAllocation vk_model_uniform_buffer_allocations[object_count] = {};
        ModelUniform* vk_model_uniform_buffers_mapped[object_count] = {};

        // создание Scene-буфера и Model-буфера (по одному на объект)
        bool createUniformBuffers() {
            // Scene buffer один на всю сцену
            {
                const VkBufferCreateInfo buffer_info = {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                    .size = sizeof(SceneUniforms),
                    .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                };

                // выделение памяти для SceneUniforms
                const VmaAllocationCreateInfo alloc_info = {
                    .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
                           | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                    .usage = VMA_MEMORY_USAGE_AUTO,
                };

                // место, куда VMA положит информацию о выделенной памяти
                VmaAllocationInfo allocation_info{};

                // создание буфера и аллокации (процесс выделения памяти под буферы и др данные под использвание GPU)
                if (vmaCreateBuffer(graphics::internal::context.allocator,
                    &buffer_info, &alloc_info,
                    &vk_scene_uniform_buffer,
                    &vk_scene_uniform_buffer_allocation,
                    &allocation_info) != VK_SUCCESS) {
                    std::cerr << "Failed to create scene uniform buffer\n";
                    return false;
                }

                vk_scene_uniform_buffer_mapped =
                    static_cast<SceneUniforms*>(allocation_info.pMappedData);

                // инициализация значениями по умолчанию
                // записывает новую матрицу view в память Scene uniform-буфера
                vk_scene_uniform_buffer_mapped->view = glm::mat4(1.0f);
                vk_scene_uniform_buffer_mapped->proj = glm::mat4(1.0f);
            }

            // Model buffers по одному на каждый объект
            // создание object_count буферов в цикле
            for (uint32_t i = 0; i < object_count; ++i) {
                // описание создаваемого буфера
                const VkBufferCreateInfo buffer_info = {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // тег типа
                    .size = sizeof(ModelUniform),                // размер в байтах
                    // uniform-буфер
                    .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,   // Vulkan по usage понимает, как размещать данные в памяти
                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,      // использование одним семейством очередей за раз
                };

                // описание того, как VMA (библиотека для выделения памяти) должна выделить память под буфер
                const VmaAllocationCreateInfo alloc_info = {
                    // память доступна с CPU через pMappedData (указатель на начало блока памяти,
                    // который был отображен из памяти устройства в память хоста (CPU))
                    // последовательная запись в память с CPU
                    .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
                           | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                    .usage = VMA_MEMORY_USAGE_AUTO,
                };

                // место, куда VMA положит информацию о выделенной памяти
                VmaAllocationInfo allocation_info{};

                // создание буфера и аллокации (процесс выделения памяти под буферы и др данные под использвание GPU)
                if (vmaCreateBuffer(graphics::internal::context.allocator,
                    &buffer_info, &alloc_info,
                    &vk_model_uniform_buffers[i],
                    &vk_model_uniform_buffer_allocations[i],
                    &allocation_info) != VK_SUCCESS) {
                    std::cerr << "Failed to create uniform buffer #" << i << '\n';
                    return false;
                }

                // сохранение указателя, передача каждого кадра
                vk_model_uniform_buffers_mapped[i] = static_cast<ModelUniform*>(allocation_info.pMappedData);

                // инициализация значениями по умолчанию. Сначала белый цвет, при первом кадре update() все перезаписывает
                vk_model_uniform_buffers_mapped[i]->model = glm::mat4(1.0f);
                vk_model_uniform_buffers_mapped[i]->color = glm::vec3(1.0f);
                vk_model_uniform_buffers_mapped[i]->_padding = 0.0f;
            }
            return true;
        }

        // дескрипторы
        // описывает, какие ресурсы видит шейдер
        // Два layout. Один для Scene, другой для Model
        VkDescriptorSetLayout vk_scene_descriptor_set_layout = VK_NULL_HANDLE;
        VkDescriptorSetLayout vk_model_descriptor_set_layout = VK_NULL_HANDLE;
        // выделяет память под дескрипторы. Из него выделяются descriptor sets
        VkDescriptorPool vk_descriptor_pool = VK_NULL_HANDLE;
        // Один Scene set
        // по одному Model set на объект, каждый ссылается на свой uniform-буфер
        VkDescriptorSet vk_scene_descriptor_set = VK_NULL_HANDLE;
        VkDescriptorSet vk_model_descriptor_sets[object_count] = {};
        // связка descriptor set layout и push-констант
        // пайплайн использует его, чтобы знать, какие наборы будут привязаны
        VkPipelineLayout vk_pipeline_layout = VK_NULL_HANDLE;
        // графический пайплайн. Описание того, как GPU рисует
        VkPipeline vk_pipeline = VK_NULL_HANDLE;

        // дескриптор - описание внешнего ресурса, кот диктует его тип, место связывания (binding) в шейдере,
        // кол-во связанных с ним ресурсов и шейдеры, где они используются.
        // создание двух дескрипторов set layout (Scene и Model), pipline layout,
        // пул дескрипторов и сами наборы: 1 Scene и N Model
        bool createDescriptors() {
            // Descriptor set layout для Scene (set=0)
            const VkDescriptorSetLayoutBinding scene_binding = {
                .binding = 0, // совпадает с layout в шейдере
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // тип uniform buffer
                .descriptorCount = 1, // 1 буфер на дескриптор, виден в vertex и fragment шейдерах
                // шейдер использует этот буфер и в вершинном (матрицы), и во фрагментном (цвет) шейдере
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            };

            // описывает тип внешних ресурсов и локацию связывания с объектом внешнего ресурса для использования их в шейдерах
            const VkDescriptorSetLayoutCreateInfo scene_layout_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 1,
                .pBindings = &scene_binding,
            };

            if (vkCreateDescriptorSetLayout(graphics::internal::context.device,
                &scene_layout_info, nullptr,
                &vk_scene_descriptor_set_layout) != VK_SUCCESS) {
                std::cerr << "Failed to create descriptor set layout\n";
                return false;
            }

            // Descriptor set layout для Model (set=1)
            const VkDescriptorSetLayoutBinding model_binding = {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            };

            const VkDescriptorSetLayoutCreateInfo model_layout_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 1,
                .pBindings = &model_binding,
            };

            if (vkCreateDescriptorSetLayout(graphics::internal::context.device,
                &model_layout_info, nullptr,
                &vk_model_descriptor_set_layout) != VK_SUCCESS) {
                std::cerr << "Failed to create model descriptor set layout\n";
                return false;
            }

            // Pipeline layout, знает про оба set layout: index 0 — Scene, index 1 — Model.
            const VkDescriptorSetLayout set_layouts[] = {
                vk_scene_descriptor_set_layout,
                vk_model_descriptor_set_layout,
            };

            // Pipeline Layout ссылается на наборы описателей ресурсов (descriptor set layout)
            // пайплайн через него узнает, какие ресурсы будут привязаны нему
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

            // Descriptor pool выделяет объекты наборов дескрипторов. Описывает макс кол-во дескрипторов и макс кол-во наборов дескрипторов, кот можно из него выделить
            // в пуле должно хватить на 1 Sceen set и object_count Model sets
            // по одному uniform-буферу на каждый объект
            const VkDescriptorPoolSize pool_size = {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1 + object_count,
            };

            const VkDescriptorPoolCreateInfo pool_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 1 + object_count,
                .poolSizeCount = 1,
                .pPoolSizes = &pool_size,
            };

            if (vkCreateDescriptorPool(graphics::internal::context.device,
                &pool_info, nullptr,
                &vk_descriptor_pool) != VK_SUCCESS) {
                std::cerr << "Failed to create descriptor pool\n";
                return false;
            }

            // выделение наборов дескрипторов по описанию
            // выделение Scene set (1 шт)
            const VkDescriptorSetAllocateInfo scene_alloc_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = vk_descriptor_pool,
                .descriptorSetCount = 1,
                .pSetLayouts = &vk_scene_descriptor_set_layout,
            };

            // инициализирует наборы в массиве, который указан в последнем аргументе
            if (vkAllocateDescriptorSets(graphics::internal::context.device,
                &scene_alloc_info,
                &vk_scene_descriptor_set) != VK_SUCCESS) {
                std::cerr << "Failed to allocate scene descriptor set\n";
                return false;
            }

            // выделение Model sets — по одному на объект (object_count шт)
            VkDescriptorSetLayout model_layouts[object_count];
            for (uint32_t i = 0; i < object_count; ++i) {
                model_layouts[i] = vk_model_descriptor_set_layout;
            }

            // выделение object_count наборов
            const VkDescriptorSetAllocateInfo model_alloc_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = vk_descriptor_pool,
                .descriptorSetCount = object_count,
                .pSetLayouts = model_layouts,
            };

            // инициализирует наборы в массике, который указан в последнем аргументе
            if (vkAllocateDescriptorSets(graphics::internal::context.device,
                &model_alloc_info, vk_model_descriptor_sets) != VK_SUCCESS) {
                std::cerr << "Failed to allocate descriptor sets\n";
                return false;
            }

            // привязка uniform буфера к созданному набору (Scene-буфера к Scene set)
            {
                const VkDescriptorBufferInfo buffer_info = {
                    .buffer = vk_scene_uniform_buffer, // объект ресурса
                    .offset = 0,                       // куда сместить указатель начала памяти
                    .range = sizeof(SceneUniforms),    // размер памяти
                };

                const VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = vk_scene_descriptor_set, // к какому набору нужно привязать ресурс
                    .dstBinding = 0,                   // индекс дескриптора в шейдере
                    .dstArrayElement = 0,
                    .descriptorCount = 1,              // сколько ресурсов описывает дескриптор
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // тип ресурса
                    .pBufferInfo = &buffer_info,       // адрес/массив на описание ресурсов
                };

                vkUpdateDescriptorSets(graphics::internal::context.device,
                    1, &write, 0, nullptr);
            }

            // привязка каждого Model-буфера к соответствующему Model set
            // после VkUpdateDescriptorSets шейдер увидит данные буфера
            for (uint32_t i = 0; i < object_count; ++i) {
                const VkDescriptorBufferInfo buffer_info = {
                    .buffer = vk_model_uniform_buffers[i], // буфер
                    .offset = 0,                           // с начала
                    .range = sizeof(ModelUniform),         // весь размер структуры
                };

                const VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = vk_model_descriptor_sets[i], // то, в какой набор происходит запись
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &buffer_info,    // описание буфера
                };

                vkUpdateDescriptorSets(graphics::internal::context.device,
                    1, &write, 0, nullptr);
            }

            return true;
        }

        // графический пайплайн
        bool createPipeline() {
            // загрузка скомпилированных шейдеров
            VkShaderModule vert_module = loadShaderModule("shaders/shader.vert.spv");
            VkShaderModule frag_module = loadShaderModule("shaders/shader.frag.spv");
            if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
                return false;
            }

            // описание стадий пайплайна: какие шейдеры использовать
            const VkPipelineShaderStageCreateInfo stages[] = {
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT, // вершинный
                    .module = vert_module,
                    .pName = "main",                     // точка входа
                },
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT, // фрагментный
                    .module = frag_module,
                    .pName = "main",
                },
            };

            // описание того, как читать одну вершину из вершинного буфера
            const VkVertexInputBindingDescription vertex_binding = {
                .binding = 0,                               // индекс буфера, по которому будут браться данные
                .stride = sizeof(Vertex),                   // размер одной вершины в байтах
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,   // сдвиг указателя буфера после каждого чтения вершины
            };

            // описание атрибутов вершины
            const VkVertexInputAttributeDescription vertex_attributes[] = {
                {
                    .location = 0,                          // совпадает с layout в шейдере
                    .binding = 0,                           // индекс буфера, по которому будут браться данные
                    .format = VK_FORMAT_R32G32B32_SFLOAT,   // формат данных для атрибута вершины (vec3)
                    .offset = offsetof(Vertex, position),   // смещение внутри структуры
                },
                {
                    // для цвета
                    .location = 1,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, color),
                },
            };

            // формирует примитив для вершинного шейдера
            const VkPipelineVertexInputStateCreateInfo vertex_input = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = 1,
                .pVertexBindingDescriptions = &vertex_binding, // описывает из каких буферов брать данные: размер в байтах данных для 1 примитива и индексы привязки буферов
                .vertexAttributeDescriptionCount = 2,
                .pVertexAttributeDescriptions = vertex_attributes, // описывает атрибуты и вершины: их типы данных, из какого юуфера брать значения, индексы объявления в вершинном шейдере
            };

            // список треугольников. То, как собирать примитивы
            const VkPipelineInputAssemblyStateCreateInfo input_assembly = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE,
            };

            // viewport и scissor - динамические, задаются в render, здесь счетчики
            const VkPipelineViewportStateCreateInfo viewport_state = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .scissorCount = 1,
            };

            // растеризация: заливка треугольников, отсечение задних граней
            const VkPipelineRasterizationStateCreateInfo rasterization = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,    // примитивы не отбрасываются
                .polygonMode = VK_POLYGON_MODE_FILL,    // залитые треугольники
                .cullMode = VK_CULL_MODE_BACK_BIT,      // отсечение задних граней
                .frontFace = VK_FRONT_FACE_CLOCKWISE,   // обход треугольника для вектор. произв., чтобы показывать ту сторону, кот повернута к нам и не показ ту, кот отвернута (лицевая грань - по часовой)
                .depthBiasEnable = VK_FALSE,
                .lineWidth = 1.0f,
            };

            // мультисэмплинг (сглаживание на краях геометрических фигур): 1 sample
            const VkPipelineMultisampleStateCreateInfo multisample = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable = VK_FALSE,
            };

            // Depth-тест (определяет, какой из перекрывающих фрагментов находится ближе к камере, чтобы отрисовать его)
            // задние грани не перекрывают передние
            const VkPipelineDepthStencilStateCreateInfo depth_stencil = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .depthTestEnable = VK_TRUE, // не игнорировать ли проверку глубины перед закрашиванием
                .depthWriteEnable = VK_TRUE, // можно ли записать значение глубины, если проверка пройдена
                .depthCompareOp = VK_COMPARE_OP_LESS, // оператор сравнения глубины
                .depthBoundsTestEnable = VK_FALSE,
                .stencilTestEnable = VK_FALSE,
            };

            // цвет: без смешивания, все 4 канала
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

            // динамические состояния: viewport и scissor задаются в render
            const VkDynamicState dynamic_states[] = {
                VK_DYNAMIC_STATE_VIEWPORT, // размер рисуемой области
                VK_DYNAMIC_STATE_SCISSOR, // размер вырезаемой области
            };

            const VkPipelineDynamicStateCreateInfo dynamic_state = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = 2,
                .pDynamicStates = dynamic_states,
            };

            // сборка всего 
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
                .layout = vk_pipeline_layout,
                .renderPass = graphics::internal::context.render_pass,
                .subpass = 0,
            };

            // создание пайплайна
            if (vkCreateGraphicsPipelines(graphics::internal::context.device,
                VK_NULL_HANDLE, 1, &pipeline_info,
                nullptr, &vk_pipeline) != VK_SUCCESS) {
                std::cerr << "Failed to create graphics pipeline\n";
                // очистка шейдерных модулей перед выходом
                vkDestroyShaderModule(graphics::internal::context.device, vert_module, nullptr);
                vkDestroyShaderModule(graphics::internal::context.device, frag_module, nullptr);
                return false;
            }

            // пайплайн вмещает в себя шейдеры
            vkDestroyShaderModule(graphics::internal::context.device, vert_module, nullptr);
            vkDestroyShaderModule(graphics::internal::context.device, frag_module, nullptr);

            return true;
        }
    } // namespace

    // инициализация (создание ключевых объектов и загрузка необходимых функций)
    bool initialize() {
        // порядок: пайплайн зависит от дескрипторов, дескрипторы - от uniform-буферов
        if (!createVertexBuffer()) {
            return false;
        }
        if (!createIndexBuffer()) {
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

    // освобождение ресурсов и корректное завершение работы
    void shutdown() {
        auto& context = graphics::internal::context;
        // ожидание завершения работы GPU, т к нельзя удалять используемые ресурсы
        vkQueueWaitIdle(context.graphics_queue);

        // уничтожение в порядке, обратном созданию
        // уничтожение зшзудшту
        if (vk_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(context.device, vk_pipeline, nullptr);
            vk_pipeline = VK_NULL_HANDLE;
        }
        // уничтожение Pipeline layout, ссылается на set layout
        if (vk_pipeline_layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(context.device, vk_pipeline_layout, nullptr);
            vk_pipeline_layout = VK_NULL_HANDLE;
        }
        // уничтожение Pool, автоматическое уничтожение всех set'ов из него
        if (vk_descriptor_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(context.device, vk_descriptor_pool, nullptr);
            vk_descriptor_pool = VK_NULL_HANDLE;
            vk_scene_descriptor_set = VK_NULL_HANDLE;
            for (uint32_t i = 0; i < object_count; ++i) {
                vk_model_descriptor_sets[i] = VK_NULL_HANDLE;
            }
        }
        // уничтожение set layout
        if (vk_scene_descriptor_set_layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(context.device, vk_scene_descriptor_set_layout, nullptr);
            vk_scene_descriptor_set_layout = VK_NULL_HANDLE;
        }
        if (vk_model_descriptor_set_layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(context.device, vk_model_descriptor_set_layout, nullptr);
            vk_model_descriptor_set_layout = VK_NULL_HANDLE;
        }
        // уничтожение Scene uniform buffer (один)
        if (vk_scene_uniform_buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(context.allocator,
                vk_scene_uniform_buffer,
                vk_scene_uniform_buffer_allocation);
            vk_scene_uniform_buffer = VK_NULL_HANDLE;
            vk_scene_uniform_buffer_mapped = nullptr;
        }
        // уничтожение Model uniform buffers (по одному на объект)
        for (uint32_t i = 0; i < object_count; ++i) {
            if (vk_model_uniform_buffers[i] != VK_NULL_HANDLE) {
                vmaDestroyBuffer(context.allocator, vk_model_uniform_buffers[i], vk_model_uniform_buffer_allocations[i]);
                vk_model_uniform_buffers[i] = VK_NULL_HANDLE;
                vk_model_uniform_buffers_mapped[i] = nullptr;
            }
        }
        // уничтожение vertex и index буферов
        if (vk_vertex_buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(context.allocator, vk_vertex_buffer, vk_vertex_buffer_allocation);
            vk_vertex_buffer = VK_NULL_HANDLE;
        }
        if (vk_index_buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(context.allocator, vk_index_buffer, vk_index_buffer_allocation);
            vk_index_buffer = VK_NULL_HANDLE;
        }
    }

    // обновление состояния объектов, данных для каждого кадра
    void update(double time) {
        // размер окна и соотношение сторон
        const auto& extent = graphics::internal::context.swapchain_extent;
        const float aspect = float(extent.width) / float(extent.height); // aspect для правильной перспективы

        // состояние, сохраняется между кадрами, переменные не сбрасываются каждый кадр
        static int projection_mode = 0; // 0 = перспективная, 1 = ортографическая

        // какой объект сейчас выбран в UI (0, 1 или 2)
        static int selected_object = 0;

        // массивы параметров индивидуально для каждого из 3 объектов
        // смещение объектов
        static glm::vec3 manual_position[object_count] = {
            glm::vec3(0.0f, 0.0f, 0.0f),  // объект 0
            glm::vec3(-2.0f, 0.0f, 0.0f), // объект 1 (начальный сдвиг влево на 2)
            glm::vec3(2.0f, 0.0f, 0.0f)   // объект 2 (начальный сдвиг вправо на 2)
        };

        // поворот объектов
        static glm::vec3 manual_rotation[object_count] = {
            glm::vec3(0.0f),
            glm::vec3(0.0f, 45.0f, 0.0f),
            glm::vec3(0.0f, -30.0f, 0.0f)
        };

        // масштаб объектов
        static glm::vec3 scale[object_count] = {
            glm::vec3(1.0f),
            glm::vec3(0.8f),
            glm::vec3(0.6f)
        };

        // UI-цвет объектов
        static glm::vec3 color[object_count] = {
            glm::vec3(1.0f, 1.0f, 1.0f), // объект 0 — белый
            glm::vec3(1.0f, 0.5f, 0.2f), // объект 1 — оранжевый
            glm::vec3(0.2f, 0.5f, 1.0f)  // объект 2 — синий
        };

        // состояние анимации
        static bool  is_playing = true;   // идет ли анимация
        static float anim_speed = 1.0f;   // скорость анимации
        static float anim_radius = 0.6f;  // радиус траектории
        static float anim_height = 0.0f;  // высота траектории по Y
        static float anim_angle = 0.0f;   // текущий угол на траектории

        // deltaTime, разница времени между текущим и прерыдущим кадром
        // нужна, чтобы скорость анимации не зависела от FPS
        static double last_time = time; // время предыдущего кадра
        const double delta = time - last_time; // сколько секунд прошло с прошлого кадра
        last_time = time; // текущее время для следующего кадра

        // обновление угла анимации
        if (is_playing) {
            // рост угла пропорционален времени и скорости
            anim_angle += float(delta) * anim_speed;
        }

        // UI (ImGUI)
        ImGui::Begin("Controls"); // открытие окна с заголовком "Controls"

        // выбор объекта для редактирования
        ImGui::Text("Select Pyramid:");
        ImGui::RadioButton("Pyramid 0", &selected_object, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Pyramid 1", &selected_object, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Pyramid 2", &selected_object, 2);

        ImGui::Separator();

        // проекция
        ImGui::Text("Projection:");
        ImGui::RadioButton("Perspective", &projection_mode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Orthographic", &projection_mode, 1);

        ImGui::Separator();

        // трансформации, выставляются для выбранного объекта
        ImGui::Text("Manual transforms (Pyramid %d):", selected_object);
        ImGui::SliderFloat3("Position", &manual_position[selected_object].x, -5.0f, 5.0f); // сдвиг
        ImGui::SliderFloat3("Rotation", &manual_rotation[selected_object].x, -180.0f, 180.0f); // поворот
        ImGui::SliderFloat3("Scale", &scale[selected_object].x, 0.1f, 3.0f); // масштаб

        ImGui::Separator();

        // цвет для выбранного объекта
        ImGui::Text("Color:");
        ImGui::ColorEdit3("Base color", &color[selected_object].x);

        ImGui::Separator();
        // анимация
        ImGui::Text("Animation:");

        if (is_playing) {
            // кнопка паузы, останавливает анимацию
            if (ImGui::Button("||  Pause")) {
                is_playing = false;
            }
        }
        else {
            if (ImGui::Button(">  Play")) {
                is_playing = true;
            }
        } // кнопка "Play", запуск анимации

        ImGui::SliderFloat("Speed", &anim_speed, -3.0f, 3.0f); // скорость анимации
        ImGui::SliderFloat("Radius", &anim_radius, 0.0f, 3.0f); // радиус траектории
        ImGui::SliderFloat("Height", &anim_height, -1.0f, 1.0f); // высота траектории

        ImGui::End(); // закрытие окна

        // View-матрица, общая для всех объектов
        glm::mat4 view = glm::lookAt(
            glm::vec3(2.5f, 2.0f, 3.5f), // позиция камеры
            glm::vec3(0.0f, 0.0f, 0.0f), // куда смотрит (центр сцены)
            glm::vec3(0.0f, 1.0f, 0.0f) // "верх" по оси Y
        );

        // Proj-матрица, общая для всех объектов
        glm::mat4 proj;
        if (projection_mode == 0) {
            // перспективная: далекие объекты меньше
            // угол обзора, соотношение сторон окна, ближнаяя и дальняя плоскости отсечения
            proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        }
        else {
            // ортографическая: размеры не зависят от расстояния
            float ortho_size = 3.0f; // размер области видимости
            proj = glm::ortho(
                // левый, правый, нижний, верхний края, ближнаяя и дальняя плоскости отсечения
                -ortho_size * aspect, ortho_size * aspect,
                -ortho_size, ortho_size,
                0.1f, 100.0f
            );
        }
        proj[1][1] *= -1.0f; // NDC-Y идет вниз, иначе картинка получится перевернутой

        // заполнение Scene-буфера (общий для всей сцены)
        // view и proj одинаковы для всех объектов
        vk_scene_uniform_buffer_mapped->view = view;
        vk_scene_uniform_buffer_mapped->proj = proj;

        // расчет и запись для каждого объекта
        for (int i = 0; i < object_count; ++i) {
            glm::mat4 model = glm::mat4(1.0f); // изначально нет преобразований

            // итоговая позиция = ручное положение + движение по траектории
            glm::vec3 final_position = manual_position[i];

            // круговое движение для анимации
            // смещение по фазе, чтобы фигуры не летали друг в друге
            float phase_angle = anim_angle + float(i) * 2.094f; // 120 градусов
            final_position.x += anim_radius * std::cos(phase_angle);
            final_position.z += anim_radius * std::sin(phase_angle);
            // плавные вертикальные колебания по Y
            float wave_amplitude = 0.3f; // амплитуда колебаний по высоте
            final_position.y += anim_height + wave_amplitude * std::sin(phase_angle * 2.0f);

            // сдвиг в финальную позицию
            model = glm::translate(model, final_position);

            // повороты вокруг X, Y, Z, (вокруг Y в 2 раза быстрее)
            model = glm::rotate(model, glm::radians(manual_rotation[i].x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(manual_rotation[i].y) + anim_angle * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(manual_rotation[i].z), glm::vec3(0.0f, 0.0f, 1.0f));

            // масштабирование по всем осям
            model = glm::scale(model, scale[i]);


            // заполнение Model-буферов (свои для каждого объекта)
            // model и color свои у каждого объекта
            vk_model_uniform_buffers_mapped[i]->model = model;
            vk_model_uniform_buffers_mapped[i]->color = color[i];
        }
}


    // рендер (что и как отрисовывается в текущем кадре)
    // fd - данные текущего кадра: fd.framebuffer и fd.command_buffer (куда рисовать и куда записывать команды для GPU)
    void render(const graphics::internal::FrameData& fd) {
        // начало записи команд
        vkResetCommandBuffer(fd.command_buffer, 0); // очищение буфера от предыддущего кадра

        const VkCommandBufferBeginInfo command_buffer_begin = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // буфер используется один раз
        };

        // начало записи, все VkCmd* пишутся в буфер
        vkBeginCommandBuffer(fd.command_buffer, &command_buffer_begin);

        // значения очистки
        const VkClearValue clear_values[] = {
            {.color = {.float32 = { 0.1f, 0.1f, 0.1f, 1.0f } } }, // изображение №1: цветное, выставление RGBA во float
            {.depthStencil = { 1.0f, 0 } }, // изображение №2: глубина, выставление float=1, очень далеко, второй параметр - stencil, он равен 0
        };

        // начало render pass (определение ресурсов для рендеринга, их обработка и определение операций)
        const VkRenderPassBeginInfo render_pass_begin = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = graphics::internal::context.render_pass, // выбор render pass (какой объект для отрисовки используется)
            .framebuffer = fd.framebuffer, // в какой framebuffer (контейнер изображений) рисовать
            .renderArea = {.extent = graphics::internal::context.swapchain_extent }, // размер изображений, область рисования - размер окна
            .clearValueCount = sizeof(clear_values) / sizeof(clear_values[0]),
            .pClearValues = clear_values,
        };

        // начало render pass, начинаем рисовать
        vkCmdBeginRenderPass(fd.command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

        // динамические viewpoint и scissor, задаем значения областей отрисовки и рисования
        // Viewport - прямоугольник, куда будет отображаться NDC
        const VkViewport viewport = {
            .x = 0.0f, .y = 0.0f, // левый верхний угол
            .width = float(graphics::internal::context.swapchain_extent.width), // ширина - ширина окна
            .height = float(graphics::internal::context.swapchain_extent.height), // высота - высота окна
            .minDepth = 0.0f, .maxDepth = 1.0f, // диапазон глубины [0, 1]
        };

        // Scissor - область, за пределами которой пиксели не рисуются (совпадает с окном)
        const VkRect2D scissor = {
            .extent = graphics::internal::context.swapchain_extent,
        };

        vkCmdSetViewport(fd.command_buffer, 0, 1, &viewport);
        vkCmdSetScissor(fd.command_buffer, 0, 1, &scissor);

        // привязка общих ресурсов, задаем текущим новый графический конвейер (привязка набора дескрипторов к конвейеру)
        // пайплайн, вершинный и индексный буферы общие для всех объектов
        // установка пайплайна. GPU знает, как рисовать
        vkCmdBindPipeline(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);

        const VkDeviceSize vertex_buffer_offset = 0; // смещение в вершинном буфере (с начала 0)
        // привязка вершинного буфера к binding=0, количество, массив буферов, массив смещений
        vkCmdBindVertexBuffers(fd.command_buffer, 0, 1, &vk_vertex_buffer, &vertex_buffer_offset);
        // привязка индексного буфера
        vkCmdBindIndexBuffer(fd.command_buffer, vk_index_buffer, 0, VK_INDEX_TYPE_UINT32);

        // Scene set (set=0) общий для всех объектов
        vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            vk_pipeline_layout,
            0,  // первый set index
            1,  // сколько всего set'ов
            &vk_scene_descriptor_set,
            0, nullptr);

        // отрисовка каждого объекта со своим Model set (set=1)
        for (uint32_t i = 0; i < object_count; ++i) {
            // для каждого объекта свой descriptor set, свой uniform-буфер
            // привязка descriptor set объекта i
            vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                vk_pipeline_layout, 1, 1, &vk_model_descriptor_sets[i],
                0, nullptr);

            vkCmdDrawIndexed(fd.command_buffer,
                sizeof(pyramid_indices) / sizeof(pyramid_indices[0]), // количество индексов
                1, 0, 0, 0); // количество экземпляров, все смещения с 0
        }

        vkCmdEndRenderPass(fd.command_buffer); // выход из render pass
        vkEndCommandBuffer(fd.command_buffer); // конец записи, буфер готов к submit (запуск команд на GPU)
    }

} // namespace application
#version 450 core

// входные атрибуты вершины
layout(location = 0) in vec3 in_position; // позиция вершины в локальном пространстве (из вершинного буфера)
layout(location = 1) in vec3 in_color; // цвет вершины из буфера

// выходной атрибут, передается во фрагментный шейдер
layout(location = 0) out vec3 out_color; // цвет, который увидит фрагментный шейдер

//uniform-буфер, данные всей сцены, общие для всех объектов
layout(std140, set = 0, binding = 0) uniform SceneUniforms {
    mat4 view;   // из мировых координат в координаты камеры
    mat4 proj;   // из координат камеры в NDC (Normalized Device Coordinates, то, что видит GPU)
} scene_uniforms;

// Uniform-буфер, данные конкретного объекта
layout(std140, set = 1, binding = 0) uniform ModelUniform {
    mat4 model; // из локальных координат в мировые (сдвиг, поворот, масштаб)
    vec3 color; // UI-цвет
    float _padding; // выравнивание (std140, 16 байт)
} model_uniforms;

void main() {
    // сохранение финальной позиции
    gl_Position = scene_uniforms.proj
                * scene_uniforms.view
                * model_uniforms.model
                * vec4(in_position, 1.0);
    // процедурный цвет: нормализация локальной позиции в [0, 1]
    // color = (position - min) / (max - min)
    out_color = (in_position + vec3(0.5, 0.0, 0.5)) / vec3(1.0, 0.7, 1.0);
}
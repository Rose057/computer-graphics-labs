#version 450 core

// входной атрибут, приходит из вершинного шейдера
// значение интерполируется между вершинами треугольника
layout(location = 0) in vec3 in_color;
layout(location = 0) out vec4 out_color; // выходной цвет, идет в framebuffer

// Uniform-буфер, нужен для получения UI-цвета и умножения его на процедурный
layout(std140, set = 0, binding = 0) uniform GlobalUniforms {
    mat4 model; // из локальных координат в мировые (сдвиг, поворот, масштаб)
    mat4 view; // из мировых координат в координаты камеры
    mat4 proj; // из координат камеры в NDC (Normalized Device Coordinates, то, что видит GPU)
    vec3 color; // UI-цвет
    float _padding; // выравнивание
} global_uniforms;

void main() {
    // умножение процедурного цвета вершины на UI-цвет
    // in_color - градиент от процедурной формулы
    // global_uniforms.color - цвет, выбранный через ColorEdit
    out_color = vec4(in_color * global_uniforms.color, 1.0);
}
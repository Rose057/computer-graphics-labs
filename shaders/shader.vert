#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_world_position;

layout(std140, set = 0, binding = 0) uniform SceneUniforms {
    mat4 view_projection;
    vec3 camera_position;
    float _padding0;
    vec3 directional_light_direction;
    float _padding1;
    vec3 directional_light_color;
    float directional_light_intensity;
    vec3 ambient_light;
    float _padding2;
    uint point_light_count;
    uint spot_light_count;
    float _padding3;
    float _padding4;
} scene_uniforms;

layout(std140, set = 1, binding = 0) uniform ModelUniform {
    mat4 model;
    vec3 albedo;
    float _padding0;
    vec3 specular;
    float alpha;
    float is_gizmo;
    float _padding1;
    float _padding2;
    float _padding3;
} model_uniforms;

void main() {
    vec4 world_pos = model_uniforms.model * vec4(in_position, 1.0);
    gl_Position = scene_uniforms.view_projection * world_pos;

    out_color = in_position + vec3(0.5, 0.5, 0.5);
    out_normal = mat3(transpose(inverse(model_uniforms.model))) * in_normal;
    out_world_position = world_pos.xyz;
}
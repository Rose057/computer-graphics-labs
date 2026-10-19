#version 450 core

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_world_position;

layout(location = 0) out vec4 out_color;

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

struct PointLight {
    vec3 position;
    float _padding0;
    vec3 color;
    float intensity;
};

layout(std430, set = 0, binding = 1) buffer PointLights {
    PointLight point_lights[];
};

struct SpotLight {
    vec3 position;
    float _padding0;
    vec3 direction;
    float _padding1;
    vec3 color;
    float intensity;
    float inner_cosine;
    float outer_cosine;
    float inner_angle_deg;
    float outer_angle_deg;
};

layout(std430, set = 0, binding = 2) buffer SpotLights {
    SpotLight spot_lights[];
};

void main() {
    if (model_uniforms.is_gizmo > 0.5) {
        out_color = vec4(model_uniforms.albedo, 1.0);
        return;
    }

    vec3 N = normalize(in_normal);
    vec3 V = normalize(scene_uniforms.camera_position - in_world_position);
    vec3 material_albedo = model_uniforms.albedo * in_color;
    vec3 final_color = scene_uniforms.ambient_light * material_albedo;

    vec3 L_dir = normalize(-scene_uniforms.directional_light_direction);
    float NdotL_dir = max(dot(N, L_dir), 0.0);
    if (NdotL_dir > 0.0) {
        vec3 H_dir = normalize(L_dir + V);
        float NdotH_dir = max(dot(N, H_dir), 0.0);
        vec3 diffuse_dir = scene_uniforms.directional_light_color
                         * scene_uniforms.directional_light_intensity
                         * material_albedo * NdotL_dir;
        vec3 specular_dir = scene_uniforms.directional_light_color
                          * scene_uniforms.directional_light_intensity
                          * model_uniforms.specular
                          * pow(NdotH_dir, model_uniforms.alpha);
        final_color += diffuse_dir + specular_dir;
    }

    for (uint i = 0; i < scene_uniforms.point_light_count; ++i) {
        PointLight light = point_lights[i];
        vec3 to_light = light.position - in_world_position;
        float distance_squared = max(dot(to_light, to_light), 0.0001);
        float distance_to_light = sqrt(distance_squared);
        vec3 L = to_light / distance_to_light;
        float NdotL = max(dot(N, L), 0.0);

        if (NdotL > 0.0) {
            vec3 H = normalize(L + V);
            float NdotH = max(dot(N, H), 0.0);
            float attenuation = light.intensity / distance_squared;
            vec3 diffuse = light.color * material_albedo * NdotL * attenuation;
            vec3 specular = light.color * model_uniforms.specular
                          * pow(NdotH, model_uniforms.alpha) * attenuation;
            final_color += diffuse + specular;
        }
    }

    for (uint i = 0; i < scene_uniforms.spot_light_count; ++i) {
        SpotLight light = spot_lights[i];
        vec3 to_light = light.position - in_world_position;
        float distance_squared = max(dot(to_light, to_light), 0.0001);
        float distance_to_light = sqrt(distance_squared);
        vec3 L = to_light / distance_to_light;
        vec3 light_direction = normalize(light.direction);
        float theta = dot(-L, light_direction);
        float spot_intensity = smoothstep(light.outer_cosine, light.inner_cosine, theta);
        float NdotL = max(dot(N, L), 0.0);

        if (NdotL > 0.0 && spot_intensity > 0.0) {
            vec3 H = normalize(L + V);
            float NdotH = max(dot(N, H), 0.0);
            float attenuation = light.intensity / distance_squared;
            vec3 diffuse = light.color * material_albedo * NdotL
                         * attenuation * spot_intensity;
            vec3 specular = light.color * model_uniforms.specular
                          * pow(NdotH, model_uniforms.alpha)
                          * attenuation * spot_intensity;
            final_color += diffuse + specular;
        }
    }

    out_color = vec4(final_color, 1.0);
}

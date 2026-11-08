#version 450 core

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_world_position;
layout(location = 3) in vec2 in_uv;

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
    float _padding1;
    float _padding2;
    float _padding3;
    float _padding4;
} model_uniforms;

struct PointLight {
    vec3 position;
    float _padding0;
    vec3 color;
    float intensity;
};

layout(std430, set = 0, binding = 1) readonly buffer PointLights {
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

layout(std430, set = 0, binding = 2) readonly buffer SpotLights {
    SpotLight spot_lights[];
};

layout(set = 2, binding = 0) uniform sampler2D albedo_texture;
layout(set = 2, binding = 1) uniform sampler2D specular_texture;
layout(set = 2, binding = 2) uniform sampler2D emissive_texture;

layout(push_constant) uniform PushConstants {
    float time;
    float enable_effect;
} pc;

void main() {
    vec2 uv = in_uv;
    vec4 texel;
    vec4 texel_albedo = texture(albedo_texture, uv);
    vec3 texel_specular = texture(specular_texture, uv).rgb;
    vec3 texel_emissive = texture(emissive_texture, uv).rgb;

    if (pc.enable_effect > 2.5) {
        vec2 distorted_uv = in_uv;
        distorted_uv.x += sin(in_uv.y * 15.0 + pc.time * 2.0) * 0.005;
        distorted_uv.y += cos(in_uv.x * 15.0 + pc.time * 2.0) * 0.005;

        texel = texture(albedo_texture, distorted_uv);
        float safe_time = mod(pc.time, 100.0);
        float wave_line = sin((in_uv.x + in_uv.y) * 3.0 - safe_time * 5.0);
        float glow_intensity = smoothstep(0.2, 0.9, wave_line) * 0.5;
        vec3 white_glow = vec3(1.2, 1.4, 2.0) * glow_intensity;
        texel.rgb += white_glow;

    } else if (pc.enable_effect > 1.5) {
        vec2 distorted_uv = in_uv;
        distorted_uv.x += sin(in_uv.y * 15.0 + pc.time * 2.0) * 0.005;
        distorted_uv.y += cos(in_uv.x * 15.0 + pc.time * 2.0) * 0.005;

        vec4 tex1 = texture(albedo_texture, distorted_uv);
        vec4 tex2 = texture(albedo_texture, distorted_uv + vec2(0.015, 0.0));
        vec4 tex3 = texture(albedo_texture, distorted_uv + vec2(0.0, 0.015));
        
        texel = (tex1 + tex2 + tex3) / 3.0;

        float safe_time = mod(pc.time, 100.0);
        float sparkle = fract(sin(dot(in_uv, vec2(12.9898, 78.233)) + safe_time * 0.00001) * 4375.5453);
        sparkle = step(0.96, sparkle) * (0.5 + 0.5 * sin(safe_time * 2.0));
        vec3 sparkle_color = vec3(1.2, 1.2, 1.5) * sparkle;
        texel.rgb += sparkle_color;

    } else if (pc.enable_effect > 0.5) {
        vec2 distorted_uv = in_uv;
        distorted_uv.x += sin(in_uv.y * 20.0 + pc.time * 2.5) * 0.01;
        distorted_uv.y += cos(in_uv.x * 20.0 + pc.time * 2.5) * 0.01;

        texel = texture(albedo_texture, distorted_uv);
        float pulse = 0.5 + 0.5 * sin(pc.time * 3.0);
        float center_dist = length(in_uv - vec2(0.5));
        float glow_mask = smoothstep(0.5, 0.0, center_dist) * 1.0;
        vec3 rune_emissive = vec3(0.4, 0.2, 0.9) * (pulse * 0.7 + 0.3) * glow_mask;
        texel.rgb += rune_emissive;

    } else {
        texel = texture(albedo_texture, in_uv);
    }

    vec3 N = normalize(in_normal);
    vec3 V = normalize(scene_uniforms.camera_position - in_world_position);
    vec3 material_albedo = model_uniforms.albedo * texel.rgb;
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
                          * (model_uniforms.specular * texel_specular)
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
            vec3 specular = light.color * (model_uniforms.specular * texel_specular)
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
            vec3 specular = light.color * (model_uniforms.specular * texel_specular)
                          * pow(NdotH, model_uniforms.alpha)
                          * attenuation * spot_intensity;
            final_color += diffuse + specular;
        }
    }

    final_color += texel_emissive;

    out_color = vec4(final_color, 1.0);
}
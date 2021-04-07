// Copyright (C) 2018-2021 Mariusz Dzikowski (thecyste@gmail.com)
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define PI 3.14159265359

#define COLOR_MAP_BIT     1
#define NORMAL_MAP_BIT    2
#define ROUGHNESS_MAP_BIT 4
#define METALLIC_MAP_BIT  8
#define SKYBOX_MAP_BIT    16

struct Light {
	int type;
	vec3 position;
	vec3 direction;
	vec3 color;
	float intensity;
	float radius;
	float angle;
};

layout (location = 0) in vec3 varNormal;
layout (location = 1) in vec2 varTexcoord;
layout (location = 2) in vec3 varPosition;

layout (binding = 0) uniform Data {
	vec3 diffuse;
	float roughness;
	float metallic;

	bool hasDiffuseMap;
	bool hasNormalMap;
	bool hasRoughnessMap;
	bool hasMetallicMap;

	vec3 lightDirection;
	float lightIntensity;
	vec3 lightColor;
	vec3 cameraPosition;

	Light lights[16];
	int lightCount;

	mat4 lightProjView;
	mat4 invProjView;

	mat4 view;
	mat4 proj;
	mat4 projView;
	mat4 world;
};

layout (binding = 1) uniform sampler2D diffuseMap;
layout (binding = 2) uniform sampler2D normalMap;
layout (binding = 3) uniform sampler2D roughnessMap;
layout (binding = 4) uniform sampler2D metallicMap;
layout (binding = 5) uniform samplerCube irradianceMap;
layout (binding = 6) uniform samplerCube prefilterMap;
layout (binding = 7) uniform sampler2D   lutMap;
layout (binding = 8) uniform sampler2D shadowMap;

layout (location = 0) out vec4 out_color;

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   

float computeShadow() {
	float shadow = 1.0;
	
	vec4 shadowPosition = lightProjView * vec4(varPosition, 1.0);
	shadowPosition.xyz = (shadowPosition.xyz / shadowPosition.w) * 0.5 + 0.5;
	
	vec2 shadowTexCoord = vec2(shadowPosition.x, shadowPosition.y);
	
	shadowPosition.z -= 0.005;
	
	if (shadowPosition.x > 0.0 && shadowPosition.x < 1.0 &&
		shadowPosition.y > 0.0 && shadowPosition.y < 1.0&&
		shadowPosition.z > 0.0 && shadowPosition.z < 1.0) {
		vec2 texel = vec2(1.0 / 2048.0, 1.0 / 2048.0);
		
		shadow = step(shadowPosition.z, texture(shadowMap, shadowPosition.xy).x);
		// shadow = texture(shadowMap, shadowPosition.xyz);
		
//		shadow += texture(shadowMap, vec3(shadowPosition.x + texel.x, shadowPosition.y + texel.y, shadowPosition.z - 0.001));
//		shadow += texture(shadowMap, vec3(shadowPosition.x - texel.x, shadowPosition.y + texel.y, shadowPosition.z - 0.001));
//		shadow += texture(shadowMap, vec3(shadowPosition.x + texel.x, shadowPosition.y - texel.y, shadowPosition.z + 0.001));
//		shadow += texture(shadowMap, vec3(shadowPosition.x - texel.x, shadowPosition.y - texel.y, shadowPosition.z + 0.001));
//		
//		shadow += texture(shadowMap, vec3(shadowPosition.x + texel.x, shadowPosition.y, shadowPosition.z));
//		shadow += texture(shadowMap, vec3(shadowPosition.x - texel.x, shadowPosition.y, shadowPosition.z));
//		shadow += texture(shadowMap, vec3(shadowPosition.x, shadowPosition.y + texel.y, shadowPosition.z - 0.001));
//		shadow += texture(shadowMap, vec3(shadowPosition.x, shadowPosition.y - texel.y, shadowPosition.z + 0.001));
//		
//		shadow += texture(shadowMap, vec3(shadowPosition.x + texel.x * 2.0, shadowPosition.y + texel.y * 2.0, shadowPosition.z - 0.001));
//		shadow += texture(shadowMap, vec3(shadowPosition.x - texel.x * 2.0, shadowPosition.y + texel.y * 2.0, shadowPosition.z - 0.001));
//		shadow += texture(shadowMap, vec3(shadowPosition.x + texel.x * 2.0, shadowPosition.y - texel.y * 2.0, shadowPosition.z + 0.001));
//		shadow += texture(shadowMap, vec3(shadowPosition.x - texel.x * 2.0, shadowPosition.y - texel.y * 2.0, shadowPosition.z + 0.001));
//		
//		shadow += texture(shadowMap, vec3(shadowPosition.x + texel.x * 2.0, shadowPosition.y, shadowPosition.z));
//		shadow += texture(shadowMap, vec3(shadowPosition.x - texel.x * 2.0, shadowPosition.y, shadowPosition.z));
//		shadow += texture(shadowMap, vec3(shadowPosition.x, shadowPosition.y + texel.y * 2.0, shadowPosition.z - 0.001));
//		shadow += texture(shadowMap, vec3(shadowPosition.x, shadowPosition.y - texel.y * 2.0, shadowPosition.z + 0.001));
//		
//		shadow /= 17.0;
	}

	return shadow;
}

mat3 cotangentFrame(vec3 N, vec3 p, vec2 uv) {
  // get edge vectors of the pixel triangle
  vec3 dp1 = dFdx(p);
  vec3 dp2 = dFdy(p);
  vec2 duv1 = dFdx(uv);
  vec2 duv2 = dFdy(uv);

  // solve the linear system
  vec3 dp2perp = cross(dp2, N);
  vec3 dp1perp = cross(N, dp1);
  vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
  vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

  // construct a scale-invariant frame 
  float invmax = 1.0 / sqrt(max(dot(T,T), dot(B,B)));
  return mat3(T * invmax, B * invmax, N);
}

vec3 perturb(vec3 map, vec3 N, vec3 V, vec2 texcoord) {
  mat3 TBN = cotangentFrame(N, -V, texcoord);
  return normalize(TBN * map);
}

void main() {
	vec3 albedo = diffuse;
	if (hasDiffuseMap) {
		albedo *= texture(diffuseMap, vec2(varTexcoord.x, varTexcoord.y)).rgb;
	}

	float r = roughness;
	if (hasRoughnessMap) {
		r = texture(roughnessMap, varTexcoord).r;
	}

	float m = metallic;
	if (hasMetallicMap) {
		m = texture(metallicMap, varTexcoord).r;
	}
	
    vec3 V = normalize(cameraPosition - varPosition);
	vec3 N = normalize(varNormal);
	
	if (hasNormalMap) {
		vec3 normal = texture(normalMap, varTexcoord).rgb * 2.0 - 1.0;
		
		N = perturb(normal, N, V, varTexcoord);
	}
	
	float NdotV = abs(dot(N, V)) + 1e-5;
	
	vec3 F0 = mix(vec3(0.04), albedo, m);

	vec3 Lo = vec3(0.0);
	for (int i = 0; i < lightCount; ++i) {
		vec3 radiance = lights[i].color;

		vec3 L;
		if (lights[i].type == 0) {
			L = -normalize(lights[i].direction); 
		} else if (lights[i].type == 1) {
			L = normalize(lights[i].position - varPosition); 
        	float distance = length(lights[i].position - varPosition);
			float attenuation = pow(clamp(1.0 - pow(distance / lights[i].radius, 4.0), 0.0, 1.0), 2.0) / (distance * distance + 1.0);
			radiance *= attenuation;
		} else if (lights[i].type == 2) {
			L = normalize(lights[i].position - varPosition); 
        	float distance = length(lights[i].position - varPosition);
			float attenuation = pow(clamp(1.0 - pow(distance / lights[i].radius, 4.0), 0.0, 1.0), 2.0) / (distance * distance + 1.0);

			float lightToSurfaceAngle = acos(dot(-L, lights[i].direction));
			if (lightToSurfaceAngle > lights[i].angle) {
				attenuation = 0.0;
			}

			radiance *= attenuation;
		}

		vec3 H = normalize(V + L);
	
		vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
		
		vec3 kS = F;
		vec3 kD = (1.0 - kS) * (1.0 - m); 

		float NDF = distributionGGX(N, H, r);   
		float G   = geometrySmith(N, V, L, r);    

		float NdotL = max(dot(N, L), 0.0); 


 // in float NdL, in float NdV, in float NdH, in vec3 specular, in float roughness)
		vec3 nominator  = NDF * G * F; 
		float denominator = 4.0 * NdotV * NdotL + 0.001;
		vec3 specular = nominator / denominator;

		if (i == 0) {
			Lo += (kD * albedo / PI + specular)  * radiance * lights[i].intensity * NdotL;
		} else {
			Lo += (kD * albedo / PI + specular)  * radiance * lights[i].intensity * NdotL;
		}
	}

	vec3 kS = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, r); 
	vec3 kD = (1.0 - kS) * (1.0 - m);
	
	vec3 irradiance = texture(irradianceMap, N).rgb;
	vec3 diff       = irradiance * albedo;
	
	vec3 reflected = reflect(-V, N);

    vec3 prefilteredColor = textureLod(prefilterMap, reflected, r * 5.0).rgb;    
	vec2 brdf             = texture(lutMap, vec2(NdotV, r)).rg;
    vec3 spec             = prefilteredColor * (kS * brdf.x + brdf.y);
	
    vec3 ambient = (kD * diff + spec);

	out_color = vec4(ambient + Lo, 1.0);

/*
	vec3 Lo = (kD * albedo / PI + specular) * lightColor * lightIntensity * NdotL;// * shadow;

	kS = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness); 

	vec3 prefilteredColor = textureLod(prefilterMap, reflect(-V, N), roughness * 3.0).rgb;    
	vec2 brdf             = texture(lutMap, vec2(max(NdotV, 0.0), roughness)).rg;
    vec3 spec             = prefilteredColor * (kS * brdf.x + brdf.y) * lightIntensity;
	
	outColor = vec4(Lo + spec, 0.0);
*/
	//outColor = vec4(F, 0.0);
	//outColor = vec4(depth, depth, depth, 0.0);
}
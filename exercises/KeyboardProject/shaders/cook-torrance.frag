#version 330 core

in vec3 WorldPosition;
in vec3 WorldTangent;
in vec2 TexCoord;
in vec3 WorldNormal;

out vec4 FragColor;

uniform vec4 Color;
uniform vec3 AmbientColor;
uniform vec3 LightColor;
uniform vec3 LightPosition;
uniform vec3 CameraPosition;

uniform float Roughness;
uniform float Metallic;
uniform float HeightScale;

uniform sampler2D NormalTexture;
uniform sampler2D ColorTexture;
//uniform sampler2D HeightTexture;
//uniform sampler2D RoughnessTexture;
//uniform sampler2D EmissiveTexture;  // These texture maps are packed into the RGB channels of PackedTexture
uniform sampler2D PackedTexture;

uniform int EffectMode;
uniform float EffectSpeed;
uniform int WaveDirection; // 0 for X-axis, 1 for Z-axis
uniform float GlowIntensity; // Overall intensity of the glow effect
uniform vec3 BreathingColor; // Color for the breathing effect, can be set from the application

uniform float Time;

const float PI = 3.14159265359;

float DistributionGGX(vec3 normalVector, vec3 halfVector, float roughness) 
{
	float a2 = pow(roughness, 4.0);
	float NdotH = max(dot(normalVector, halfVector), 0.0);
	float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;

	return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) 
{
	float k    = pow(roughness + 1.0, 2.0) / 8.0;
	float nom   = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;

}

float GeometrySmith(vec3 normalVector, vec3 viewVector, vec3 lightVector, float roughness) 
{	
	float NdotV = max(dot(normalVector, viewVector), 0.0);
	float NdotL = max(dot(normalVector, lightVector), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;

}

vec3 FresnelSchlick(float cosTheta, vec3 F0) 
{	
	return F0 + (1.0 - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

vec3 GetAmbientReflection(vec3 objectColor)
{
	vec3 F0 = mix(vec3(0.04), objectColor, Metallic);
	vec3 kD = (1.0 - F0) * (1.0 - Metallic);
	return AmbientColor * kD * objectColor * 0.1;
}

vec3 GetCookTorranceReflection(vec3 objectColor, vec3 lightVector, vec3 viewVector, vec3 normalVector, float pixelRoughness) 
{
	vec3 halfVector = normalize(lightVector + viewVector);
	vec3 F0 = mix(vec3(0.04f), objectColor, Metallic);

	float NDF = DistributionGGX(normalVector, halfVector, pixelRoughness);
	float G   = GeometrySmith(normalVector, viewVector, lightVector, pixelRoughness);
	vec3 F    = FresnelSchlick(max(dot(halfVector, viewVector), 0.0), F0);

	vec3 nominator    = NDF * G * F;
	float denominator = 4.0 * max(dot(normalVector, viewVector), 0.0) * max(dot(normalVector, lightVector), 0.0) + 0.001;
	vec3 specular = nominator / denominator;

	vec3 kD = (vec3(1.0) - F) * (1.0 - Metallic);

	float NdotL = max(dot(normalVector, lightVector), 0.0);
	return (kD * objectColor / PI + specular) * LightColor * NdotL;
}

vec3 hsv2rgb(vec3 c)  // Convert HSV to RGB
{
	vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
	// Construct TBN matrix
	vec3 N = normalize(WorldNormal);
	vec3 T = normalize(WorldTangent);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);

	// Transform camera position and fragment position to tangent space
	mat3 invTBN = transpose(TBN);
	vec3 tangentCameraPos = invTBN * CameraPosition;
	vec3 tangentFragPos = invTBN * WorldPosition;
	vec3 tangentViewDir = normalize(tangentCameraPos - tangentFragPos);

	// Parallax Mapping
	float height = texture(PackedTexture, TexCoord).b;
	vec2 shift = (tangentViewDir.xy / tangentViewDir.z) * ((1.0-height) * HeightScale);
	vec2 finalTexCoord = TexCoord - shift;

	// Sample packed texture for glow mask and roughness
	vec3 packedData = texture(PackedTexture, finalTexCoord).rgb;
	float glowMask = packedData.r;
	float finalRoughness = Roughness * packedData.g;

	// Sample color texture and apply gamma correction to get linear color space
	vec4 texColor = texture(ColorTexture, finalTexCoord);
	vec3 objectColor = pow(texColor.rgb, vec3(2.2)); 

	// Generate normals from height map 
	vec2 texelSize = 1.0 / textureSize(PackedTexture, 0);

	// Sample heights from neighboring texels
	float heightL = texture(PackedTexture, finalTexCoord - vec2(texelSize.x, 0.0)).b;
	float heightR = texture(PackedTexture, finalTexCoord + vec2(texelSize.x, 0.0)).b;
	float heightD = texture(PackedTexture, finalTexCoord - vec2(0.0, texelSize.y)).b;
	float heightU = texture(PackedTexture, finalTexCoord + vec2(0.0, texelSize.y)).b;

	vec3 generatedNormal = normalize(vec3(heightL - heightR, heightD - heightU, 2.0));
	vec3 finalWorldNormal = normalize(TBN * generatedNormal);

	// Sample normal map and transform to world space -- Without 
	//vec3 normalMap = texture(NormalTexture, finalTexCoord).rgb; 
	//normalMap = normalize(normalMap * 2.0 - 1.0); 
	//vec3 normalVector = normalize(TBN * normalMap);

	// Calculate lighting vectors
	vec3 lightVector = normalize(LightPosition - WorldPosition);
	vec3 viewVector = normalize(CameraPosition - WorldPosition);
	
	vec3 finalColor = GetAmbientReflection(objectColor) + GetCookTorranceReflection(objectColor, lightVector, viewVector, finalWorldNormal, finalRoughness);
	
	// Apply glow effects based on the selected mode
	vec3 glowColor = vec3(0.0);
	
	if (EffectMode == 1) // Breathing effect
	{
		float breathingIntensity = (sin(Time * 3.0) + 1.0) * 0.5;
		glowColor = BreathingColor * breathingIntensity * 0.1;
	}
	else if (EffectMode == 2) // Rainbow effect
	{
		float effectDirection = (WaveDirection == 0) ? WorldPosition.x : WorldPosition.z;

		float waveDensity = 0.7;
		effectDirection *= waveDensity;

		float currentHue = fract(effectDirection - (Time * EffectSpeed));

		glowColor += hsv2rgb(vec3(currentHue, 1.0, 1.0)) * 0.5;
	}
	else if (EffectMode == 3) // Cycle effect
	{
		float cycleTimer = Time * EffectSpeed;

		float phase = fract(cycleTimer);
		float intensity = (sin(phase * 2.0 * PI - (PI / 2.0)) + 1.0) * 0.5;

		float colorStep = floor(cycleTimer);

		float hue = fract(colorStep * 0.381);

		glowColor = hsv2rgb(vec3(hue, 1.0, 1.0)) * intensity * 0.5;
	}

	// Combine glow color with the base color using the glow mask and intensity
	finalColor += (glowColor * GlowIntensity) * glowMask;

	// Apply tone mapping and gamma correction
	finalColor = finalColor / (finalColor + vec3(1.0)); 
	finalColor = pow(finalColor, vec3(1.0/2.2));	


	FragColor = vec4(finalColor, texColor.a);
	
}
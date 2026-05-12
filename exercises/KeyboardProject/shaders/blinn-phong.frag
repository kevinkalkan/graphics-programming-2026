#version 330 core

in vec3 WorldPosition;
in vec3 WorldNormal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec4 Color;
uniform sampler2D ColorTexture;
uniform sampler2D RoughnessTexture;

uniform float AmbientReflection;
uniform float DiffuseReflection;
uniform float SpecularReflection;
uniform float SpecularExponent;

uniform vec3 AmbientColor;
uniform vec3 LightColor;
uniform vec3 LightPosition;
uniform vec3 CameraPosition;

vec3 GetAmbientReflection(vec3 objectColor)
{
	return AmbientColor * AmbientReflection * objectColor;
}

vec3 GetDiffuseReflection(vec3 objectColor, vec3 lightVector, vec3 normalVector)
{
	return LightColor * DiffuseReflection * objectColor * max(dot(lightVector, normalVector), 0.0f);
}

vec3 GetSpecularReflection(vec3 lightVector, vec3 viewVector, vec3 normalVector, float shininess)
{
	vec3 halfVector = normalize(lightVector + viewVector);
	return LightColor * SpecularReflection * pow(max(dot(halfVector, normalVector), 0.0f), shininess);
}

vec3 GetBlinnPhongReflection(vec3 objectColor, vec3 lightVector, vec3 viewVector, vec3 normalVector, float shininess)
{
	return GetAmbientReflection(objectColor)
		 + GetDiffuseReflection(objectColor, lightVector, normalVector)
		 + GetSpecularReflection(lightVector, viewVector, normalVector, shininess);
}

void main()
{
	vec4 objectColor = Color * texture(ColorTexture, TexCoord);
	vec3 lightVector = normalize(LightPosition - WorldPosition);
	vec3 viewVector = normalize(CameraPosition - WorldPosition);
	vec3 normalVector = normalize(WorldNormal);

	float textureRoughness = texture(RoughnessTexture, TexCoord).r;
	float shininess = exp2(10.0f * (1.0f - textureRoughness));
	FragColor = vec4(GetBlinnPhongReflection(objectColor.rgb, lightVector, viewVector, normalVector, shininess), 1.0f);
}

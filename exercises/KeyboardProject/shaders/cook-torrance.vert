#version 330 core

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec3 VertexTangent;

out vec3 WorldPosition;
out vec3 WorldNormal;
out vec2 TexCoord;
out vec3 WorldTangent;

uniform mat4 WorldMatrix;
uniform mat4 ViewProjMatrix;

void main()
{
	vec4 worldPosition = WorldMatrix * vec4(VertexPosition, 1.0);
	WorldPosition = worldPosition.xyz;

	mat3 normalMatrix = mat3(WorldMatrix);
	WorldNormal = normalize(normalMatrix * VertexNormal);
	WorldTangent = normalize(normalMatrix * VertexTangent);

	TexCoord = VertexTexCoord;
	gl_Position = ViewProjMatrix * vec4(WorldPosition, 1.0);

}

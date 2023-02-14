#version 330 core

layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in int a_EntityID;

out vec2 v_TexCoords;
out int v_EntityID;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
	//gl_Position = vec4(a_Position.x, a_Position.y, a_Position.z, 1.0);
	gl_Position = u_Projection * u_View * u_Model * a_Position;
	// gl_Position = inverse(u_Model) * a_Position;
	v_TexCoords = a_TexCoords;
	v_EntityID = a_EntityID;
}
#version 330 core

layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec4 a_Colour;
layout(location = 2) in int a_EntityID;

out vec4 v_Colour;
flat out int v_EntityID;

uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
	gl_Position = u_Projection * u_View * a_Position;
	v_Colour = a_Colour;
	v_EntityID = a_EntityID;
}
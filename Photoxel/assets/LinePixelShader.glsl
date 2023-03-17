#version 330 core

layout(location = 0) out vec4 o_FragColor;
layout(location = 1) out int o_EntityID;

in vec4 v_Colour;
flat in int v_EntityID;

void main()
{
    o_FragColor = v_Colour;
}
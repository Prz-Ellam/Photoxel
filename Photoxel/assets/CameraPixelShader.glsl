#version 330 core

layout(location = 0) out vec4 o_FragColor;

in vec2 v_TexCoords;

uniform sampler2D u_Texture;
uniform sampler2D u_PrevTexture;
uniform int u_Movement;

void main() {
    vec4 prev = texture(u_PrevTexture, v_TexCoords);
    vec4 actual = texture(u_Texture, v_TexCoords);

    if (u_Movement == 0) {
        o_FragColor = actual;
        return;
    }

    vec3 diff = vec3(prev.rgb - actual.rgb);
    vec4 newDiff = (length(diff) > 0.4f) ? vec4(1.0f, 0.0f, 0.0f, 1.0f) : vec4(0.0f);
	o_FragColor = newDiff;

    if (newDiff.a == 0.0f) {
        o_FragColor = actual;
    }
}

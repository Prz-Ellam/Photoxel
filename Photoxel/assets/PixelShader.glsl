#version 330 core

layout(location = 0) out vec4 o_FragColor;

in vec2 v_TexCoords;

uniform sampler2D u_Texture;
/*{{HEADER}}*/

vec4 negative(vec4 fragColor) {
	return vec4(1.0f - fragColor.rgb, fragColor.a);
}

vec4 grayscale(vec4 fragColor) {
 	float grayFactor = 0.2199f * fragColor.r + 0.7152f * fragColor.g + 0.0722f * fragColor.b;
 	return vec4(vec3(grayFactor), fragColor.a);
}

vec4 sepia(vec4 fragColor) {
	fragColor.r = min(255.0f, fragColor.r * 0.393f + fragColor.g * 0.769f + fragColor.b * 0.189f);
	fragColor.g = min(255.0f, fragColor.r * 0.349f + fragColor.g * 0.686f + fragColor.b * 0.168f);
	fragColor.b = min(255.0f, fragColor.r * 0.272f + fragColor.g * 0.534f + fragColor.b * 0.131f);
	return fragColor;
}

vec4 brightness(vec4 fragColor, float range) {
	fragColor.rgb *= range;
	fragColor.rgb = clamp(fragColor.rgb, 0.0f, 1.0f);
	return fragColor;
}

vec4 contrast(vec4 fragColor, float range) {
	float factor = (1.0156f * (range + 1.0f)) / (1.0f * (1.0156f - range));

	fragColor.r = factor * (fragColor.r - 0.5f) + 0.5f;
	fragColor.g = factor * (fragColor.g - 0.5f) + 0.5f;
	fragColor.b = factor * (fragColor.b - 0.5f) + 0.5f;

	fragColor.r = clamp(fragColor.r, 0.0f, 1.0f);
	fragColor.g = clamp(fragColor.g, 0.0f, 1.0f);
	fragColor.b = clamp(fragColor.b, 0.0f, 1.0f);

	return fragColor;
}

vec4 binary(vec4 fragColor, float thresehold) {
	vec4 grayColor = grayscale(fragColor);
	return length(grayColor) > thresehold ? vec4(1.0f) : vec4(vec3(0.0f), 1.0f);
}

vec4 edgeDetection(int width, int height) {
	float widthSize = 1.0f / width;
	float heightSize = 1.0f / height;

	vec2 offsets[9] = vec2[](
        vec2(-widthSize,  heightSize), // top-left
        vec2( 0.0f,       heightSize), // top-center
        vec2( widthSize,  heightSize), // top-right
        vec2(-widthSize,  0.0f),   // center-left
        vec2( 0.0f,       0.0f),   // center-center
        vec2( widthSize,  0.0f),   // center-right
        vec2(-widthSize, -heightSize), // bottom-left
        vec2( 0.0f,      -heightSize), // bottom-center
        vec2( widthSize, -heightSize)  // bottom-right    
    );

    float kernel[9] = float[](
         1,  1,  1,
         1, -8,  1,
         1,  1,  1
    );
    
    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(u_Texture, v_TexCoords.st + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++)
        col += sampleTex[i] * kernel[i];
    
    return vec4(col, 1.0);
}

vec4 gradient(vec4 fragColor, vec3 startColour, vec3 endColour, float angle, float intensity) {
	vec2 origin = vec2(0.5f, 0.5f);
	vec2 uv = v_TexCoords;
	uv -= origin;
    uv *= 2.0f;
    
    float finalAngle = radians(angle) + atan(uv.y, uv.x);
	float len = length(uv);
    uv = vec2(cos(finalAngle) * len, sin(finalAngle) * len) + origin;

	vec4 colour = mix(vec4(startColour, 1.0f), vec4(endColour, 1.0f), smoothstep(0.0, 1.0, uv.x));
	return mix(fragColor, colour, intensity);
}

vec4 mosaic(int pixelSize, int width, int height) {
	vec2 pixel = vec2(pixelSize) / vec2(width, height);
	vec2 coord = floor(v_TexCoords / pixel) * pixel;
	return texture(u_Texture, coord);
}

void main() {
	o_FragColor = texture(u_Texture, v_TexCoords);
	/*{{CONTENT}}*/
}

#version 330 core

layout(location = 0) out vec4 o_FragColor;
layout(location = 1) out int o_EntityID;

in vec2 v_TexCoords;
flat in int v_EntityID;

uniform sampler2D u_Texture;
/*{{HEADER}}*/

// vec4 negative(vec4 fragColor)
// {
// 	return 1.0f - fragColor;
// }

// vec4 grayscale(vec4 fragColor)
// {
// 	float grayFactor = 0.2199f * fragColor.r + 0.7152f * fragColor.g + 0.0722f * fragColor.b;
// 	return vec4(vec3(grayFactor), fragColor.a);
// }

// vec4 sepia(vec4 fragColor)
// {
// 	fragColor.r = min(255.0f, fragColor.r * 0.393f + fragColor.g * 0.769f + fragColor.b * 0.189f);
// 	fragColor.g = min(255.0f, fragColor.r * 0.349f + fragColor.g * 0.686f + fragColor.b * 0.168f);
// 	fragColor.b = min(255.0f, fragColor.r * 0.272f + fragColor.g * 0.534f + fragColor.b * 0.131f);
// 	return fragColor;
// }

vec4 brightness(vec4 fragColor, float range)
{
	fragColor.rgb *= range;
	fragColor.rgb = clamp(fragColor.rgb, 0.0f, 1.0f);
	return fragColor;
}

const float IMAGE_WIDTH = 1.0f / 750.0f;
const float IMAGE_HEIGHT = 1.0f / 732.0f;

void main()
{
	o_FragColor = texture(u_Texture, v_TexCoords);
	/*
	vec2 offsets[9] = vec2[](
        vec2(-IMAGE_WIDTH,   IMAGE_HEIGHT), // top-left
        vec2( 0.0f,    		 IMAGE_HEIGHT), // top-center
        vec2( IMAGE_WIDTH,   IMAGE_HEIGHT), // top-right
        vec2(-IMAGE_WIDTH,   0.0f),   		// center-left
        vec2( 0.0f,    		 0.0f),   		// center-center
        vec2( IMAGE_WIDTH,   0.0f),   		// center-right
        vec2(-IMAGE_WIDTH, 	-IMAGE_HEIGHT), // bottom-left
        vec2( 0.0f,   		-IMAGE_HEIGHT), // bottom-center
        vec2( IMAGE_WIDTH, 	-IMAGE_HEIGHT)  // bottom-right    
    );
	*/

	const int size = 23;

	vec2 offsets[23] = vec2[](
		vec2(IMAGE_WIDTH * -11.0f, 	0.0f),
		vec2(IMAGE_WIDTH * -10.0f, 	0.0f),
		vec2(IMAGE_WIDTH * -9.0f, 	0.0f),
		vec2(IMAGE_WIDTH * -8.0f, 	0.0f),
		vec2(IMAGE_WIDTH * -7.0f, 	0.0f),
		vec2(IMAGE_WIDTH * -6.0f, 	0.0f),
		vec2(IMAGE_WIDTH * -5.0f, 	0.0f),
		vec2(IMAGE_WIDTH * -4.0f, 	0.0f),
        vec2(IMAGE_WIDTH * -3.0f, 	0.0f),
        vec2(IMAGE_WIDTH * -2.0f, 	0.0f),
        vec2(IMAGE_WIDTH * -1.0f,   0.0f),
        vec2(IMAGE_WIDTH *  0.0f,   0.0f),
        vec2(IMAGE_WIDTH *  1.0f,   0.0f),
        vec2(IMAGE_WIDTH *  2.0f,   0.0f),
        vec2(IMAGE_WIDTH *  3.0f, 	0.0f),
		vec2(IMAGE_WIDTH *  4.0f, 	0.0f),
		vec2(IMAGE_WIDTH *  5.0f, 	0.0f),
		vec2(IMAGE_WIDTH *  6.0f, 	0.0f),
		vec2(IMAGE_WIDTH *  7.0f, 	0.0f),
		vec2(IMAGE_WIDTH *  8.0f, 	0.0f),
		vec2(IMAGE_WIDTH *  9.0f, 	0.0f),
		vec2(IMAGE_WIDTH *  10.0f, 	0.0f),
		vec2(IMAGE_WIDTH *  11.0f, 	0.0f)
    );

    float kernel[23] = float[](
        7.172582375623374e-8,
  		9.42127190479501e-7,
  		0.00000967735769344153,
  		0.000077748908994306,
  		0.0004886419288280076,
		0.002402732215872638,
		0.009244615261350906,
		0.027834681336560585,
		0.06559072781320335,
		0.12097744334949354,
  		0.17466644640734785,
  		0.1974125431352822,
  		0.17466644640734785,
  		0.12097744334949354,
  		0.06559072781320335,
  		0.027834681336560585,
  		0.009244615261350906,
  		0.002402732215872638,
  		0.0004886419288280076,
  		0.000077748908994306,
  		0.00000967735769344153,
  		9.42127190479501e-7,
  		7.172582375623374e-8
    );
    
    vec3 sampleTex[23];
    for(int i = 0; i < size; i++)
    {
        sampleTex[i] = vec3(texture(u_Texture, v_TexCoords.st + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for(int i = 0; i < size; i++)
        col += sampleTex[i] * kernel[i];
    
    //o_FragColor = vec4(col, 1.0);

	/*{{CONTENT}}*/

	o_EntityID = v_EntityID;
}
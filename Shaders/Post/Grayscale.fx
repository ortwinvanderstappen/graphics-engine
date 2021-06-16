//=============================================================================
//// Shader uses position and texture
//=============================================================================
SamplerState samPoint
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Mirror;
	AddressV = Mirror;
};

Texture2D gTexture;

/// Create Depth Stencil State (ENABLE DEPTH WRITING)
DepthStencilState gEnableDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
};
/// Create Rasterizer State (Backface culling) 
RasterizerState gBackFaceRasterizer
{
	CullMode = BACK;
};

//IN/OUT STRUCTS
//--------------
struct VS_INPUT
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD1;
};


//VERTEX SHADER
//-------------
PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT) 0;
	// Set the Position
	output.Position = float4(input.Position, 1.f);
	// Set the TexCoord
	output.TexCoord = input.TexCoord;
	
	return output;
}


//PIXEL SHADER
//------------
float4 PS(PS_INPUT input) : SV_Target
{
    // Step 1: sample the texture
	float4 color = gTexture.Sample(samPoint, input.TexCoord);
	// Step 2: calculate the mean value
	// https://en.wikipedia.org/wiki/Grayscale
	float greyScaleColor = 0.2126f * color.r + 0.7152.r * color.g + 0.0722 * color.b;
	// Step 3: return the color
	return float4(greyScaleColor.r, greyScaleColor.r, greyScaleColor.r, 1.0f);
}


//TECHNIQUE
//---------
technique11 Grayscale
{
	pass P0
	{
        // Set states...
		SetRasterizerState(gBackFaceRasterizer);
		SetDepthStencilState(gEnableDepth, 0);

		SetVertexShader(CompileShader(vs_4_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS()));
	}
}


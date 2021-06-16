float4x4 gWorld : WORLD;
float4x4 gWorldViewProj : WORLDVIEWPROJECTION;
float4x4 gWorldViewProj_Light;
float3 gLightDirection = float3(-0.577f, -0.577f, 0.577f);
float gShadowMapBias = 0.01f;
float4x4 gBones[70];
float2 shadowMapResolution = float2(1280.f*4, 720.f*4);

Texture2D gDiffuseMap;
Texture2D gShadowMap;

SamplerComparisonState cmpSampler
{
	// sampler state
	Filter = COMPARISON_MIN_MAG_MIP_LINEAR;
	AddressU = MIRROR;
	AddressV = MIRROR;

	// sampler comparison state
	ComparisonFunc = LESS_EQUAL;
};

SamplerState samLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap; // or Mirror or Clamp or Border
	AddressV = Wrap; // or Mirror or Clamp or Border
};

struct VS_INPUT
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD;
	float4 BoneIndices : BLENDINDICES;
	float4 BoneWeights : BLENDWEIGHTS;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD;
	float4 lPos : TEXCOORD1;
};

DepthStencilState EnableDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
};

RasterizerState NoCulling
{
	CullMode = NONE;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT) 0;

	//TODO: complete Vertex Shader 
	//Hint: use the previously made shaders PosNormTex3D_Shadow and PosNormTex3D_Skinned as a guide
	
	float4 originalPosition = float4(input.pos, 1);
	float4 transformedPosition = 0;
	float3 transformedNormal = 0;

	//Skinning Magic...
	for (int i = 0; i < 4; i++)
	{
		int boneIndex = input.BoneIndices[i];
		if (boneIndex > -1)
		{
			float boneweight = input.BoneWeights[i];
			float4x4 boneMatrix = gBones[boneIndex];
			transformedPosition += mul(originalPosition * boneweight, boneMatrix);
			transformedNormal += mul(input.normal * boneweight, (float3x3) boneMatrix);
		}
	}
	transformedPosition.w = 1;
	
	output.pos = mul(transformedPosition, gWorldViewProj);
	output.normal = normalize(mul(transformedNormal, (float3x3) gWorld));
	output.texCoord = input.texCoord;
	
	//Hint: Don't forget to project our position to light clip space and store it in lPos
	output.lPos = mul(transformedPosition, gWorldViewProj_Light);
	
	return output;
}

float2 texOffset(int u, int v)
{
	// Return offseted value (our shadow map has the following dimensions: 1280 * 720)
	return float2(u * 1.f / shadowMapResolution.x, v * 1.f / shadowMapResolution.y);
}

float EvaluateShadowMap(float4 lpos)
{
	float ambient = 0.4f;
	
	// Re-homogonize position after interpolation
	lpos.xyz /= lpos.w;
	
	// Don't illuminate if the position of outside of the clip space
	if (lpos.x < -1.f || lpos.x > 1.f ||
		lpos.y < -1.f || lpos.y > 1.f ||
		lpos.z < 0.f || lpos.z > 1.f)
		return 1.f;
	
	// Clip space -> texture space coords
	lpos.x = (lpos.x / 2.f) + 0.5;
	lpos.y = (lpos.y / -2.f) + 0.5;
	
	// Apply shadowmap bias
	lpos.z -= gShadowMapBias;
	
	// Sample from the shadowmap
	float sum = 0;
	float x, y;
	
	// PCF filtering on a 4x4 texel neighborhood
	for (y = -1.5f; y < 1.5f; y += 1.f)
	{
		for (x = -1.5f; x < 1.5f; x += 1.f)
		{
			sum += gShadowMap.SampleCmpLevelZero(cmpSampler, lpos.xy + texOffset(x, y), lpos.z).r;
		}
	}
	
	float shadowmapDepth = sum / 16.f;
	
	// Pixel is in shadow if the clip space z is greater than the shadow map
	if (shadowmapDepth < lpos.z)
		return ambient;
	
	return 1.f;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_TARGET
{
	float shadowValue = EvaluateShadowMap(input.lPos);

	float4 diffuseColor = gDiffuseMap.Sample(samLinear, input.texCoord);
	float3 color_rgb = diffuseColor.rgb;
	float color_a = diffuseColor.a;
	
	//HalfLambert Diffuse :)
	float diffuseStrength = dot(input.normal, -gLightDirection);
	diffuseStrength = diffuseStrength * 0.5 + 0.5;
	diffuseStrength = saturate(diffuseStrength);
	color_rgb = color_rgb * diffuseStrength;

	return float4(color_rgb * shadowValue, color_a);
}

//--------------------------------------------------------------------------------------
// Technique
//--------------------------------------------------------------------------------------
technique11 Default
{
	pass P0
	{
		SetRasterizerState(NoCulling);
		SetDepthStencilState(EnableDepth, 0);

		SetVertexShader(CompileShader(vs_4_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS()));
	}
}


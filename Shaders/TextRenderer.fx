float4x4 gTransform : WORLDVIEWPROJECTION;
Texture2D gSpriteTexture;
float2 gTextureSize;

SamplerState samPoint
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = WRAP;
	AddressV = WRAP;
};

BlendState EnableBlending
{
	BlendEnable[0] = TRUE;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
};

DepthStencilState NoDepth
{
	DepthEnable = FALSE;
};

RasterizerState BackCulling
{
	CullMode = BACK;
};

//SHADER STRUCTS
//**************
struct VS_DATA
{
	int Channel : TEXCOORD2; //Texture Channel
	float3 Position : POSITION; //Left-Top Character Quad Starting Position
	float4 Color : COLOR; //Color of the vertex
	float2 TexCoord : TEXCOORD0; //Left-Top Character Texture Coordinate on Texture
	float2 CharSize : TEXCOORD1; //Size of the character (in screenspace)
};

struct GS_DATA
{
	float4 Position : SV_POSITION; //HOMOGENEOUS clipping space position
	float4 Color : COLOR; //Color of the vertex
	float2 TexCoord : TEXCOORD0; //Texcoord of the vertex
	int Channel : TEXCOORD1; //Channel of the vertex
};

//VERTEX SHADER
//*************
VS_DATA MainVS(VS_DATA input)
{
	return input;
}

//GEOMETRY SHADER
//***************
void CreateVertex(inout TriangleStream<GS_DATA> triStream, float3 pos, float4 col, float2 texCoord, int channel)
{
	// Create a new GS_DATA object
	GS_DATA vertex = (GS_DATA) 0;
	// Fill in all the fields
	vertex.Position = mul(float4(pos, 1.f), gTransform);
	vertex.Color = col;
	vertex.TexCoord = texCoord;
	vertex.Channel = channel;
	// Append it to the TriangleStream
	triStream.Append(vertex);
}

[maxvertexcount(4)]
void MainGS(point VS_DATA vertex[1], inout TriangleStream<GS_DATA> triStream)
{
	// Create a Quad using the character information of the given vertex
	// Note that the Vertex.CharSize is in screenspace, TextureCoordinates aren't ;) [Range 0 > 1]

	float3 originalPos = vertex[0].Position;
	float2 originalTexcoord = vertex[0].TexCoord;
	
	float3 position;
	float4 col;
	float2 texCoord;
	int channel;
	
	float2 charSize = vertex[0].CharSize;
	
	float2 texCoordOffset = float2(charSize.x / gTextureSize.x, charSize.y / gTextureSize.y);
	
	// Define shared data
	col = vertex[0].Color;
	channel = vertex[0].Channel;
	
	//1. Vertex Left-Top
	position = float3(originalPos.x, originalPos.y, originalPos.z);
	texCoord = float2(originalTexcoord.x, originalTexcoord.y);
	CreateVertex(triStream, position, col, texCoord, channel);

	//2. Vertex Right-Top
	position = float3(originalPos.x + charSize.x, originalPos.y, originalPos.z);
	texCoord = float2(originalTexcoord.x + texCoordOffset.x, originalTexcoord.y);
	CreateVertex(triStream, position, col, texCoord, channel);

	//3. Vertex Left-Bottom
	position = float3(originalPos.x, originalPos.y + charSize.y, originalPos.z);
	texCoord = float2(originalTexcoord.x, originalTexcoord.y + texCoordOffset.y);
	CreateVertex(triStream, position, col, texCoord, channel);

	//4. Vertex Right-Bottom
	position = float3(originalPos.x + charSize.x, originalPos.y + charSize.y, originalPos.z);
	texCoord = float2(originalTexcoord.x + texCoordOffset.x, originalTexcoord.y + texCoordOffset.y);
	CreateVertex(triStream, position, col, texCoord, channel);
}

//PIXEL SHADER
//************
float4 MainPS(GS_DATA input) : SV_TARGET
{
	// Sample the texture and return the correct channel [Vertex.Channel]
	float4 textureSample = gSpriteTexture.Sample(samPoint, input.TexCoord);
	// You can iterate a float4 just like an array, using the index operator
	float textureSampleWithChannel = textureSample[input.Channel];
	// Also, don't forget to colorize ;) [Vertex.Color]
	float4 finalColor = input.Color * textureSampleWithChannel;
	
	return finalColor;
}

// Default Technique
technique10 Default
{

	pass p0
	{
		SetRasterizerState(BackCulling);
		SetBlendState(EnableBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetDepthStencilState(NoDepth, 0);
		SetVertexShader(CompileShader(vs_4_0, MainVS()));
		SetGeometryShader(CompileShader(gs_4_0, MainGS()));
		SetPixelShader(CompileShader(ps_4_0, MainPS()));
	}
}

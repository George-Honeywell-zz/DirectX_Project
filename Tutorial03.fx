//--------------------------------------------------------------------------------------
// File: Tutorial07.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register(t0);
//Texture2DArray textureArray : register(t0);
//Texture2D txDiffuse1 : register(t1);
SamplerState samLinear : register(s0);

cbuffer ConstantBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 LightDir;
	float4 LightColour;
	float4 OutputColour;
}

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL;

};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.Pos = mul(input.Pos, World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Tex = input.Tex;
	output.Norm = mul(float4(input.Norm, 1), World).xyz;

	return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
	float4 finalColour = 0;
	//float4 finalColour = textureArray.Sample(sampler, float3(u, v, 1));

	finalColour = saturate(dot((float3)LightDir, input.Norm) * LightColour);
		
	finalColour.a = 1;
	return txDiffuse.Sample(samLinear, input.Tex) * finalColour;
	//return textureArray.Sample(samLinear, input.Tex) * finalColour;
}

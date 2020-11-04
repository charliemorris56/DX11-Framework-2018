//--------------------------------------------------------------------------------------
// File: DX11 Framework.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Texture Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
    //float gTime;   //May need to add a buffer to get gtime working 

    float4 DiffuseMtrl;
    float4 DiffuseLight;
    float4 AmbientMtrl;
    float4 AmbientLight;
    float4 SpecularMtrl;
    float4 SpecularLight;
    float SpecularPower;
    float3 EyePosW;
    float3 LightVecW;
    float buffer;
}
//--------------------------------------------------------------------------------------

struct VS_INPUT
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
    float3 NormalW : NORMAL;
	float3 PosW : POSITION;
	float2 Tex : TEXCOORD;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
//VS_OUTPUT VS( float4 Pos : POSITION, float4 Color : COLOR )
//{
//    //Pos.xy += 0.5f * sin(Pos.x) * sin(3.0f * gTime);
//    //Pos.z *= 0.6f + 0.4f * sin(2.0f * gTime);
//
//    VS_OUTPUT output = (VS_OUTPUT)0;
//    output.Pos = mul( Pos, World );
//    output.Pos = mul( output.Pos, View );
//    output.Pos = mul( output.Pos, Projection );
//    output.Color = Color;
//    return output;
//}

//------------------------------------------------------------------------------------
// Vertex Shader - Implements Gouraud Shading using Diffuse lighting only
//------------------------------------------------------------------------------------
//VS_OUTPUT VS(float4 Pos : POSITION, float3 NormalL : NORMAL)
//{
//    VS_OUTPUT output = (VS_OUTPUT)0;
//
//    output.Pos = mul(Pos, World);
//    output.Pos = mul(output.Pos, View);
//    output.Pos = mul(output.Pos, Projection);
//
//    float3 toEye = normalize(EyePosW - output.Pos.xyz);
//
//    // Convert from local space to world space 
//    // W component of vector is 0 as vectors cannot be translated
//    float3 normalW = mul(float4(NormalL, 0.0f), World).xyz;
//    normalW = normalize(normalW);
//
//    // Compute Colour using Diffuse lighting only
//    float diffuseAmount = max(dot(LightVecW, normalW), 0.0f);
//    float3 ambient = AmbientMtrl * AmbientLight;
//    float3 diffuse = diffuseAmount * (DiffuseMtrl * DiffuseLight).rgb;
//    float3 r = reflect(-LightVecW, normalW);
//    float specularAmount = pow(max(dot(r, toEye), 0.0f), SpecularPower);
//    float3 specular = specularAmount * (SpecularMtrl * SpecularLight).rgb;
//
//    output.Color.rgb = diffuse + ambient + specular;
//    output.Color.a = DiffuseMtrl.a;
//
//    return output;
//}
//
////--------------------------------------------------------------------------------------
//// Pixel Shader
////--------------------------------------------------------------------------------------
//float4 PS(VS_OUTPUT input) : SV_Target
//{
//    return input.Color;
//}

//------------------------------------------------------------------------------------
// Vertex Shader - Implements Gouraud Shading using Diffuse lighting only
//------------------------------------------------------------------------------------
VS_OUTPUT VS(float4 Pos : POSITION, float3 NormalL : NORMAL, float2 Tex : TEXCOORD)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    float4 posW = mul(Pos, World);

    output.PosW = posW.xyz;

    output.Pos = mul(posW, View);
    output.Pos = mul(output.Pos, Projection); 

    float3 normalW = mul(float4(NormalL, 0.0f), World).xyz;
    output.NormalW = normalW;

	output.Tex = Tex;

    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( VS_OUTPUT input ) : SV_Target
{
	float4 textureColour = txDiffuse.Sample(samLinear, input.Tex); 

    float3 toEye = normalize(EyePosW - input.PosW.xyz);

    // Convert from local space to world space 
    // W component of vector is 0 as vectors cannot be translated
    input.NormalW = normalize(input.NormalW);

    // Compute Colour using Diffuse lighting only
    float diffuseAmount = max(dot(LightVecW, input.NormalW), 0.0f);
    float3 ambient = (AmbientMtrl * AmbientLight).rgb;
    float3 diffuse = diffuseAmount * (DiffuseMtrl * DiffuseLight).rgb;
    float3 r = reflect(-LightVecW, input.NormalW);
    float specularAmount = pow(max(dot(r, toEye), 0.0f), SpecularPower);
    float3 specular = specularAmount * (SpecularMtrl * SpecularLight).rgb;

    input.Color.rgb = diffuse + ambient + specular + textureColour.rgb;
    input.Color.a = DiffuseMtrl.a + textureColour.a;

    return input.Color;
}
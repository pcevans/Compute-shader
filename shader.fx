//--------------------------------------------------------------------------------------
// File: lecture 8.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
#include "shaderheader.h"
Texture2D txDiffuse : register(t0);
Texture2D txnormal : register(t1);
Texture2D txSkybox : register(t2);
Texture2D txVL : register(t4);
Texture2D txBloom : register(t5);

//RWTexture3D<float4> Voxel_GI : register(u1);
RWTexture1D<unsigned int> Octree_RW : register(u1);
RWTexture1D<float4> VFL : register(u2);
RWTexture1D<unsigned int> count : register(u3);

Texture3D Voxels_SR : register(t3);
Texture1D<unsigned int> Octree_SR : register(t6);

SamplerState samLinear : register(s0);


cbuffer ConstantBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 info;
	float4 campos;
	float4 sun_position; //rotation of the sun
	float4 daytimer; //day+night cycle
};



//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL0;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
	float3 WorldPos : TEXCOORD1;
	float4 Norm : Normal0;
	float4 Opos : TEXCOORD2;

};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.Opos = input.Pos;
	output.Pos = mul(input.Pos, World);
	output.WorldPos = output.Pos;
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Tex = input.Tex;

	return output;
}

PS_INPUT VS_screen(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	float4 pos = input.Pos;
	output.Pos = pos;
	output.Tex = input.Tex;
	output.Norm.xyz = input.Norm;
	output.Norm.w = 1;

	return output;
}

uint put_in_octree(float3 pos) {
	uint level = 0;
	uint octdex = 1;
	uint currdex = 0;
	uint temp = 0;
	float3 midpt = float3(0, 0, 0);
	float midsize = vxarea / 4.;
	uint idx = 0;
	if (Octree_RW[0] == 0)
		InterlockedExchange(Octree_RW[0], 9, temp); 
		//Octree_RW[0] = 9;
	

	while (level != maxlevel) {
		if (pos.x < midpt.x) {
			if (pos.y < midpt.y) {
				if (pos.z < midpt.z) idx = 0;
				else idx = 6;
			}
			else {
				if (pos.z < midpt.z) idx = 2;
				else idx = 4;
			}
		}
		else {
			if (pos.y < midpt.y) {
				if (pos.z < midpt.z) idx = 1;
				else idx = 7;
			}
			else {
				if (pos.z < midpt.z) idx = 3;
				else idx = 5;
			}
		}

		currdex = octdex + idx;
		if (level + 1 != maxlevel) {
			if (Octree_RW[currdex] > 1) {
				//InterlockedAdd(Octree_RW[currdex], 0, octdex);
				octdex = Octree_RW[currdex];
			}
			else {
				//InterlockedAdd(Octree_RW[0], 8, octdex);
				//octdex += 1;
				octdex = Octree_RW[0];
				Octree_RW[0] += 8;
				Octree_RW[currdex] = octdex;
				//InterlockedExchange(Octree_RW[currdex], octdex, temp);
			}
		}
		else
			//InterlockedExchange(Octree_RW[currdex],1, temp);
			Octree_RW[currdex] = 1;

		if (idx % 2 == 0) midpt.x -= midsize;
		else midpt.x += midsize; 

		if (idx < 4) midpt.z -= midsize;
		else midpt.z += midsize;

		if (idx < 6 && idx > 1) midpt.y += midsize;
		else midpt.y -= midsize;

		midsize = midsize / 2.;

		level += 1;
	}

	return currdex;
}

/*bool find_in_octree(float3 pos) {
	uint level = 0;
	uint octdex = 1;
	float3 midpt = float3(0, 0, 0);
	float midsize = vxarea / 4.;
	uint idx = 0;
	[unroll(8)]
	while (level != maxlevel) {
		if (pos.x < midpt.x) {
			if (pos.y < midpt.y) {
				if (pos.z < midpt.z) idx = 0;
				else idx = 6;
			}
			else {
				if (pos.z < midpt.z) idx = 2;
				else idx = 4;
			}
		}
		else {
			if (pos.y < midpt.y) {
				if (pos.z < midpt.z) idx = 1;
				else idx = 7;
			}
			else {
				if (pos.z < midpt.z) idx = 3;
				else idx = 5;
			}
		}
		
		if (Octree_SR[octdex + idx] > 0)
			octdex = Octree_SR[octdex + idx];
		else {
			return false;
		}

		if (idx % 2 == 0) midpt.x -= midsize;
		else midpt.x += midsize;

		if (idx < 4) midpt.z -= midsize;
		else midpt.z += midsize;

		if (idx < 6 && idx > 1) midpt.y += midsize;
		else midpt.y -= midsize;

		midsize = midsize / 2.;

		level += 1;	
	}

	return true;
}*/

// 3D texture space converter
float3 voxelspace(float3 worldpos)
{
	float f = 20. / 128;
	float3 wp = worldpos + float3(10, 10, 10);
	float3 pos = wp / f;
	return pos + float3(0.001, 0.001, 0.001);
}

float3 re_voxelspace(float3 worldpos)
{
	float f = 20. / 128;
	float3 pos = worldpos;
	pos.xyz *= f;
	pos.xyz += float3(-10, -10, -10);
	return pos;
}
//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
struct PS_MRTOutput
{
	float4 Color    : SV_Target0;
};

float4 voxel_ON = float4(float3(0, 1, 0), 1);

/*PS_MRTOutput PS(PS_INPUT input) : SV_Target
{
	PS_MRTOutput outp;
	float4 normaltex = txnormal.Sample(samLinear, input.Tex);
	normaltex -= float4(0.5, 0.5, 0.5,0);
	normaltex *= 2.;

	float4 color = txDiffuse.Sample(samLinear, input.Tex);
	float depth = saturate(input.Pos.z / input.Pos.w);
	float radius = pow(pow(input.Opos.x, 2) + pow(input.Opos.y, 2),0.5);

	float2 normal2 = (input.Tex - float2(0.5, 0.5))*2.;
	float3 normal3;
	normal3.xy = normal2.xy;
	normal3.z = sqrt(1 - (normal2.x*normal2.x + normal2.y*normal2.y));
	normal3 = mul(normal3, View);
	float height = normal3.z;

	/// skysphere sun
	float4 sunposition = sun_position.xyzw;
	float3 lightdirection = normalize(input.WorldPos - sunposition.xyz); 

	normaltex.rgb = mul(normaltex.rgb, View);

	normal3 = normalize(normal3 + normaltex.rgb*0.3);

	float3 nn = saturate(dot(lightdirection, normal3));
	nn.r += 0.3;
	//color.rgb = float3(1, 1, 1);

	float4 viewpos = input.Opos;
	float4 originalPos = viewpos;

	matrix rotview = View;
	rotview._41 = rotview._42 = rotview._43 = 0.0;
	viewpos = mul(viewpos, World);

	int3 ipos;

	// add sphere shapes to the voxels
	ipos = voxelspace(viewpos);
	if (ipos.x < 512 && ipos.x >= 0 && ipos.y < 512 && ipos.y >= 0 && ipos.z < 512 && ipos.z >= 0)
	{
		float r = sqrt((originalPos.x * originalPos.x) + (originalPos.y * originalPos.y));
		if (r > 1) r = 1;
		if (r < 0) r = 0;
		float z = cos((PI / 2.0) * r);

		matrix nrw = World;
		nrw._41 = nrw._42 = nrw._43 = 0;

		float3 depthVector = float3 (0, 0, 1);
		depthVector = mul(depthVector, nrw);
		depthVector = normalize(depthVector);

		int3 intPos;
		if (color.a > 0.01) {
			for (int i = 0; i < 10; i++)
			{

				float inum = (((float)i / 10.0) * 2 * z) - z;
				float3 f3 = viewpos + (depthVector * inum);
				intPos = voxelspace(f3);
				if (intPos.x < 512 && intPos.x >= 0 && intPos.y < 512 && intPos.y >= 0 && intPos.z < 512 && intPos.z >= 0) {
					Voxel_GI[intPos] = float4(float3(0, 0, 1), 1);
					//Voxel_GI[intPos] = color;
				}
			}
		}
	}

	outp.Color = float4(color.rgb*nn.r, color.a);
	return outp;
}*/

PS_MRTOutput PS(PS_INPUT input) : SV_Target
{
	PS_MRTOutput outp;
	float4 normaltex = txnormal.Sample(samLinear, input.Tex);
	normaltex -= float4(0.5, 0.5, 0.5,0);
	normaltex *= 2.;

	float4 color = txDiffuse.Sample(samLinear, input.Tex);
	float depth = saturate(input.Pos.z / input.Pos.w);
	float radius = pow(pow(input.Opos.x, 2) + pow(input.Opos.y, 2),0.5);

	float2 normal2 = (input.Tex - float2(0.5, 0.5))*2.;
	float3 normal3;
	normal3.xy = normal2.xy;
	normal3.z = sqrt(1 - (normal2.x*normal2.x + normal2.y*normal2.y));
	normal3 = mul(normal3, View);
	float height = normal3.z;

	/// skysphere sun
	float4 sunposition = sun_position.xyzw;
	float3 lightdirection = normalize(input.WorldPos - sunposition.xyz);

	normaltex.rgb = mul(normaltex.rgb, View);

	normal3 = normalize(normal3 + normaltex.rgb*0.3);

	float3 nn = saturate(dot(lightdirection, normal3));
	nn.r += 0.3;
	//color.rgb = float3(1, 1, 1);

	float4 viewpos = float4(input.WorldPos, 1);
	float4 originalPos = input.Opos;

	matrix rotview = View;
	rotview._41 = rotview._42 = rotview._43 = 0.0;
	
	/////////////////////////////////////////////////////////

	/*Octree_RW[1] = 1;
	Octree_RW[2] = 0;
	Octree_RW[3] = 1;
	Octree_RW[4] = 1;
	Octree_RW[5] = 1;
	Octree_RW[6] = 1;
	Octree_RW[7] = 1;
	Octree_RW[8] = 1;*/


	/*Octree_RW[1] = 9;
	Octree_RW[14] = 17;
	Octree_RW[22] = 25;
	Octree_RW[30] = 33;
	Octree_RW[38] = 41;
	Octree_RW[46] = 49;
	Octree_RW[54] = 57;
	Octree_RW[62] = 1;*/
	//put_in_octree(float3(0.1,0.1,0.1));
	//put_in_octree(float3(-0.1, -0.1, -0.1));
	//put_in_octree(viewpos);

	float idx = 0;
	
	InterlockedAdd(count[0], 1, idx);
	VFL[idx] = viewpos;

	/*Octree_RW[0] = 121;
	Octree_RW[1] = 9;
	Octree_RW[2] = 0;
	Octree_RW[3] = 0;
	Octree_RW[4] = 0;
	Octree_RW[5] = 0;
	Octree_RW[6] = 65;
	Octree_RW[7] = 0;
	Octree_RW[8] = 0;
	Octree_RW[9] = 0;
	Octree_RW[10] = 0;
	Octree_RW[11] = 0;
	Octree_RW[12] = 0;
	Octree_RW[13] = 0;
	Octree_RW[14] = 17;
	Octree_RW[15] = 0;
	Octree_RW[16] = 0;
	Octree_RW[17] = 0;
	Octree_RW[18] = 0;
	Octree_RW[19] = 0;
	Octree_RW[20] = 0;
	Octree_RW[21] = 0;
	Octree_RW[22] = 25;
	Octree_RW[23] = 0;
	Octree_RW[24] = 0;
	Octree_RW[25] = 0;
	Octree_RW[26] = 0;
	Octree_RW[27] = 0;
	Octree_RW[28] = 0;
	Octree_RW[29] = 0;
	Octree_RW[30] = 33;
	Octree_RW[31] = 0;
	Octree_RW[32] = 0;
	Octree_RW[33] = 0;
	Octree_RW[34] = 0;
	Octree_RW[35] = 0;
	Octree_RW[36] = 0;
	Octree_RW[37] = 0;
	Octree_RW[38] = 41;
	Octree_RW[39] = 0;
	Octree_RW[40] = 0;
	Octree_RW[41] = 0;
	Octree_RW[42] = 0;
	Octree_RW[43] = 0;
	Octree_RW[44] = 0;
	Octree_RW[45] = 0;
	Octree_RW[46] = 49;
	Octree_RW[47] = 0;
	Octree_RW[48] = 0;
	Octree_RW[49] = 0;
	Octree_RW[50] = 0;
	Octree_RW[51] = 0;
	Octree_RW[52] = 0;
	Octree_RW[53] = 0;
	Octree_RW[54] = 57;
	Octree_RW[55] = 0;
	Octree_RW[56] = 0;
	Octree_RW[57] = 0;
	Octree_RW[58] = 0;
	Octree_RW[59] = 0;
	Octree_RW[60] = 0;
	Octree_RW[61] = 0;
	Octree_RW[62] = 1;
	Octree_RW[63] = 0;
	Octree_RW[64] = 0;
	Octree_RW[65] = 73;
	Octree_RW[66] = 0;
	Octree_RW[67] = 0;
	Octree_RW[68] = 0;
	Octree_RW[69] = 0;
	Octree_RW[70] = 0;
	Octree_RW[71] = 0;
	Octree_RW[72] = 0;
	Octree_RW[73] = 81;
	Octree_RW[74] = 0;
	Octree_RW[75] = 0;
	Octree_RW[76] = 0;
	Octree_RW[77] = 0;
	Octree_RW[78] = 0;
	Octree_RW[79] = 0;
	Octree_RW[80] = 0;
	Octree_RW[81] = 89;
	Octree_RW[82] = 0;
	Octree_RW[83] = 0;
	Octree_RW[84] = 0;
	Octree_RW[85] = 0;
	Octree_RW[86] = 0;
	Octree_RW[87] = 0;
	Octree_RW[88] = 0;
	Octree_RW[89] = 97;
	Octree_RW[90] = 0;
	Octree_RW[91] = 0;
	Octree_RW[92] = 0;
	Octree_RW[93] = 0;
	Octree_RW[94] = 0;
	Octree_RW[95] = 0;
	Octree_RW[96] = 0;
	Octree_RW[97] = 105;
	Octree_RW[98] = 0;
	Octree_RW[99] = 0;
	Octree_RW[100] = 0;
	Octree_RW[101] = 0;
	Octree_RW[102] = 0;
	Octree_RW[103] = 0;
	Octree_RW[104] = 0;
	Octree_RW[105] = 113;
	Octree_RW[106] = 0;
	Octree_RW[107] = 0;
	Octree_RW[108] = 0;
	Octree_RW[109] = 0;
	Octree_RW[110] = 0;
	Octree_RW[111] = 0;
	Octree_RW[112] = 0;
	Octree_RW[113] = 1;*/
	/*int ii = 0;
for (; ii <= 113; ii++)
	Octree_RW[ii] = 0;
put_in_octree(float3(-.1, -.1, -.1));
put_in_octree(float3(.1, .1, .1));*/
	/////////////////////////////////////////////////////////

	int3 ipos;

	// add sphere shapes to the voxels
	//ipos = voxelspace(viewpos);
	//if (ipos.x < 512 && ipos.x >= 0 && ipos.y < 512 && ipos.y >= 0 && ipos.z < 512 && ipos.z >= 0)
	//{
		float r = sqrt((originalPos.x * originalPos.x) + (originalPos.y * originalPos.y));
		if (r > 1) r = 1;
		if (r < 0) r = 0;
		float z = cos((PI / 2.0) * r);

		matrix nrw = World;
		nrw._41 = nrw._42 = nrw._43 = 0;

		float3 depthVector = float3 (0, 0, 1);
		depthVector = mul(depthVector, nrw);
		depthVector = normalize(depthVector);

		//int3 intPos;
		if (color.a > 0.01) {
			for (int i = 0; i < 10; i++) {
				float inum = (((float)i / 10.0) * 2 * z) - z;
				float3 f3 = viewpos + (depthVector * inum);

				/////////////////////////////////////////////////////////
				//put_in_octree(f3);
				/////////////////////////////////////////////////////////

				//intPos = voxelspace(f3);
				//if (intPos.x < 512 && intPos.x >= 0 && intPos.y < 512 && intPos.y >= 0 && intPos.z < 512 && intPos.z >= 0) {
				//	Voxel_GI[intPos] = float4(float3(0, 0, 1), 1);
				//	Voxel_GI[intPos] = color;
				//}
			}
		}
	//}

	outp.Color = float4(color.rgb*nn.r, color.a);
	return outp;
}


//--------------------------------------------------------------------------------------
// Pixel Shader skybox/sphere
//--------------------------------------------------------------------------------------
float4 PSsky(PS_INPUT input) : SV_Target
{
	float4 day = txSkybox.Sample(samLinear, input.Tex);
	float4 sunset = txSkybox.Sample(samLinear, input.Tex);
	float4 night = txSkybox.Sample(samLinear, input.Tex);

	day.a = 1;
	sunset.a = .7;
	sunset.rgb -= float3(.01, .161, .353); //orange
	//sunset.rgb += float3(.1, -.1, .1); //purple
	night.a = .05;

	//float4 color = day*(1 - f) + night*f; day/night, no sunset

	float f = saturate(daytimer.x); //0->1
	float4 v, a, b, c;
	a = day; b = sunset; c = night;
	if (f >= 1)
	{
		f -= 1;
		v = a*(1 - f) + b*f;
	}
	else
	{
		v = b*(1 - f) + c*f;
	}
	//v.a = 0;
	return v;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
float4 PS_screen(PS_INPUT input) : SV_Target
{

	float4 texture_color = txDiffuse.Sample(samLinear, input.Tex);
	float4 vl = txVL.Sample(samLinear, input.Tex);
	float4 bloom = txBloom.Sample(samLinear, input.Tex);


	if(texture_color.a < 0.1)
		texture_color += vl/2;
	texture_color += bloom/2;
	texture_color.a = 1;

	return texture_color;

	for (int i = 0; i < 5; i++)
	{
		float3 col = txDiffuse.Sample(samLinear, input.Tex + float2(0.01,0)*i);
		texture_color.rgb += col * (5 - i) * 0.2;
	}
	return texture_color;
}


float4 PScloud(PS_INPUT input) : SV_Target
{
	float4 outp;
	float4 normaltex = txnormal.Sample(samLinear, input.Tex);
	normaltex -= float4(0.5, 0.5, 0.5,0);
	normaltex *= 2.;

	float4 color = txDiffuse.Sample(samLinear, input.Tex);
	float depth = saturate(input.Pos.z / input.Pos.w);

	float2 normal2 = (input.Tex - float2(0.5, 0.5))*2.;
	float3 normal3;
	normal3.xy = normal2.xy;
	normal3.z = sqrt(1 - (normal2.x*normal2.x + normal2.y*normal2.y));
	normal3 = mul(normal3, View);
	normaltex.rgb = mul(normaltex.rgb, View);
	normal3 = normalize(normal3 + normaltex.rgb*0.3);
	
	float3 lightposition = sun_position.xyz;
	float3 lightdirection = normalize(lightposition - input.WorldPos);

	float3 nn = saturate(dot(lightdirection, normal3));
	nn.r += 0.3;
	color.rgb = float3(1, 1, 1);

	//traverse through voxel cloud:
	float3 pos = voxelspace(input.WorldPos);
	pos /= 512.0;

	float collectmain = 0;
	float collectsides = 0;
	float collectsidescomp[8] = { 0,0,0,0,0,0,0,0 };
	float3 dir[5];

	/////////////////////////////////////////////////////////

	/*if (find_in_octree(input.WorldPos)) { // finds w/in .03
		return float4(0, 0, 1, 1);
	}
	else return float4(1, 1, 1, 1);*/

	/////////////////////////////////////////////////////////

	/*for (int ii = 0 + 1; ii < 8 + 1; ii = ii + 1)
	{
		dir[0] = normalize(lightdirection) * 1./512.;
		float3 vpos;
		vpos = pos + dir[0]*ii;

		float4 voxelcol = Voxels_SR.SampleLevel(samLinear, vpos,ii / 2);
		collectmain += voxelcol.a*0.0009*pow(ii, 2) * 2;
	}

	//collect voxel info from sides
	for (int i = 0; i < 2; i++) {
		for (int ii = 0 + 1; ii < 8 + 1; ii = ii + 1)
		{
			dir[1] = float3(1. / 512., 0.0, 0.0);
			if (i % 2 == 1) {
				dir[1] = dir[1] * -1;
			}
			float3 vpos;
			vpos = pos + dir[1]*ii;

			float4 voxelcol = Voxels_SR.SampleLevel(samLinear, vpos, ii / 2);
			collectsidescomp[i] += voxelcol.a*0.0009*pow(ii, 2) * 2;
		}
	}
	for (int i = 2; i < 4; i++) {
		for (int ii = 0 + 1; ii < 8 + 1; ii = ii + 1)
		{
			dir[2] = cross(dir[0], dir[1]);
			if (i % 2 == 1) {
				dir[2] = dir[2] * -1;
			}
			float3 vpos;
			vpos = pos + dir[2] * ii;

			float4 voxelcol = Voxels_SR.SampleLevel(samLinear, vpos, ii / 2);
			collectsidescomp[i] += voxelcol.a*0.0009*pow(ii,2) * 2;
		}
	}

	for (int i = 0; i < 4; i = i + 1) {
		collectsides += collectsidescomp[i];
	}

	collectsides /= 4; 

	//collectmain = collectsides;
	collectmain = (collectmain * .7) + (collectsides * .3);

	collectmain = pow(collectmain, 1.0)*1.7;
	collectmain = saturate(collectmain + 0.2);

	color.rgb *= saturate(1. - collectmain);*/
	return float4(color.rgb, color.a);

}

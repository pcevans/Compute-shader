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
RWTexture1D<unsigned int> Oct_GI : register(u1);
RWTexture1D<float4> VFL : register(u2);
RWTexture1D<unsigned int> cunt : register(u3);

Texture3D Voxels_SR : register(t3);
Texture1D<unsigned int> Oct_SR : register(t6);

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
	if (Oct_GI[0] == 0)
		InterlockedExchange(Oct_GI[0], 9, temp); 
		//Oct_GI[0] = 9;
	

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
			if (Oct_GI[currdex] > 1) {
				//InterlockedAdd(Oct_GI[currdex], 0, octdex);
				octdex = Oct_GI[currdex];
			}
			else {
				//InterlockedAdd(Oct_GI[0], 8, octdex);
				//octdex += 1;
				octdex = Oct_GI[0];
				Oct_GI[0] += 8;
				Oct_GI[currdex] = octdex;
				//InterlockedExchange(Oct_GI[currdex], octdex, temp);
			}
		}
		else
			//InterlockedExchange(Oct_GI[currdex],1, temp);
			Oct_GI[currdex] = 1;

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
		
		if (Oct_SR[octdex + idx] > 0)
			octdex = Oct_SR[octdex + idx];
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

	/*Oct_GI[1] = 1;
	Oct_GI[2] = 0;
	Oct_GI[3] = 1;
	Oct_GI[4] = 1;
	Oct_GI[5] = 1;
	Oct_GI[6] = 1;
	Oct_GI[7] = 1;
	Oct_GI[8] = 1;*/


	/*Oct_GI[1] = 9;
	Oct_GI[14] = 17;
	Oct_GI[22] = 25;
	Oct_GI[30] = 33;
	Oct_GI[38] = 41;
	Oct_GI[46] = 49;
	Oct_GI[54] = 57;
	Oct_GI[62] = 1;*/
	//put_in_octree(float3(0.1,0.1,0.1));
	//put_in_octree(float3(-0.1, -0.1, -0.1));
	//put_in_octree(viewpos);

	float idx = 0;
	
	InterlockedAdd(cunt[0], 1, idx);
	VFL[idx] = viewpos;

	/*Oct_GI[0] = 121;
	Oct_GI[1] = 9;
	Oct_GI[2] = 0;
	Oct_GI[3] = 0;
	Oct_GI[4] = 0;
	Oct_GI[5] = 0;
	Oct_GI[6] = 65;
	Oct_GI[7] = 0;
	Oct_GI[8] = 0;
	Oct_GI[9] = 0;
	Oct_GI[10] = 0;
	Oct_GI[11] = 0;
	Oct_GI[12] = 0;
	Oct_GI[13] = 0;
	Oct_GI[14] = 17;
	Oct_GI[15] = 0;
	Oct_GI[16] = 0;
	Oct_GI[17] = 0;
	Oct_GI[18] = 0;
	Oct_GI[19] = 0;
	Oct_GI[20] = 0;
	Oct_GI[21] = 0;
	Oct_GI[22] = 25;
	Oct_GI[23] = 0;
	Oct_GI[24] = 0;
	Oct_GI[25] = 0;
	Oct_GI[26] = 0;
	Oct_GI[27] = 0;
	Oct_GI[28] = 0;
	Oct_GI[29] = 0;
	Oct_GI[30] = 33;
	Oct_GI[31] = 0;
	Oct_GI[32] = 0;
	Oct_GI[33] = 0;
	Oct_GI[34] = 0;
	Oct_GI[35] = 0;
	Oct_GI[36] = 0;
	Oct_GI[37] = 0;
	Oct_GI[38] = 41;
	Oct_GI[39] = 0;
	Oct_GI[40] = 0;
	Oct_GI[41] = 0;
	Oct_GI[42] = 0;
	Oct_GI[43] = 0;
	Oct_GI[44] = 0;
	Oct_GI[45] = 0;
	Oct_GI[46] = 49;
	Oct_GI[47] = 0;
	Oct_GI[48] = 0;
	Oct_GI[49] = 0;
	Oct_GI[50] = 0;
	Oct_GI[51] = 0;
	Oct_GI[52] = 0;
	Oct_GI[53] = 0;
	Oct_GI[54] = 57;
	Oct_GI[55] = 0;
	Oct_GI[56] = 0;
	Oct_GI[57] = 0;
	Oct_GI[58] = 0;
	Oct_GI[59] = 0;
	Oct_GI[60] = 0;
	Oct_GI[61] = 0;
	Oct_GI[62] = 1;
	Oct_GI[63] = 0;
	Oct_GI[64] = 0;
	Oct_GI[65] = 73;
	Oct_GI[66] = 0;
	Oct_GI[67] = 0;
	Oct_GI[68] = 0;
	Oct_GI[69] = 0;
	Oct_GI[70] = 0;
	Oct_GI[71] = 0;
	Oct_GI[72] = 0;
	Oct_GI[73] = 81;
	Oct_GI[74] = 0;
	Oct_GI[75] = 0;
	Oct_GI[76] = 0;
	Oct_GI[77] = 0;
	Oct_GI[78] = 0;
	Oct_GI[79] = 0;
	Oct_GI[80] = 0;
	Oct_GI[81] = 89;
	Oct_GI[82] = 0;
	Oct_GI[83] = 0;
	Oct_GI[84] = 0;
	Oct_GI[85] = 0;
	Oct_GI[86] = 0;
	Oct_GI[87] = 0;
	Oct_GI[88] = 0;
	Oct_GI[89] = 97;
	Oct_GI[90] = 0;
	Oct_GI[91] = 0;
	Oct_GI[92] = 0;
	Oct_GI[93] = 0;
	Oct_GI[94] = 0;
	Oct_GI[95] = 0;
	Oct_GI[96] = 0;
	Oct_GI[97] = 105;
	Oct_GI[98] = 0;
	Oct_GI[99] = 0;
	Oct_GI[100] = 0;
	Oct_GI[101] = 0;
	Oct_GI[102] = 0;
	Oct_GI[103] = 0;
	Oct_GI[104] = 0;
	Oct_GI[105] = 113;
	Oct_GI[106] = 0;
	Oct_GI[107] = 0;
	Oct_GI[108] = 0;
	Oct_GI[109] = 0;
	Oct_GI[110] = 0;
	Oct_GI[111] = 0;
	Oct_GI[112] = 0;
	Oct_GI[113] = 1;*/
	/*int ii = 0;
for (; ii <= 113; ii++)
	Oct_GI[ii] = 0;
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

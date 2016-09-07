//--------------------------------------------------------------------------------------
// File: Tutorial07.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
#include "shaderheader.h"
SamplerState Sampler : register(s0);
SamplerState SamplerNone : register(s1);

Texture3D Voxel_SR : register(t0);
//Texture1D<unsigned int> Octree_SR : register(t1);
StructuredBuffer<uint> Octree_SR : register(t1);
StructuredBuffer<uint> count : register(t2);



cbuffer ConstantBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 info;
};

//--------------------------------------------------------------------------------------
// Structs for Geometry Shader
//--------------------------------------------------------------------------------------
//define the vertex shader input struct 

struct VS_INPUT
{
	float4 Position : POSITION;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL0;
};


// Define the vertex shader output struct 
struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float4 color   : COLOR;
};
//--------------
// VERTEX SHADER 
//------------
VS_OUTPUT VS(VS_INPUT vsInput)
{
	VS_OUTPUT vsOutput = (VS_OUTPUT)0;
	vsOutput.Position = vsInput.Position;
	vsOutput.color.r = 1;
	vsOutput.color.g = 0;
	vsOutput.color.b = 0;
	vsOutput.color.a = 1;
	return vsOutput;
}
//---------
// GEOMETRY SHADER
//---------
//  input topology    size of input array           comments
//  point             1                             a single vertex of input geometry
//  line              2                             two adjacent vertices
//  triangle          3                             three vertices of a triangle
//  lineadj           4                             two vertices defining a line segment, as well as vertices adjacent to the segment end points
//  triangleadj       6                             describes a triangle as well as 3 surrounding triangles

//  output topology        comments
//  PointStream            output is interpreted as a series of disconnected points
//  LineStream             each vertex in the list is connected with the next by a line segment
//  TriangleStream         the first 3 vertices in the stream will become the first triangle; any additional vertices become additional triangles (when combined with the previous 2)

float4 find_in_octree(float3 pos) {
	uint level = 0;
	uint octdex = 0;
	uint currdex = 0;
	float3 midpt = float3(0, 0, 0);
	float midsize = vxarea / 4.;
	uint idx = 0;

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
		uint octsr = Octree_SR[currdex];
		if (level + 1 != maxlevel) {
			if (octsr > 0)
				octdex = octsr;
			else
				return float4(midpt, 0);
		}
		else if (octsr == 1)
			return float4(midpt, level + 1);
		else
			return float4(midpt, 0);

		if (idx % 2 == 0) midpt.x -= midsize;
		else midpt.x += midsize;

		if (idx < 4) midpt.z -= midsize;
		else midpt.z += midsize;

		if (idx < 6 && idx > 1) midpt.y += midsize;
		else midpt.y -= midsize;

		midsize = midsize / 2.;

		level += 1;
	}
	
	return float4(midpt, 0);
}

/*float3 re_voxelspace(float3 worldpos)
{
	float f = 20. / 128;
	float3 pos = worldpos;
	pos.xyz *= f;
	pos.xyz -= float3(10, 10, 10);
	return pos;
}*/

/*[maxvertexcount(36)]   // produce a maximum of 3 output vertices
void GS(point VS_OUTPUT input[1], inout TriangleStream<VS_OUTPUT> triStream)
{
	
	float4x4 proj = Projection;
	float4x4 view = View;
	float4x4 world = World;
	float4 tri[36];
	//float f = 20. / 256.;
	float4 pos = input[0].Position;

	world._42 = 0;
	world._41 = 0;
	world._43 = 0;
	world._44 = 1;

	tri[0] = float4(0, 0, 0, 1);
	tri[1] = float4(0, 1, 0, 1);
	tri[2] = float4(1, 1, 0, 1);
	tri[3] = float4(0, 0, 0, 1);
	tri[4] = float4(1, 1, 0, 1);
	tri[5] = float4(1, 0, 0, 1);

	tri[7] = float4(0, 0, 1, 1);
	tri[6] = float4(0, 1, 1, 1);
	tri[8] = float4(1, 1, 1, 1);
	tri[9] = float4(0, 0, 1, 1);
	tri[11] = float4(1, 1, 1, 1);
	tri[10] = float4(1, 0, 1, 1);

	//ou
	tri[12] = float4(0, 1, 0, 1);
	tri[13] = float4(0, 1, 1, 1);
	tri[14] = float4(1, 1, 1, 1);
	tri[15] = float4(0, 1, 0, 1);
	tri[16] = float4(1, 1, 1, 1);
	tri[17] = float4(1, 1, 0, 1);

	tri[18] = float4(0, 0, 0, 1);
	tri[19] = float4(1, 0, 1, 1);
	tri[20] = float4(0, 0, 1, 1);
	tri[21] = float4(0, 0, 0, 1);
	tri[22] = float4(1, 0, 0, 1);
	tri[23] = float4(1, 0, 1, 1);

	//lr
	tri[24] = float4(0, 0, 0, 1);
	tri[25] = float4(0, 0, 1, 1);
	tri[26] = float4(0, 1, 1, 1);
	tri[27] = float4(0, 0, 0, 1);
	tri[28] = float4(0, 1, 1, 1);
	tri[29] = float4(0, 1, 0, 1);

	tri[31] = float4(1, 0, 0, 1);
	tri[30] = float4(1, 0, 1, 1);
	tri[32] = float4(1, 1, 1, 1);
	tri[33] = float4(1, 0, 0, 1);
	tri[35] = float4(1, 1, 1, 1);
	tri[34] = float4(1, 1, 0, 1);


	//int4 vgi = float4(pos.x + 0.0001, pos.y+ 0.0001, pos.z  + 0.0001, 0);
	
	//float4 col = (find_in_octree(pos.xyz)? float4(0,1,1,1) : float4(0,0,0,0));
	float4 col;
	pos.xyz = re_voxelspace(pos.xyz);
	if (find_in_octree(pos.xyz))
		col = float4(0, 1, 1, 1);
	else
		col == float4(0, 0, 0, 0);
	//float4 col = (pos.x == 0 ? float4(0, 1, 1, 1) : float4(0, 0, 0, 0));

	VS_OUTPUT psInput = (VS_OUTPUT)0;
	if (col.a > 0.01)
		for (uint i = 0; i < 36; i++)
		{
			pos = tri[i] + input[0].Position;
			//pos.w = 1;
			
			//pos.xyz=re_voxelspace(pos.xyz);

			pos = mul(pos, View);
			pos = mul(pos, Projection);

			psInput.Position = pos;
			
			float4 acol = col;

			if (i >= 28) acol.rgb *= 0.5;
			else if (i >= 22) acol.rgb *= 0.6;
			else if (i >= 16) acol.rgb *= 0.7;
			else if (i >= 10) acol.rgb *= 0.8;
			else if (i >= 4) acol.rgb*=0.9;
			else if (i >= 1) acol.rgb *= 0.95;			
			
			psInput.color = float4(acol.rgb, 1);

			triStream.Append(psInput);
			if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14 || i == 17 || i == 20 || i == 23 || i == 26 || i == 29 || i == 32)
				triStream.RestartStrip();
		}
	triStream.RestartStrip();
}*/

/*[maxvertexcount(36)]
void GS(point VS_OUTPUT input[1], inout TriangleStream<VS_OUTPUT> triStream)
{

	float4x4 proj = Projection;
	float4x4 view = View;
	float4x4 world = World;
	float4 tri[36];
	float4 pos = input[0].Position - float4(vxarea / 2, (float)vxarea / 2, (float)vxarea / 2,0); // level below - 1

	world._42 = 0;
	world._41 = 0;
	world._43 = 0;
	world._44 = 1;
	float ff = 0.5;
	tri[0] = float4(0, 0, 0, 1)*ff;
	tri[1] = float4(0, 1, 0, 1)*ff;
	tri[2] = float4(1, 1, 0, 1)*ff;
	tri[3] = float4(0, 0, 0, 1)*ff;
	tri[4] = float4(1, 1, 0, 1)*ff;
	tri[5] = float4(1, 0, 0, 1)*ff;

	tri[6] = float4(0, 1, 1, 1)*ff;
	tri[7] = float4(0, 0, 1, 1)*ff;
	tri[8] = float4(1, 1, 1, 1)*ff;
	tri[9] = float4(0, 0, 1, 1)*ff;
	tri[10] = float4(1, 0, 1, 1)*ff;
	tri[11] = float4(1, 1, 1, 1)*ff;

	//ou
	tri[12] = float4(0, 1, 0, 1)*ff;
	tri[13] = float4(0, 1, 1, 1)*ff;
	tri[14] = float4(1, 1, 1, 1)*ff;
	tri[15] = float4(0, 1, 0, 1)*ff;
	tri[16] = float4(1, 1, 1, 1)*ff;
	tri[17] = float4(1, 1, 0, 1)*ff;

	tri[18] = float4(0, 0, 0, 1)*ff;
	tri[19] = float4(1, 0, 1, 1)*ff;
	tri[20] = float4(0, 0, 1, 1)*ff;
	tri[21] = float4(0, 0, 0, 1)*ff;
	tri[22] = float4(1, 0, 0, 1)*ff;
	tri[23] = float4(1, 0, 1, 1)*ff;

	//lr
	tri[24] = float4(0, 0, 0, 1)*ff;
	tri[25] = float4(0, 0, 1, 1)*ff;
	tri[26] = float4(0, 1, 1, 1)*ff;
	tri[27] = float4(0, 0, 0, 1)*ff;
	tri[28] = float4(0, 1, 1, 1)*ff;
	tri[29] = float4(0, 1, 0, 1)*ff;

	
	tri[30] = float4(1, 0, 1, 1)*ff;
	tri[31] = float4(1, 0, 0, 1)*ff;
	tri[32] = float4(1, 1, 1, 1)*ff;
	tri[33] = float4(1, 0, 0, 1)*ff;
	tri[34] = float4(1, 1, 0, 1)*ff;
	tri[35] = float4(1, 1, 1, 1)*ff;

	float4 pos2 = find_in_octree(pos.xyz);

	VS_OUTPUT psInput = (VS_OUTPUT)0;
	if (pos2.a == maxlevel)
	{
		for (uint i = 0; i < 36; i++)
		{
			//float4 trimodvec = float4(0.5, 0.5, 0.5,1);
			float4 posfin = pos + tri[i];
			posfin.w = 1;


			float4 acol = float4(1, 1, 1, 1);
			////////////////
			if (pos.x < 0) //1 red
				if (pos.y < 0)
					if (pos.z < 0)
						acol = float4(1, 0, 0, 1);

			if (pos.x >= 0) //2 orange
				if (pos.y < 0)
					if (pos.z < 0)
						acol = float4(1, 0.3, 0, 1);

			if (pos.x < 0) //3 yellow
				if (pos.y >= 0)
					if (pos.z < 0)
						acol = float4(1, 1, 0, 1);

			if (pos.x >= 0) //4 green
				if (pos.y >= 0)
					if (pos.z < 0)
						acol = float4(0, 0.5, 0, 1);

			if (pos.x < 0) //5 blue
				if (pos.y >= 0)
					if (pos.z >= 0)
						acol = float4(0, 0, 1, 1);

			if (pos.x >= 0) //6 purple
				if (pos.y >= 0)
					if (pos.z >= 0)
						acol = float4(0.3, 0, 0.5, 1);

			if (pos.x < 0) //7 pink
				if (pos.y < 0)
					if (pos.z >= 0)
						acol = float4(0.9, 0.5, 0.9, 1);

			if (pos.x >= 0)
				if (pos.y < 0)
					if (pos.z >= 0)
						acol = float4(.9, .9, .9, 1);
			///////////////////////


			posfin = mul(posfin, View);
			posfin = mul(posfin, Projection);

			psInput.Position = posfin;

			

			if (i >= 28) acol.rgb *= 0.5;
			else if (i >= 22) acol.rgb *= 0.6;
			else if (i >= 16) acol.rgb *= 0.7;
			else if (i >= 10) acol.rgb *= 0.8;
			else if (i >= 4) acol.rgb *= 0.9;
			else if (i >= 1) acol.rgb *= 0.95;

			psInput.color = float4(acol.rgb, 1);

			triStream.Append(psInput);
			if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14 || i == 17 || i == 20 || i == 23 || i == 26 || i == 29 || i == 32)
				triStream.RestartStrip();
		}
	}
	triStream.RestartStrip();
}*/

[maxvertexcount(36)]
void GS(point VS_OUTPUT input[1], inout TriangleStream<VS_OUTPUT> triStream)
{

	float4x4 proj = Projection;
	float4x4 view = View;
	float4x4 world = World;
	float4 tri[36];
	float4 pos = input[0].Position - float4(vxarea / 2, (float)vxarea / 2, (float)vxarea / 2, 0); // level below - 1

	world._42 = 0;
	world._41 = 0;
	world._43 = 0;
	world._44 = 1;
	float ff = 1;
	tri[0] = float4(0, 0, 0, 1)*ff;
	tri[1] = float4(0, 1, 0, 1)*ff;
	tri[2] = float4(1, 1, 0, 1)*ff;
	tri[3] = float4(0, 0, 0, 1)*ff;
	tri[4] = float4(1, 1, 0, 1)*ff;
	tri[5] = float4(1, 0, 0, 1)*ff;

	tri[6] = float4(0, 1, 1, 1)*ff;
	tri[7] = float4(0, 0, 1, 1)*ff;
	tri[8] = float4(1, 1, 1, 1)*ff;
	tri[9] = float4(0, 0, 1, 1)*ff;
	tri[10] = float4(1, 0, 1, 1)*ff;
	tri[11] = float4(1, 1, 1, 1)*ff;

	//ou
	tri[12] = float4(0, 1, 0, 1)*ff;
	tri[13] = float4(0, 1, 1, 1)*ff;
	tri[14] = float4(1, 1, 1, 1)*ff;
	tri[15] = float4(0, 1, 0, 1)*ff;
	tri[16] = float4(1, 1, 1, 1)*ff;
	tri[17] = float4(1, 1, 0, 1)*ff;

	tri[18] = float4(0, 0, 0, 1)*ff;
	tri[19] = float4(1, 0, 1, 1)*ff;
	tri[20] = float4(0, 0, 1, 1)*ff;
	tri[21] = float4(0, 0, 0, 1)*ff;
	tri[22] = float4(1, 0, 0, 1)*ff;
	tri[23] = float4(1, 0, 1, 1)*ff;

	//lr
	tri[24] = float4(0, 0, 0, 1)*ff;
	tri[25] = float4(0, 0, 1, 1)*ff;
	tri[26] = float4(0, 1, 1, 1)*ff;
	tri[27] = float4(0, 0, 0, 1)*ff;
	tri[28] = float4(0, 1, 1, 1)*ff;
	tri[29] = float4(0, 1, 0, 1)*ff;


	tri[30] = float4(1, 0, 1, 1)*ff;
	tri[31] = float4(1, 0, 0, 1)*ff;
	tri[32] = float4(1, 1, 1, 1)*ff;
	tri[33] = float4(1, 0, 0, 1)*ff;
	tri[34] = float4(1, 1, 0, 1)*ff;
	tri[35] = float4(1, 1, 1, 1)*ff;

	float4 pos2 = find_in_octree(pos.xyz);

	VS_OUTPUT psInput = (VS_OUTPUT)0;
	if (pos2.a == maxlevel)
	{
		for (uint i = 0; i < 36; i++)
		{
			//float4 trimodvec = float4(0.5, 0.5, 0.5,1);
			float4 posfin = pos + tri[i];
			posfin.w = 1;


			float4 acol = float4(1, 1, 1, 1);
			////////////////
			if (pos.x < 0) //1 red
				if (pos.y < 0)
					if (pos.z < 0)
						acol = float4(1, 0, 0, 1);

			if (pos.x >= 0) //2 orange
				if (pos.y < 0)
					if (pos.z < 0)
						acol = float4(1, 0.3, 0, 1);

			if (pos.x < 0) //3 yellow
				if (pos.y >= 0)
					if (pos.z < 0)
						acol = float4(1, 1, 0, 1);

			if (pos.x >= 0) //4 green
				if (pos.y >= 0)
					if (pos.z < 0)
						acol = float4(0, 0.5, 0, 1);

			if (pos.x < 0) //5 blue
				if (pos.y >= 0)
					if (pos.z >= 0)
						acol = float4(0, 0, 1, 1);

			if (pos.x >= 0) //6 purple
				if (pos.y >= 0)
					if (pos.z >= 0)
						acol = float4(0.3, 0, 0.5, 1);

			if (pos.x < 0) //7 pink
				if (pos.y < 0)
					if (pos.z >= 0)
						acol = float4(0.9, 0.5, 0.9, 1);

			if (pos.x >= 0)
				if (pos.y < 0)
					if (pos.z >= 0)
						acol = float4(.9, .9, .9, 1);
			///////////////////////


			posfin = mul(posfin, View);
			posfin = mul(posfin, Projection);

			psInput.Position = posfin;



			if (i >= 28) acol.rgb *= 0.5;
			else if (i >= 22) acol.rgb *= 0.6;
			else if (i >= 16) acol.rgb *= 0.7;
			else if (i >= 10) acol.rgb *= 0.8;
			else if (i >= 4) acol.rgb *= 0.9;
			else if (i >= 1) acol.rgb *= 0.95;

			psInput.color = float4(acol.rgb, 1);

			triStream.Append(psInput);
			if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14 || i == 17 || i == 20 || i == 23 || i == 26 || i == 29 || i == 32)
				triStream.RestartStrip();
		}
	}
	triStream.RestartStrip();
}

/*void GS(point VS_OUTPUT input[1], inout TriangleStream<VS_OUTPUT> triStream)
{

	float4x4 proj = Projection;
	float4x4 view = View;
	float4x4 world = World;
	float4 tri[36];
	float f = 20. / 256.;
	float4 pos = input[0].Position;

	world._42 = 0;
	world._41 = 0;
	world._43 = 0;
	world._44 = 1;

	tri[0] = float4(0, 0, 0, 1);
	tri[1] = float4(0, 1, 0, 1);
	tri[2] = float4(1, 1, 0, 1);
	tri[3] = float4(0, 0, 0, 1);
	tri[4] = float4(1, 1, 0, 1);
	tri[5] = float4(1, 0, 0, 1);

	tri[7] = float4(0, 0, 1, 1);
	tri[6] = float4(0, 1, 1, 1);
	tri[8] = float4(1, 1, 1, 1);
	tri[9] = float4(0, 0, 1, 1);
	tri[11] = float4(1, 1, 1, 1);
	tri[10] = float4(1, 0, 1, 1);

	//ou
	tri[12] = float4(0, 1, 0, 1);
	tri[13] = float4(0, 1, 1, 1);
	tri[14] = float4(1, 1, 1, 1);
	tri[15] = float4(0, 1, 0, 1);
	tri[16] = float4(1, 1, 1, 1);
	tri[17] = float4(1, 1, 0, 1);

	tri[18] = float4(0, 0, 0, 1);
	tri[19] = float4(1, 0, 1, 1);
	tri[20] = float4(0, 0, 1, 1);
	tri[21] = float4(0, 0, 0, 1);
	tri[22] = float4(1, 0, 0, 1);
	tri[23] = float4(1, 0, 1, 1);

	//lr
	tri[24] = float4(0, 0, 0, 1);
	tri[25] = float4(0, 0, 1, 1);
	tri[26] = float4(0, 1, 1, 1);
	tri[27] = float4(0, 0, 0, 1);
	tri[28] = float4(0, 1, 1, 1);
	tri[29] = float4(0, 1, 0, 1);

	tri[31] = float4(1, 0, 0, 1);
	tri[30] = float4(1, 0, 1, 1);
	tri[32] = float4(1, 1, 1, 1);
	tri[33] = float4(1, 0, 0, 1);
	tri[35] = float4(1, 1, 1, 1);
	tri[34] = float4(1, 1, 0, 1);


	int4 vgi = float4(pos.x + 0.0001, pos.y + 0.0001, pos.z + 0.0001, 0);

	float4 col = Voxel_SR.Load(vgi);

	VS_OUTPUT psInput = (VS_OUTPUT)0;
	if (col.a>0.01)
		for (uint i = 0; i < 36; i++)
		{
			pos = tri[i] + input[0].Position;
			pos.w = 1;

			pos.xyz = re_voxelspace(pos.xyz);

			pos = mul(pos, View);
			pos = mul(pos, Projection);

			psInput.Position = pos;

			float4 acol = col;

			if (i >= 28) acol.rgb *= 0.5;
			else if (i >= 22) acol.rgb *= 0.6;
			else if (i >= 16) acol.rgb *= 0.7;
			else if (i >= 10) acol.rgb *= 0.8;
			else if (i >= 4) acol.rgb *= 0.9;
			else if (i >= 1) acol.rgb *= 0.95;

			psInput.color = float4(acol.rgb, 1);

			triStream.Append(psInput);
			if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14 || i == 17 || i == 20 || i == 23 || i == 26 || i == 29 || i == 32)
				triStream.RestartStrip();
		}
	triStream.RestartStrip();
}*/

//---------------
// Pixel Shader
//-----------------

float4 PS(VS_OUTPUT dataIn) : SV_TARGET
{
	
	float4 col = dataIn.color;
	col.a = 1;
	return col;
}

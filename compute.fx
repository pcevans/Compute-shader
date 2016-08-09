#include "shaderheader.h"


RWTexture1D<unsigned int> Oct_GI : register(u1);
RWTexture1D<float4> VFL : register(u2);
RWTexture1D<unsigned int> cunt : register(u3);

groupshared uint 

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

uint put_in_octree(float3 pos, uint level) {
	//uint level = 0;
	uint octdex = 3;
	uint currdex = 0;
	uint flag;
	float3 midpt = float3(0, 0, 0);
	float midsize = vxarea / 4.;
	uint idx = 0;
	if (Oct_GI[0] == 0)
		Oct_GI[0] = 8 + octdex;

	if (level == 0)
		flag = 1;
	else flag = 2;

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
		if (level + 1 != maxlevel)
			octdex = Oct_GI[currdex];
		else
			Oct_GI[currdex] = 2;

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

void index_octree(int curridx) {
	//int start = Oct_GI[2];
	//int end = Oct_GI[1];
	//int nextidx;
	
	//InterlockedAdd(Oct_GI[0], 8, nextidx);
	//Oct_GI[curridx] = nextidx;
}

[numthreads(NUM_THREADS, 1, 1)]	// means, DTid.x goes from 0 to 511 !!!!!!!!!
void CS(uint3 DTid : SV_DispatchThreadID)
{
	//how many passes do you need? We have DTid.x going from 0 to 511
	int numpassesflag = ceil(VFL[0] / NUM_THREADS);
	int currindex;

	//go from 0 to VFL[0]
	for (int i = 0; i < numpassesflag; i++) {
		InterlockedAdd(?????, 1, currindex); // put incrementing variable here

		if (currindex < VFL[0])
			put_in_octree(VFL[currindex], ??level?? ); // put level variable in here
	}				
}

[numthreads(NUM_THREADS, 1, 1)]	// means, DTid.x goes from 0 to 511 !!!!!!!!!
void CS2(uint3 DTid : SV_DispatchThreadID)
{
	//how many passes do you need? We have DTid.x going from 0 to 511
	int numpassesindex = ceil((Oct_GI[1] - Oct_GI[2]) / NUM_THREADS);
	int currindex;

	//go from Oct_GI[1] to Oct_GI[0]
	for (int i = 0; i < numpassesindex; i++) {
		InterlockedAdd(?????, 1, currindex); // put incrementing variable here
		//if (currindex is between Oct_GI[2] and Oct_GI[1])
			index_octree(currindex);
	}
	//when all threads are done,
	//Oct_GI[2] = Oct_GI[1];
	//Oct_GI[1] = Oct_GI[0];
}
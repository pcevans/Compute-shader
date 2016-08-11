#include "shaderheader.h"

RWTexture1D<unsigned int> Octree_RW : register(u1);
RWTexture1D<float> VFL : register(u2);
RWTexture1D<unsigned int> count : register(u3);

cbuffer ConstantBuffer : register(b0)
{
	float4 CBvalues; 
};

uint put_in_octree(float3 pos, uint level) {
	uint octdex = 0;
	uint currdex = 0;
	uint flag;
	float3 midpt = float3(0, 0, 0);
	float midsize = vxarea / 4.;
	uint idx = 0;
	if (Octree_RW[0] == 0)
		Octree_RW[0] = 8 + octdex;

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

////	count[4]
//		0 - vfl count
//		1 - octree last index
//		2 - octree next index = current total number of elements in the 1d octree array
//		3 - vfl index counter (cs)
//		4 - octree index counter (cs2) = same as [2], but can be changed during CS2
////

[numthreads(NUM_THREADS, 1, 1)]	// means, DTid.x goes from 0 to 511 !!!!!!!!!
void CS(uint3 DTid : SV_DispatchThreadID)
	{
	
	if (DTid.x == 0)
		{
		count[1] = count[2];	//updates after CS2
		count[2] = count[4];		//updates after CS2
		}
	//how many passes do you need? We have DTid.x going from 0 to 511
	float fnumpassesflag = ceil((float)count[0] / (float)NUM_THREADS);
	int numpassesflag = (int)fnumpassesflag;
	uint octdex = 0;
	uint currdex = 0;
	int maxlevel = (int)CBvalues.x;
	float3 midpt = float3(0, 0, 0);
	float midsize = vxarea / 4.;
	uint idx = 0;
	//go from 0 to count[0]
	[allow_uav_condition]
	for (int i = 0; i < numpassesflag; i++)
		{
		uint vocel_to_work_on = DTid.x + NUM_THREADS*i;
		if (vocel_to_work_on >= count[0])break;

		
		float px = VFL[vocel_to_work_on * 3 + 0];
		float py = VFL[vocel_to_work_on * 3 + 1];
		float pz = VFL[vocel_to_work_on * 3 + 2];
		float3 pos = float3(px, py, pz);
		[allow_uav_condition]
		for (int level = 0; level < maxlevel; level++)
			{
			if (pos.x < midpt.x)
				{
				if (pos.y < midpt.y)
					{
					if (pos.z < midpt.z) idx = 0;
					else idx = 6;
					}
				else
					{
					if (pos.z < midpt.z) idx = 2;
					else idx = 4;
					}
				}
			else
				{
				if (pos.y < midpt.y)
					{
					if (pos.z < midpt.z) idx = 1;
					else idx = 7;
					}
				else
					{
					if (pos.z < midpt.z) idx = 3;
					else idx = 5;
					}
				}

			currdex = octdex + idx;

			if (idx % 2 == 0) midpt.x -= midsize;
			else midpt.x += midsize;

			if (idx < 4) midpt.z -= midsize;
			else midpt.z += midsize;

			if (idx < 6 && idx > 1) midpt.y += midsize;
			else midpt.y -= midsize;

			midsize = midsize / 2.;
			}

		Octree_RW[currdex] = 2;

		}


	}

[numthreads(NUM_THREADS, 1, 1)]	// means, DTid.x goes from 0 to 511 !!!!!!!!!
void CS2(uint3 DTid : SV_DispatchThreadID)
{

	//how many passes do you need? We have DTid.x going from 0 to 511

	int num_of_flag_area = count[2] - count[1];
	float fnumpassesflag = ceil((float)num_of_flag_area / (float)NUM_THREADS);
	int numpassesflag = (int)fnumpassesflag;


	[allow_uav_condition]
	for (int i = 0; i < numpassesflag; i++)
		{
		int ocv_to_work_on = DTid.x + NUM_THREADS*i;
		if (ocv_to_work_on >= num_of_flag_area)break;
		ocv_to_work_on += count[1];
		if (Octree_RW[ocv_to_work_on] != 2)continue;
		uint free_space = 0;
		InterlockedAdd(count[4], 8, free_space);
		
		Octree_RW[ocv_to_work_on] = free_space;
		}

}
/*
[numthreads(NUM_THREADS, 1, 1)]	// means, DTid.x goes from 0 to 511 !!!!!!!!!
void CS2(uint3 DTid : SV_DispatchThreadID)
	{
	count[1] = count[2];	//updates after CS2
	count[2] = count[4];		//updates after CS2
	}*/
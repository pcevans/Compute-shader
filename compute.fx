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

[numthreads(1, 1, 1)]
void CSstart(uint3 DTid : SV_DispatchThreadID) 
{
	count[2] = 0;
	count[4] = 8;
}

[numthreads(NUM_THREADS, 1, 1)]	// means, DTid.x goes from 0 to NUM_THREADS !!!!!!!!!
void CS(uint3 DTid : SV_DispatchThreadID)
	{
	if (DTid.x == 0)
	{
		count[1] = count[2];	//updates after CS2
		count[2] = count[4];	//updates after CS2
	}
	
	//how many passes do you need? We have DTid.x going from 0 to NUM_THREADS
	float fnumpassesflag = ceil((float)count[0] / (float)NUM_THREADS);
	int numpassesflag = (int)fnumpassesflag;

	uint octdex = 0;
	uint currdex = 0;
	int currlevel = (int)CBvalues.x;
	float3 midpt = float3(0, 0, 0);
	float midsize = vxarea / 4.;
	uint idx = 0;

	if (DTid.x != 0)
	{
		//return;
	}

	//go from 0 to count[0]
	[allow_uav_condition]
	for (int i = 0; i < numpassesflag; i++)
		{
		uint voxel_to_work_on = DTid.x + NUM_THREADS*i;
		if (voxel_to_work_on >= count[0]) break;

		
		/*float px = VFL[voxel_to_work_on * 3 + 0];
		float py = VFL[voxel_to_work_on * 3 + 1];
		float pz = VFL[voxel_to_work_on * 3 + 2];
		float3 pos = float3(px, py, pz);*/

		float3 pos = float3(7,7,7);
		if (DTid.x ==1)
			pos = float3(-7, 7, 7);
		[allow_uav_condition]
		for (int xlevel = 0; xlevel < currlevel; xlevel++)
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
			octdex = Octree_RW[currdex];

			if (idx % 2 == 0) midpt.x -= midsize;
			else midpt.x += midsize;

			if (idx < 4) midpt.z -= midsize;
			else midpt.z += midsize;

			if (idx < 6 && idx > 1) midpt.y += midsize;
			else midpt.y -= midsize;

			midsize = midsize / 2.;
			}

		Octree_RW[currdex] = 1;

		}


	}

[numthreads(NUM_THREADS, 1, 1)]	// means, DTid.x goes from 0 to 511 !!!!!!!!!
void CS2(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x != 5)
	{
		//return;
	}
	//how many passes do you need? We have DTid.x going from 0 to 511

	int num_of_flag_area = count[2] - count[1];
	float fnumpassesflag = ceil((float)num_of_flag_area / (float)NUM_THREADS);
	int numpassesflag = (int)fnumpassesflag;

	[allow_uav_condition]
	for (int i = 0; i < numpassesflag; i++)
		{
		if (CBvalues.x == maxlevel) 
			break; // don't index the last level

		int ocv_to_work_on = DTid.x + NUM_THREADS*i;
		if (ocv_to_work_on >= num_of_flag_area) 
			break; // don't run if there are no more indices to work on

		ocv_to_work_on += count[1];
		if (Octree_RW[ocv_to_work_on] != 1) 
			continue; // go to next iteration of loop if not flagged

		uint free_space = 0;
		InterlockedAdd(count[4], 8, free_space);
		
		Octree_RW[ocv_to_work_on] = free_space;
		}

}

[numthreads(NUM_THREADS, 1, 1)]	// means, DTid.x goes from 0 to 511 !!!!!!!!!
void CSfake(uint3 DTid : SV_DispatchThreadID)
{

	if (DTid.x == 0)
	{
		Octree_RW[0] = 0;
		Octree_RW[1] = 0;
		Octree_RW[2] = 0;
		Octree_RW[3] = 0;
		Octree_RW[4] = 0;
		Octree_RW[5] = 8;
		Octree_RW[6] = 0;
		Octree_RW[7] = 0;
		Octree_RW[8] = 16;
		Octree_RW[9] = 0;
		Octree_RW[10] = 0;
		Octree_RW[11] = 0;
		Octree_RW[12] = 0;
		Octree_RW[13] = 0;
		Octree_RW[14] = 0;
		Octree_RW[15] = 0;
		Octree_RW[16] = 0;
		Octree_RW[17] = 0;
		Octree_RW[18] = 0;
		Octree_RW[19] = 0;
		Octree_RW[20] = 0;
		Octree_RW[21] = 1;
		Octree_RW[22] = 0;
		Octree_RW[23] = 0;
		Octree_RW[24] = 0;
		Octree_RW[25] = 0;
		Octree_RW[26] = 0;
		Octree_RW[27] = 0;
		Octree_RW[28] = 0;
		Octree_RW[29] = 0;
		Octree_RW[30] = 0;
		Octree_RW[31] = 0;
		Octree_RW[32] = 0;
		Octree_RW[33] = 0;
		Octree_RW[34] = 0;
		Octree_RW[35] = 0;
		Octree_RW[36] = 0;
		Octree_RW[37] = 0;
		Octree_RW[38] = 0;
		Octree_RW[39] = 0;
		Octree_RW[40] = 0;
		Octree_RW[41] = 0;
		Octree_RW[42] = 0;
		Octree_RW[43] = 0;
		Octree_RW[44] = 0;
		Octree_RW[45] = 0;
		Octree_RW[46] = 0;
		Octree_RW[47] = 0;
		Octree_RW[48] = 0;
		Octree_RW[49] = 0;
		Octree_RW[50] = 0;
		Octree_RW[51] = 0;
		Octree_RW[52] = 0;
		Octree_RW[53] = 0;
		Octree_RW[54] = 0;
		Octree_RW[55] = 0;
		Octree_RW[56] = 0;
		Octree_RW[57] = 0;
		Octree_RW[58] = 0;
		Octree_RW[59] = 0;
		Octree_RW[60] = 0;
		Octree_RW[61] = 0;
		Octree_RW[62] = 0;
		Octree_RW[63] = 0;
		Octree_RW[64] = 0;
		Octree_RW[65] = 0;
		Octree_RW[66] = 0;
		Octree_RW[67] = 0;
		Octree_RW[68] = 0;
		Octree_RW[69] = 0;
		Octree_RW[70] = 0;
		Octree_RW[71] = 0;
		Octree_RW[72] = 0;
		Octree_RW[73] = 0;
		Octree_RW[74] = 0;
		Octree_RW[75] = 0;
		Octree_RW[76] = 0;
		Octree_RW[77] = 0;
		Octree_RW[78] = 0;
		Octree_RW[79] = 0;
		Octree_RW[80] = 0;
		Octree_RW[81] = 0;
		Octree_RW[82] = 0;
		Octree_RW[83] = 0;
		Octree_RW[84] = 0;
		Octree_RW[85] = 0;
		Octree_RW[86] = 0;
		Octree_RW[87] = 0;
		Octree_RW[88] = 0;
		Octree_RW[89] = 0;
		Octree_RW[90] = 0;
		Octree_RW[91] = 0;
		Octree_RW[92] = 0;
		Octree_RW[93] = 0;
		Octree_RW[94] = 0;
		Octree_RW[95] = 0;
		Octree_RW[96] = 0;
		Octree_RW[97] = 0;
		Octree_RW[98] = 0;
		Octree_RW[99] = 0;

	}

}
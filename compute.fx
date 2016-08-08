

#define NUM_THREADS 256

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

[numthreads(NUM_THREADS, 1, 1)]	// means, DTid.x goes from 0 to 255 !!!!!!!!!
void cs(uint3 DTid : SV_DispatchThreadID) //cs: you can have any name here, but must correspond to the c++ program like the names of a PS
	{
	
	//how many passes do you need? We have DTid.x going from 0 to 255

	//do some stuff like:
	//Voxel_GI[int3(second_pos.x, second_pos.y, second_pos.z)] = float4(color.rgb, 0.5);

				
	}
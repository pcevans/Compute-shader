#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include "resource.h"
//using namespace std;

#define EPS 0.00001

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
	XMFLOAT3 Normal;
};

class ConstantBuffer
{
public:
	ConstantBuffer()
	{
		info = XMFLOAT4(1, 1, 1, 1);
		SunPos = XMFLOAT4(0, 0, 1000, 1);
		DayTimer = XMFLOAT4(0, 0, 0, 0);
	}
	XMMATRIX World;
	XMMATRIX View;
	XMMATRIX Projection;
	XMFLOAT4 info;
	XMFLOAT4 CameraPos;  
	XMFLOAT4 SunPos;
	XMFLOAT4 DayTimer;
};
class ConstantBufferCS
{
public:
	ConstantBufferCS()
	{
		values = XMFLOAT4(0, 0, 0, 0);
	}
	XMFLOAT4 values;
};
//********************************************
class StopWatchMicro_
{
private:
	LARGE_INTEGER last, frequency;
public:
	StopWatchMicro_()
	{
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&last);
	}

	long double elapse_micro()
	{
		LARGE_INTEGER now, dif;
		QueryPerformanceCounter(&now);
		dif.QuadPart = now.QuadPart - last.QuadPart;
		long double fdiff = (long double)dif.QuadPart;
		fdiff /= (long double)frequency.QuadPart;
		return fdiff*1000000.;
	}

	long double elapse_milli()
	{
		elapse_micro() / 1000.;
	}

	void start()
	{
		QueryPerformanceCounter(&last);
	}
};

//********************************************
class billboard
{
public:
	XMFLOAT3 position;
	float scale;
	float rotation;
	float transparency;
	
	billboard()
	{
		position = XMFLOAT3(0, 0, 0);
		scale = 1;
		transparency = 1;
	}

	XMMATRIX get_matrix(XMMATRIX &ViewMatrix)
	{
		XMMATRIX view, R, Ry, T, S;
		Ry = XMMatrixRotationX(-rotation);
		view = Ry * ViewMatrix;
		//eliminate camera translation:
		view._41 = view._42 = view._43 = 0.0;
		XMVECTOR det;
		R = XMMatrixInverse(&det, view); //inverse rotation
		T = XMMatrixTranslation(position.x, position.y, position.z);
		S = XMMatrixScaling(scale, scale, scale);
		return S*R*T*Ry;


	}

	XMMATRIX get_matrix_y(XMMATRIX &ViewMatrix) { }
};

//********************************************
class camera
{
public:
	int w, s, a, d, q, e;
	XMFLOAT3 position;
	XMFLOAT3 rotation;

	camera()
	{
		w = s = a = d = q = e = 0;
		position = rotation = XMFLOAT3(0, 0, 0);
	}

	void animation(float elapsed_microseconds)
	{
		XMMATRIX Rx, Ry, T;
		Ry = XMMatrixRotationY(-rotation.y);
		Rx = XMMatrixRotationX(-rotation.x);

		XMFLOAT3 forward = XMFLOAT3(0, 0, 1);
		XMVECTOR f = XMLoadFloat3(&forward);
		f = XMVector3TransformCoord(f, Rx*Ry);
		XMStoreFloat3(&forward, f);
		XMFLOAT3 side = XMFLOAT3(1, 0, 0);
		XMVECTOR si = XMLoadFloat3(&side);
		si = XMVector3TransformCoord(si, Rx*Ry);
		XMStoreFloat3(&side, si);
		XMFLOAT3 up = XMFLOAT3(0, 1, 0);
		XMVECTOR u = XMLoadFloat3(&up);
		u = XMVector3TransformCoord(u, Rx*Ry);
		XMStoreFloat3(&up, u);

		float speed = elapsed_microseconds / 10000.0;

		if (w)
		{
			position.x -= forward.x * speed;
			position.y -= forward.y * speed;
			position.z -= forward.z * speed;
		}
		if (s)
		{
			position.x += forward.x * speed;
			position.y += forward.y * speed;
			position.z += forward.z * speed;
		}
		if (d)
		{
			position.x -= side.x * speed;
			position.y -= side.y * speed;
			position.z -= side.z * speed;
		}
		if (a)
		{
			position.x += side.x * speed;
			position.y += side.y * speed;
			position.z += side.z * speed;
		}
		
		if (e)
		{
			position.x -= up.x * speed;
			position.y -= up.y * speed;
			position.z -= up.z * speed;
		}
		if (q)
		{
			position.x += up.x * speed;
			position.y += up.y * speed;
			position.z += up.z * speed;
		}
	}

	XMMATRIX get_matrix(XMMATRIX *view)
	{
		XMFLOAT2 unit_look;
		XMFLOAT3 unit_normal;
		XMMATRIX RY, RXZ, T;

		int x, y, z;

		//if (rotation.x >= (XM_PI/2) - 0.5) rotation.x = ((XM_PI/2) - 0.5);
		//if (rotation.x <= (-XM_PI/2) + 0.5) rotation.x = ((-XM_PI/2) + 0.5);
		if (rotation.x >= (XM_PI / 2)) rotation.x = ((XM_PI / 2));
		if (rotation.x <= (-XM_PI / 2)) rotation.x = ((-XM_PI / 2));

		unit_look = { cos(rotation.y), sin(-rotation.y) }; //make vector facing z (z = 1, x = 0)
		XMVECTOR ul = XMLoadFloat2(&unit_look);
		ul = XMVector2Orthogonal(XMVector2Normalize(ul)); // get axis of rotation (z = 0, x = 1)
		XMStoreFloat2(&unit_look, ul);

		unit_normal.x = unit_look.y;
		unit_normal.y = 0;
		unit_normal.z = unit_look.x;
		XMVECTOR ra = XMLoadFloat3(&unit_normal);

		RY = XMMatrixRotationY(rotation.y);
		RXZ = XMMatrixRotationAxis(ra, rotation.x);
		T = XMMatrixTranslation(position.x, position.y, position.z);
		return T*(*view) * RXZ * RY;
	}
};

//********************************************
class voxel_
{
public:
	SimpleVertex *v;
	ID3D11Buffer *vbuffer;

	int anz;
	int size;
	voxel_()
	{
		vbuffer = NULL;
		size = 256;
		anz = pow(size,3);
		v = new SimpleVertex[anz];
		for (int xx = 0; xx < size; xx++)
			for (int yy = 0; yy < size; yy++)
				for (int zz = 0; zz < size; zz++)
				{
					v[xx + yy * size + zz * size * size].Pos.x = xx;
					v[xx + yy * size + zz * size * size].Pos.y = yy;
					v[xx + yy * size + zz * size * size].Pos.z = zz;
				}
	}

	~voxel_()
	{
		delete[] v;
	}
};

/////////////////////////////////
class octree
{
public:
	float* octray;
	int octdex;
	int size;

	octree()
	{
		int level = 1;
		int numnodes = (8 ^ (level + 1) - 1) / 7;
		octray = new float[numnodes];
		octray[0] = 1;
		for (int i = 1; i < 9; i++) 
			octray[i] = -1;
		octdex = 9;
	}
	
	//buildOctree(1,1,XMFLOAT3(0,0,0),arrlen,arr);
	//precondition: all points inside octree area
	int buildOctree(int index, int level, XMFLOAT3 midpt, int arrlen, XMFLOAT3 *arr)
	{

		XMFLOAT3 *subarr[8];
		int subarrlen[8] = { 0 };

		for (int i = 0; i < 8; i++) {
			subarr[i] = new XMFLOAT3[1000];
		}
		
		for (int i = 0; i < arrlen; i++) {
			int idx = -1;
			if (arr[i].x < midpt.x) {
				if (arr[i].y < midpt.y) {
					if (arr[i].z < midpt.z) idx = 0;
					else idx = 6;
				}
				else {
					if (arr[i].z < midpt.z) idx = 2;
					else idx = 4;
				}
			}
			else {
				if (arr[i].y < midpt.y) {
					if (arr[i].z < midpt.z) idx = 1;
					else idx = 7;
				}
				else {
					if (arr[i].z < midpt.z) idx = 3;
					else idx = 5;
				}
			}
			subarr[idx][subarrlen[idx]] = arr[i];
			subarrlen[idx]++;
		}
		
		for (int i = 0; i < 8; i++) {
			if (subarrlen[i] != 0) {
				if (level == 8) {
					octray[index + i] = 0;
				}
				else {
					octray[index + i] = octdex;
					octdex += 8;

					float octlevel = (size / pow(2, level)) / 2;
					XMFLOAT3 octmid = midpt;

					if (i % 2 == 0) octmid.x -= octlevel;
					else octmid.x += octlevel;

					if (i < 4) octmid.z -= octlevel;
					else octmid.z += octlevel;

					if (i < 6 && i > 1) octmid.y += octlevel;
					else octmid.y -= octlevel;

					buildOctree(octdex - 8, level + 1, octmid, subarrlen[i], subarr[i]);
				}
			}
			else
				octray[index + i] = -1;
		}

		return 1;
	}
};
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include <fstream>
#include <io.h>
#include "resource.h"
#include "shaderheader.h"
#include <vector>
using namespace std;

#define EPS 0.00001

/*struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
	XMFLOAT3 Normal;
};*/




float Vec3Length(const XMFLOAT3 &v);
float Vec3Dot(XMFLOAT3 a, XMFLOAT3 b);
XMFLOAT3 Vec3Cross(XMFLOAT3 a, XMFLOAT3 b);
XMFLOAT3 Vec3Normalize(const  XMFLOAT3 &a);
XMFLOAT3 operator+(const XMFLOAT3 lhs, const XMFLOAT3 rhs);
XMFLOAT3 operator*(const XMFLOAT3 lhs, const float rhs);
XMFLOAT3 operator-(const XMFLOAT3 lhs, const XMFLOAT3 rhs);
bool Load(char *filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count,float scale = 1.0);


class SimpleVertex
{
public:
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
	XMFLOAT3 Normal;
	XMFLOAT3 Tangent;
	XMFLOAT3 BiNormal;
	SimpleVertex()
	{
		Pos = Normal = Tangent = BiNormal = XMFLOAT3(0, 0, 0);
		Tex = XMFLOAT2(0, 0);
	}
	void load(XMFLOAT3 p, XMFLOAT2 tuv, XMFLOAT3 n)
	{
		Pos = p;
		Tex = tuv;
		Normal = n;
	}
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
		return elapse_micro() / 1000.;
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

		float speed = elapsed_microseconds / 10000.0f;

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
		size = vxarea;
		anz = (int)pow(size,3);
		v = new SimpleVertex[anz];
		for (int xx = 0; xx < size; xx++)
			for (int yy = 0; yy < size; yy++)
				for (int zz = 0; zz < size; zz++)
				{
					v[xx + yy * size + zz * size * size].Pos.x = (float)xx;
					v[xx + yy * size + zz * size * size].Pos.y = (float)yy;
					v[xx + yy * size + zz * size * size].Pos.z = (float)zz;
				}
	}

	~voxel_()
	{
		delete[] v;
	}
};
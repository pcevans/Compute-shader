#include "groundwork.h"

bool similar_pos(XMFLOAT3 a, XMFLOAT3 b, float crit)
{
	XMFLOAT3 c = a - b;
	if (abs(c.x) < crit && abs(c.y) < crit && abs(c.z) < crit)
		return TRUE;

	return FALSE;
}
class submodel
{
public:
	USHORT *indizes;
	int ianz;
	SimpleVertex *vertices;
	int vanz;
	submodel()
	{
		indizes = NULL;
		ianz = 0;
		vertices = NULL;
		vanz = 0;
	}
	~submodel()
	{
		int z;
		z = 0;
		//if (indizes)	delete[]indizes;
		//if (vertices)	delete[]vertices;
	}
};
//***************************************************************************************
double winkel_vec_xy(XMFLOAT3 v)
{
	v = Vec3Normalize(v);

	double w = acos(v.y);
	if (w != 0.0 && v.x < 0.)
		w = (XM_PI*2.0) - w;
	return w;
}
//***************************************************************************************
void CalculateTangentArray(long vertexCount, SimpleVertex *vertex)
{
	XMFLOAT3 *tan1 = new XMFLOAT3[vertexCount * 2];
	XMFLOAT3 *tan2 = tan1 + vertexCount;

	for (long a = 0; a < (vertexCount * 2); a++)
		tan1[a] = XMFLOAT3(0, 0, 0);
	for (long a = 0; a < vertexCount; a += 3)
	{
		long i1 = a + 0;
		long i2 = a + 1;
		long i3 = a + 2;

		const XMFLOAT3& v1 = vertex[i1].Pos;
		const XMFLOAT3& v2 = vertex[i2].Pos;
		const XMFLOAT3& v3 = vertex[i3].Pos;

		const XMFLOAT2& w1 = vertex[i1].Tex;
		const XMFLOAT2& w2 = vertex[i2].Tex;
		const XMFLOAT2& w3 = vertex[i3].Tex;

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.x - w1.x;
		float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y;
		float t2 = w3.y - w1.y;

		float r = 1.0F / (s1 * t2 - s2 * t1);
		XMFLOAT3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
			(t2 * z1 - t1 * z2) * r);
		XMFLOAT3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
			(s1 * z2 - s2 * z1) * r);

		tan1[i1] = tan1[i1] + sdir;
		tan1[i2] = tan1[i2] + sdir;
		tan1[i3] = tan1[i3] + sdir;

		tan2[i1] = tan2[i1] + tdir;
		tan2[i2] = tan2[i2] + tdir;
		tan2[i3] = tan2[i3] + tdir;

	}

	for (long a = 0; a < vertexCount; a++)
	{
		const XMFLOAT3& n = vertex[a].Normal;
		const XMFLOAT3& t = tan1[a];

		// Gram-Schmidt orthogonalize
		vertex[a].Tangent = Vec3Normalize(t - n * Vec3Dot(n, t));
		vertex[a].BiNormal = Vec3Normalize(Vec3Cross(vertex[a].Normal, vertex[a].Tangent));
		// Calculate handedness
		//tangent[a].w = (Vec3Dot(Vec3Cross(n, t), tan2[a]) < 0.0F) ? -1.0F : 1.0F;
	}

	delete[] tan1;
}
//***************************************************************************************
bool Load(char *filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count, float scale)
{
	ID3D11Buffer *pVertexBuffer = NULL;
	bool firstinit = TRUE;
	int i; //Index variable
	FILE *l_file = fopen(filename, "rb"); //File pointer
	if (l_file == NULL) return FALSE; //Open the file

	vector<submodel> submodels;

	unsigned short l_chunk_id; //Chunk identifier
	unsigned int l_chunk_lenght; //Chunk lenght

	unsigned char l_char; //Char variable
	unsigned short l_qty; //Number of elements in each chunk


	char ready = 0;
	unsigned short l_face_flags; //Flag that stores some face information
	char str[1000];
	while (ftell(l_file) < filelength(fileno(l_file))) //Loop to scan the whole file 
	{
		//getche(); //Insert this command for debug (to wait for keypress for each chuck reading)

		fread(&l_chunk_id, 2, 1, l_file); //Read the chunk header
		printf("ChunkID: %x\n", l_chunk_id);
		fread(&l_chunk_lenght, 4, 1, l_file); //Read the lenght of the chunk
		printf("ChunkLenght: %x\n", l_chunk_lenght);

		switch (l_chunk_id)
		{
		case 0x0002://version
		{
			int ver;
			fread(&ver, sizeof(int), 1, l_file); //Read the lenght of the chunk
			int z;
			z = 0;
		}
		break;
		case 0x3d3e://meshversion
		{
			int ver;
			fread(&ver, sizeof(int), 1, l_file); //Read the lenght of the chunk
			int z;
			z = 0;
		}
		break;
		case 0x0100://?????????????
		{
			int ver;
			fread(&ver, sizeof(int), 1, l_file); //Read the lenght of the chunk
			int z;
			z = 0;
		}
		break;
		//----------------- 1group -----------------
		// Description: bitmap
		// Chunk ID: a300
		// Chunk Lenght: 0 + sub chunks
		//-------------------------------------------
		case 0xa300:
		{
			fscanf(l_file, "%s", str);
		}
		break;
		//----------------- MAIN3DS -----------------
		// Description: Main chunk, contains all the other chunks
		// Chunk ID: 4d4d 
		// Chunk Lenght: 0 + sub chunks
		//-------------------------------------------
		case 0x4d4d:
		{
			int z = 0;
		}
		break;

		//----------------- EDIT3DS -----------------
		// Description: 3D Editor chunk, objects layout info 
		// Chunk ID: 3d3d (hex)
		// Chunk Lenght: 0 + sub chunks
		//-------------------------------------------
		case 0x3d3d:
			break;

			//--------------- EDIT_OBJECT ---------------
			// Description: Object block, info for each object
			// Chunk ID: 4000 (hex)
			// Chunk Lenght: len(object name) + sub chunks
			//-------------------------------------------
		case 0x4000:

		{
			submodel subm;
			submodels.push_back(subm);

		}
		ready++;
		i = 0;
		do
		{
			fread(&l_char, 1, 1, l_file);
			//p_object->name[i] = l_char;
			i++;
		} while (l_char != '\0' && i < 20);

		//name.settext(p_object->name);
		firstinit = FALSE;
		break;
		break;

		//--------------- OBJ_TRIMESH ---------------
		// Description: Triangular mesh, contains chunks for 3d mesh info
		// Chunk ID: 4100 (hex)
		// Chunk Lenght: 0 + sub chunks
		//-------------------------------------------
		case 0x4100:
		{
			int i = 0;
			i = 9;
		}
		break;

		//--------------- TRI_VERTEXL ---------------
		// Description: Vertices list
		// Chunk ID: 4110 (hex)
		// Chunk Lenght: 1 x unsigned short (number of vertices) 
		//             + 3 x float (vertex coordinates) x (number of vertices)
		//             + sub chunks
		//-------------------------------------------
		case 0x4110:
		{

			int elem_no = submodels.size() - 1;


			fread(&l_qty, sizeof(unsigned short), 1, l_file);
			submodels[elem_no].vanz = l_qty;
			submodels[elem_no].vertices = new SimpleVertex[submodels[elem_no].vanz];
			for (int ii = 0; ii < submodels[elem_no].vanz; ii++)
			{
				fread(&submodels[elem_no].vertices[ii].Pos.x, sizeof(float), 1, l_file);
				fread(&submodels[elem_no].vertices[ii].Pos.y, sizeof(float), 1, l_file);
				fread(&submodels[elem_no].vertices[ii].Pos.z, sizeof(float), 1, l_file);
			}
		}
		break;
		case 0x4160://local koordinate system
		{
			char ver[100];
			fread(&ver, sizeof(BYTE) * 48, 1, l_file); //Read the lenght of the chunk
			int z;
			z = 0;
		}
		break;
		//--------------- TRI_FACEL1 ----------------
		// Description: Polygons (faces) list
		// Chunk ID: 4120 (hex)
		// Chunk Lenght: 1 x unsigned short (number of polygons) 
		//             + 3 x unsigned short (polygon points) x (number of polygons)
		//             + sub chunks
		//-------------------------------------------
		case 0x4120:
		{
			int elem_no = submodels.size() - 1;




			fread(&l_qty, sizeof(unsigned short), 1, l_file);
			submodels[elem_no].ianz = l_qty * 3;
			submodels[elem_no].indizes = new USHORT[submodels[elem_no].ianz];
			for (int ii = 0; ii < l_qty; ii++)
			{
				fread(&submodels[elem_no].indizes[ii * 3], sizeof(USHORT), 1, l_file);
				fread(&submodels[elem_no].indizes[ii * 3 + 1], sizeof(USHORT), 1, l_file);
				fread(&submodels[elem_no].indizes[ii * 3 + 2], sizeof(USHORT), 1, l_file);
				fread(&l_face_flags, sizeof(USHORT), 1, l_file);
			}
		}
		break;

		//------------- TRI_MAPPINGCOORS ------------
		// Description: Vertices list
		// Chunk ID: 4140 (hex)
		// Chunk Lenght: 1 x unsigned short (number of mapping points) 
		//             + 2 x float (mapping coordinates) x (number of mapping points)
		//             + sub chunks
		//-------------------------------------------
		case 0x4140:
		{
			int elem_no = submodels.size() - 1;

			fread(&l_qty, sizeof(unsigned short), 1, l_file);
			for (int ii = 0; ii < submodels[elem_no].vanz; ii++)
			{
				fread(&submodels[elem_no].vertices[ii].Tex.x, sizeof(float), 1, l_file);
				fread(&submodels[elem_no].vertices[ii].Tex.y, sizeof(float), 1, l_file);
			}
		}
		break;

		//----------- Skip unknow chunks ------------
		//We need to skip all the chunks that currently we don't use
		//We use the chunk lenght information to set the file pointer
		//to the same level next chunk
		//-------------------------------------------
		default:
			fseek(l_file, l_chunk_lenght - 6, SEEK_CUR);
		}


	}

	int vertex_anz = 0;
	for (int ii = 0; ii < submodels.size(); ii++)
		vertex_anz += submodels[ii].ianz;

	SimpleVertex *noIndexVer = new SimpleVertex[vertex_anz];

	int vv = 0;
	for (int uu = 0; uu < submodels.size(); uu++)
		for (int ii = 0; ii < submodels[uu].ianz; ii++)//weil 1.dreieck nix data.... anz--;oder eben ii=1 und net 0
			noIndexVer[vv++] = submodels[uu].vertices[submodels[uu].indizes[ii]];
	*vertex_count = vertex_anz;

	//scale
	for (int ii = 0; ii < vertex_anz; ii ++)
	{
		noIndexVer[ii + 1].Pos.x *= scale;
		noIndexVer[ii + 1].Pos.y *= scale;
		noIndexVer[ii + 1].Pos.z *= scale;
	}

	//perform flat light:
	for (int ii = 0; ii < vertex_anz; ii += 3)
	{
		XMFLOAT3 a, b, c;
		a = noIndexVer[ii + 1].Pos - noIndexVer[ii + 0].Pos;
		b = noIndexVer[ii + 2].Pos - noIndexVer[ii + 0].Pos;
		c = Vec3Cross(a, b);
		c = Vec3Normalize(c);

		noIndexVer[ii + 0].Normal = c;
		noIndexVer[ii + 1].Normal = c;
		noIndexVer[ii + 2].Normal = c;
	}

	//the insanity of performing gouraud lighting:
	//first, lets have a list with flags what we already have visited:
/*	int *flags = new int[vertex_anz];
	for (int ii = 0; ii < vertex_anz; ii++)
		flags[ii] = 0;//not vistied

					  //lets store all similar normal vector addresses here:
	for (int ii = 0; ii < vertex_anz; ii++)
	{
		if (flags[ii] == 1)continue;
		vector<XMFLOAT3*> normals;
		//check all the others, if not already checked:
		for (int uu = ii + 1; uu < vertex_anz; uu++)
		{
			if (flags[uu] == 1)continue;
			if (similar_pos(noIndexVer[ii].Pos, noIndexVer[uu].Pos, 0.01))
				normals.push_back(&noIndexVer[uu].Normal);
		}
		//and now, lets do the average of all of them:
		XMFLOAT3 average_norm = noIndexVer[ii].Normal;
		for (int uu = 0; uu < normals.size(); uu++)
			average_norm = average_norm + *(normals[uu]);
		average_norm = Vec3Normalize(average_norm);
		//re aply:
		noIndexVer[ii].Normal = average_norm;
		for (int uu = 0; uu < normals.size(); uu++)
			*normals[uu] = average_norm;
	}*/

	//calculate now the Tangent Space. For that, calculate the T vector (along texture x)
	CalculateTangentArray(vertex_anz, noIndexVer);


	//initialize d3dx verexbuff:
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * vertex_anz;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = noIndexVer;
	HRESULT hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &pVertexBuffer);
	if (FAILED(hr))
		return FALSE;
	*ppVertexBuffer = pVertexBuffer;



	return TRUE;
}
//***************************************************************
float Vec3Length(const XMFLOAT3 &v)
{
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}
float Vec3Dot(XMFLOAT3 a, XMFLOAT3 b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

XMFLOAT3 Vec3Cross(XMFLOAT3 a, XMFLOAT3 b)
{
	XMFLOAT3 c = XMFLOAT3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
	return c;
}

XMFLOAT3 Vec3Normalize(const  XMFLOAT3 &a)
{
	XMFLOAT3 c = a;
	float len = Vec3Length(a);
	c.x /= len;
	c.y /= len;
	c.z /= len;
	return c;
}

XMFLOAT3 operator+(const XMFLOAT3 lhs, const XMFLOAT3 rhs)
{
	XMFLOAT3 c = XMFLOAT3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
	return c;
}
XMFLOAT3 operator*(const XMFLOAT3 lhs, const float rhs)
{
	XMFLOAT3 c = XMFLOAT3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
	return c;
}
XMFLOAT3 operator-(const XMFLOAT3 lhs, const XMFLOAT3 rhs)
{
	XMFLOAT3 c = XMFLOAT3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
	return c;
}


bool LoadOBJ(char * filename, ID3D11Device * g_pd3dDevice, ID3D11Buffer ** ppVertexBuffer, int * vertex_count)
{
	ID3D11Buffer *pVertexBuffer = NULL;
	ifstream file(filename);
	if (file.fail())
	{
		return false;
	}
	struct Vertex
	{
		float x, y, z;
	}v;
	struct Face
	{
		int vIndex1, vIndex2, vIndex3;
		int tIndex1, tIndex2, tIndex3;
		int nIndex1, nIndex2, nIndex3;
	}f;

	std::vector<Vertex> vertices, texcoords, normals;
	std::vector<Face> faces;
	char l_char;

	file.get(l_char);
	while (!file.eof()) //Loop to scan the whole file 
	{
		switch (l_char)
		{
		case 'v':
		{
			file.get(l_char);
			switch (l_char)
			{
			case ' ':
			{
				file >> v.x >> v.y >> v.z;
				v.z = v.z * -1.0f;
				vertices.push_back(v);
				break;
			}
			case 't':
			{
				file >> v.x >> v.y;
				v.y = 1.0f - v.y;
				texcoords.push_back(v);
				break;
			}
			case 'n':
			{
				file >> v.x >> v.y >> v.z;
				v.z = v.z * -1.0f;
				normals.push_back(v);
				break;
			}
			}
			break;
		}
		case 'f':
		{
			file.get(l_char);
			if (l_char == ' ')
			{
				file >> f.vIndex3 >> l_char >> f.tIndex3 >> l_char >> f.nIndex3
					>> f.vIndex2 >> l_char >> f.tIndex2 >> l_char >> f.nIndex2
					>> f.vIndex1 >> l_char >> f.tIndex1 >> l_char >> f.nIndex1;
				faces.push_back(f);
			}
			break;
		}
		default:
		{
			while (l_char != '\n')
			{
				file.get(l_char);
			}
			file.get(l_char);
		}
		}
	}
	file.close();
	int faceSize = faces.size();
	*vertex_count = 3 * faceSize;

	int vIndex, tIndex, nIndex;
	SimpleVertex* Vertices;
	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;
	HRESULT result;
	int i;
	// Create the vertex array.
	Vertices = new SimpleVertex[*vertex_count];
	if (!Vertices)
	{
		return false;
	}
	for (int i = 0; i < faceSize; i++)
	{
		vIndex = faces[i].vIndex1 - 1;
		tIndex = faces[i].tIndex1 - 1;
		nIndex = faces[i].nIndex1 - 1;

		Vertices[3 * i].Pos.x = vertices[vIndex].x;
		Vertices[3 * i].Pos.y = vertices[vIndex].y;
		Vertices[3 * i].Pos.z = vertices[vIndex].z;
		Vertices[3 * i].Tex.x = texcoords[tIndex].x;
		Vertices[3 * i].Tex.y = texcoords[tIndex].y;
		Vertices[3 * i].Normal.x = normals[nIndex].x;
		Vertices[3 * i].Normal.y = normals[nIndex].y;
		Vertices[3 * i].Normal.z = normals[nIndex].z;

		vIndex = faces[i].vIndex2 - 1;
		tIndex = faces[i].tIndex2 - 1;
		nIndex = faces[i].nIndex2 - 1;

		Vertices[3 * i + 1].Pos.x = vertices[vIndex].x;
		Vertices[3 * i + 1].Pos.y = vertices[vIndex].y;
		Vertices[3 * i + 1].Pos.z = vertices[vIndex].z;
		Vertices[3 * i + 1].Tex.x = texcoords[tIndex].x;
		Vertices[3 * i + 1].Tex.y = texcoords[tIndex].y;
		Vertices[3 * i + 1].Normal.x = normals[nIndex].x;
		Vertices[3 * i + 1].Normal.y = normals[nIndex].y;
		Vertices[3 * i + 1].Normal.z = normals[nIndex].z;

		vIndex = faces[i].vIndex3 - 1;
		tIndex = faces[i].tIndex3 - 1;
		nIndex = faces[i].nIndex3 - 1;

		Vertices[3 * i + 2].Pos.x = vertices[vIndex].x;
		Vertices[3 * i + 2].Pos.y = vertices[vIndex].y;
		Vertices[3 * i + 2].Pos.z = vertices[vIndex].z;
		Vertices[3 * i + 2].Tex.x = texcoords[tIndex].x;
		Vertices[3 * i + 2].Tex.y = texcoords[tIndex].y;
		Vertices[3 * i + 2].Normal.x = normals[nIndex].x;
		Vertices[3 * i + 2].Normal.y = normals[nIndex].y;
		Vertices[3 * i + 2].Normal.z = normals[nIndex].z;
	}
	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(SimpleVertex) * (*vertex_count);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = Vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = g_pd3dDevice->CreateBuffer(&vertexBufferDesc, &vertexData, ppVertexBuffer);
	if (FAILED(result))
	{
		return false;
	}






	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] Vertices;
	Vertices = 0;
	return true;
}
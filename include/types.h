#pragma once

#include <binary-io/data-stream.h>

#include <QIODevice>

#include <cstdint>

typedef float float32_t;
typedef uint64_t CgsID;

class Vector3
{
public:
	Vector3()
	{
		
	}

	Vector3(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	void readFPU(DataStream& file)
	{
		file >> x;
		file >> y;
		file >> z;
	}

	void readVPU(DataStream& file)
	{
		file >> x;
		file >> y;
		file >> z;
		file.skip(0x4);
	}

	float getX() { return x; }
	float getY() { return y; }
	float getZ() { return z; }

private:
	float x = 0;
	float y = 0;
	float z = 0;
};

class Vector4
{
public:
	Vector4()
	{

	}

	Vector4(float x, float y, float z, float w)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	void read(DataStream& file)
	{
		file >> x;
		file >> y;
		file >> z;
		file >> w;
	}

	float getX() { return x; }
	float getY() { return y; }
	float getZ() { return z; }
	float getW() { return w; }

private:
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;
};

// In-game districts
// Not needed yet
//enum class District
//{
//	oceanView,
//	westAcres,
//	twinBridges,
//	bigSurfBeach,
//	easternShore,
//	hillsidePass,
//	heartbreakHills,
//	rockridgeCliffs,
//	southBay,
//	parkVale,
//	paradiseWharf,
//	crystalSummit,
//	lonePeaks,
//	sunsetValley,
//	downtown,
//	riverCity,
//	motorCity,
//	waterfront,
//	paradiseKeysBridge,
//	northBeach,
//	midTown,
//	southCoast,
//	perrensPoint
//};

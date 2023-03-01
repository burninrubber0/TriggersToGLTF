#pragma once

#include <binary-io/data-stream.h>

#include <QIODevice>

#include <cstdint>

typedef float float32_t;
typedef uint64_t CgsID;

class Vector3
{
public:
	Vector3(bool isVpu = false);
	Vector3(float x, float y, float z, bool vpu = false);

	void read(DataStream& stream);

	float getX() { return x; }
	float getY() { return y; }
	float getZ() { return z; }
	bool getIsVpu() { return isVpu; }

private:
	float x = 0;
	float y = 0;
	float z = 0;
	bool isVpu = false;
};

class Vector4
{
public:
	Vector4();
	Vector4(float x, float y, float z, float w);

	void read(DataStream& stream);

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

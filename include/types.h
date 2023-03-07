#pragma once

#include <binary-io/data-stream.h>

#include <QIODevice>

#include <cstdint>

typedef float float32_t;
typedef uint64_t CgsID;

struct Vector3
{
	Vector3(bool isVpu = false);
	Vector3(float x, float y, float z, bool vpu = false);

	void read(DataStream& stream);
	void write(DataStream& stream);

	float x = 0;
	float y = 0;
	float z = 0;

private:
	bool isVpu = false;
};

struct Vector4
{
public:
	Vector4();
	Vector4(float x, float y, float z, float w);

	void read(DataStream& stream);
	void write(DataStream& stream);

	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;
};

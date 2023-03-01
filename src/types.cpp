#include <types.h>

Vector3::Vector3(bool isVpu)
	: isVpu(isVpu)
{

}

Vector3::Vector3(float x, float y, float z, bool isVpu)
	: x(x), y(y), z(z), isVpu(isVpu)
{
	
}

void Vector3::read(DataStream& stream)
{
	stream >> x;
	stream >> y;
	stream >> z;
	if (isVpu)
		stream.skip(0x4);
}

Vector4::Vector4()
{

}

Vector4::Vector4(float x, float y, float z, float w)
	: x(x), y(y), z(z), w(w)
{
	
}

void Vector4::read(DataStream& stream)
{
	stream >> x;
	stream >> y;
	stream >> z;
	stream >> w;
}

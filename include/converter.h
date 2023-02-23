#pragma once

#include <binary-io/data-stream.h>
#include <trigger-data.h>

#include <tiny_gltf.h>

#include <QFile>

#include <string>

class Converter
{
public:
	Converter(int argc, char* argv[]);
	~Converter();

	int result = 0;

private:
	// For file reading
	DataStream inStream;
	std::string inFileName;
	std::string outFileName;

	BrnTrigger::TriggerData* triggerData = nullptr;

	int getArgs(int argc, char* argv[]);
	int checkArgs(int argc, char* argv[]);
	void showUsage();

	void readTriggerData();
	tinygltf::Buffer createGLTFBuffer();
	void writeBoxRegion(DataStream& stream);
	Vector4 EulerToQuatRot(Vector3 euler);
	void convertTriggersToGLTF();
};

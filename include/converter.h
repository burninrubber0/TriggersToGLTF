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

	bool triggerRegionExists(BrnTrigger::TriggerRegion region, bool checkGenericRegions = true);
	void addTriggerRegionFields(BrnTrigger::TriggerRegion region, tinygltf::Value::Object& extras);

	void convertLandmark(BrnTrigger::Landmark landmark, tinygltf::Node& node, int index);
	void convertBlackspot(BrnTrigger::Blackspot blackspot, tinygltf::Node& node, int index);
	void convertVfxBoxRegion(BrnTrigger::VFXBoxRegion vfxBoxRegion, tinygltf::Node& node, int index);
	void convertSignatureStunt(BrnTrigger::SignatureStunt signatureStunt, tinygltf::Node& node, int index);
	void convertKillzone(BrnTrigger::Killzone killzone, tinygltf::Node& node, int index);
	void convertGenericRegion(BrnTrigger::GenericRegion region, tinygltf::Node& node, int index);
	void convertTriggerRegion(BrnTrigger::TriggerRegion triggerRegion, tinygltf::Node& node, int index);

	template <typename T>
	void addBoxRegionTransform(T entry, tinygltf::Node& node)
	{
		node.translation = {
		entry.getBoxRegion().getPosX(),
		entry.getBoxRegion().getPosY(),
		entry.getBoxRegion().getPosZ()
		};
		Vector4 rotation = EulerToQuatRot({
			entry.getBoxRegion().getRotX(),
			entry.getBoxRegion().getRotY(),
			entry.getBoxRegion().getRotZ()
			});
		node.rotation = {
			rotation.getX(),
			rotation.getY(),
			rotation.getZ(),
			rotation.getW()
		};
		node.scale = {
			entry.getBoxRegion().getDimX(),
			entry.getBoxRegion().getDimY(),
			entry.getBoxRegion().getDimZ()
		};
	}
};

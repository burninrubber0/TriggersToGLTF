#pragma once

#include <binary-io/data-stream.h>
#include <trigger-data.h>

#include <tiny_gltf.h>

#include <QFile>

#include <string>

using namespace BrnTrigger;
using namespace tinygltf;

class Converter
{
public:
	Converter(int argc, char* argv[]);
	~Converter();

	int result = 0;

private:
	enum class Platform
	{
		PS3,
		X360,
		PC,
		PS4,
		NX
	} platform = Platform::PC;

	// For file reading
	DataStream inStream;
	std::string inFileName;
	std::string outFileName;
	int8_t typeFilter = -1;
	std::string profileFileName;

	TriggerData* triggerData = nullptr;
	QList<uint64_t> hitTriggerIds;

	const int minArgCount = 3;
	int getArgs(int argc, char* argv[]);
	int checkArgs(int argc, char* argv[]);
	void showUsage();

	void readTriggerData();
	void readProfileTriggers();
	void readStuntElements(DataStream& stream, int offset, int count);
	void readIslandStuntElements(DataStream& stream, int offset, int count);
	Buffer createGLTFBuffer();
	void writeBoxRegion(DataStream& stream);
	Vector4 EulerToQuatRot(Vector3 euler);
	void convertTriggersToGLTF();

	bool triggerRegionExists(TriggerRegion region, bool checkGenericRegions = true);
	void addTriggerRegionFields(TriggerRegion region, Value::Object& extras);

	void convertLandmark(Landmark landmark, Node& node, int index);
	void convertStartingGrid(StartingGrid grid, Node& node, int index);
	void convertBlackspot(Blackspot blackspot, Node& node, int index);
	void convertVfxBoxRegion(VFXBoxRegion vfxBoxRegion, Node& node, int index);
	void convertSignatureStunt(SignatureStunt signatureStunt, Node& node, int index);
	void convertKillzone(Killzone killzone, Node& node, int index);
	void convertGenericRegion(GenericRegion region, Node& node, int index);
	void convertTriggerRegion(TriggerRegion triggerRegion, Node& node, int index);
	void convertRoamingLocation(RoamingLocation location, Node& node, int index);
	void convertSpawnLocation(SpawnLocation location, Node& node, int index);

	template <typename T>
	void addBoxRegionTransform(T entry, Node& node)
	{
		node.translation = {
			entry.boxRegion.positionX,
			entry.boxRegion.positionY,
			entry.boxRegion.positionZ
		};
		Vector4 rotation = EulerToQuatRot({
			entry.boxRegion.rotationX,
			entry.boxRegion.rotationY,
			entry.boxRegion.rotationZ
		});
		node.rotation = {
			rotation.x,
			rotation.y,
			rotation.z,
			rotation.w
		};
		node.scale = {
			entry.boxRegion.dimensionX,
			entry.boxRegion.dimensionY,
			entry.boxRegion.dimensionZ
		};
	}

	void addPointTransform(Vector3 pos, Vector3 rot, Node& node)
	{
		node.translation = {
			pos.x,
			pos.y,
			pos.z
		};
		Vector4 rotation = EulerToQuatRot({
			rot.x,
			rot.y,
			rot.z
		});
		node.rotation = {
			rotation.x,
			rotation.y,
			rotation.z,
			rotation.w
		};
	}

	void addPointTransform(Vector3 pos, Node& node)
	{
		node.translation = {
			pos.x,
			pos.y,
			pos.z
		};
	}
};

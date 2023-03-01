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
	// For file reading
	DataStream inStream;
	std::string inFileName;
	std::string outFileName;

	TriggerData* triggerData = nullptr;

	const int minArgCount = 3;
	int getArgs(int argc, char* argv[]);
	int checkArgs(int argc, char* argv[]);
	void showUsage();

	void readTriggerData();
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

	void addPointTransform(Vector3 pos, Vector3 rot, Node& node)
	{
		node.translation = {
			pos.getX(),
			pos.getY(),
			pos.getZ()
		};
		Vector4 rotation = EulerToQuatRot({
			rot.getX(),
			rot.getY(),
			rot.getZ()
		});
		node.rotation = {
			rotation.getX(),
			rotation.getY(),
			rotation.getZ(),
			rotation.getW()
		};
	}

	void addPointTransform(Vector3 pos, Node& node)
	{
		node.translation = {
			pos.getX(),
			pos.getY(),
			pos.getZ()
		};
	}
};

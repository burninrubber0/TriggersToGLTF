#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <converter.h>

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QScopedPointer>

#include <iostream>

using namespace BrnTrigger;
using namespace tinygltf;

Converter::Converter(int argc, char* argv[])
	: triggerData(new TriggerData)
{
	result = getArgs(argc, argv);
	if (result != 0)
		return;

	readTriggerData();
	if (!profileFileName.empty())
		readProfileTriggers();
	convertTriggersToGLTF();
}

Converter::~Converter()
{
	if (inStream.device() != nullptr)
		delete inStream.device();
	if (triggerData != nullptr)
		delete triggerData;
}

int Converter::getArgs(int argc, char* argv[])
{
	int checkResult = checkArgs(argc, argv);
	if (checkResult != 0)
		return checkResult;

	for (int i = 1; i < argc - 2; ++i)
	{
		// Set big endian and 64 bit based on provided platform
		if (strcmp(argv[i], "-p") == 0)
		{
			QString platform = argv[i + 1];
			if (platform == "PC")
				this->platform = Platform::PC;
			else if (platform == "PS3")
				this->platform = Platform::PS3;
			else if (platform == "PS4")
				this->platform = Platform::PS4;
			else if (platform == "X360")
				this->platform = Platform::X360;
			else if (platform == "NX")
				this->platform = Platform::NX;

			if (this->platform == Platform::PC)
				return 0;
			else if (this->platform == Platform::PS3 || this->platform == Platform::X360)
				inStream.setByteOrder(QDataStream::BigEndian);
			else if (this->platform == Platform::PS4 || this->platform == Platform::NX)
				inStream.setIs64Bit(true);

			i++;
		}
		else if (strcmp(argv[i], "-f") == 0)
		{
			uint8_t filter = atoi(argv[i + 1]);
			if (filter >= 0)
				typeFilter = filter;
			i++;
		}
		else if (strcmp(argv[i], "-s") == 0)
		{
			profileFileName = argv[i + 1];
			i++;
		}
		else
		{
			std::cerr << "Invalid option specified: " << argv[i];
			return 4;
		}
	}

	inStream.setDevice(new QFile(argv[argc - 2]));
	inFileName = argv[argc - 2];
	outFileName = argv[argc - 1];

	return 0;
}

int Converter::checkArgs(int argc, char* argv[])
{
	// Ensure minimum argument count is reached
	if (argc < minArgCount)
	{
		showUsage();
		return 1;
	}

	// Check input file exists
	QFile in(argv[argc - 2]);
	QFileInfo inputInfo(in);
	if (!inputInfo.exists() || !inputInfo.isFile())
	{
		std::cout << "Invalid input file";
		return 2;
	}

	// Check output does not exist as a non-file
	QFile out(argv[argc - 1]);
	QFileInfo outputInfo(out);
	if (outputInfo.exists() && !outputInfo.isFile())
	{
		std::cerr << "Output location exists and is not a file, cannot overwrite";
		return 3;
	}

	return 0;
}

void Converter::showUsage()
{
	std::cout << "Usage: TriggersToGLTF [options] <input file> <output file>\n\n"
		<< "Options:\n"
		<< " -p   File platform. PS3, X360, PC, PS4, or NX. Default: PC\n"
		<< " -f   GenericRegion type filter (an integer number). By default, all are converted.\n"
		<< " -s   Export only triggers not present in the provided savegame.\n"
		<< "      Should be used with filters 8, 9, or 13 (collectibles).";
}

void Converter::readTriggerData()
{
	inStream.open(QIODevice::ReadOnly);
	triggerData->read(inStream);
	inStream.close();
}

void Converter::readProfileTriggers()
{
	DataStream profile;
	profile.setDevice(new QFile(QString::fromStdString(profileFileName)));
	profile.open(QIODevice::ReadOnly);

	// Set offsets based on platform
	int base = 0; // Profile start offset
	if (platform == Platform::X360)
		base = 0x1C;
	else if (platform == Platform::PC)
		base = 0x1D246;
	int stunts = base + 0x75E8; // Stunt elements offset
	const int alloc = 512; // Number of stunt elements allocated per type

	// Get stunt element counts
	int jumpCount = 0;
	int smashCount = 0;
	int billboardCount = 0;
	profile.seek(stunts + alloc * 8);
	profile >> jumpCount;
	profile.seek(stunts + alloc * 8 * 2 + 8);
	profile >> smashCount;
	profile.seek(stunts + alloc * 8 * 3 + 8 * 2);
	profile >> billboardCount;

	// Read triggers
	readStuntElements(profile, stunts, jumpCount); // Jumps
	readStuntElements(profile, stunts + alloc * 8 + 8, smashCount); // Smashes
	readStuntElements(profile, stunts + alloc * 8 * 2 + 8 * 2, smashCount); // Billboards

	// Make sure this is not the original PC game,
	// because that version does not have the island
	if (platform == Platform::PC)
	{
		QFileInfo info(QString::fromStdString(profileFileName));
		if (info.size() == 0x5D246)
		{
			profile.close();
			return;
		}
	}

	// Island offsets
	int bsi = 0;
	switch (platform)
	{
	case Platform::PS3:
		bsi = 0x30648;
		break;
	case Platform::X360:
		bsi = 0x2F4E0;
		break;
	case Platform::PS4:
		bsi = 0x79B78;
		break;
	case Platform::PC:
		bsi = 79040;
		break;
	case Platform::NX:
		bsi = 0x7AE68;
		break;
	}

	// Get stunt element details
	int bsiBillboards = bsi + 0x31C;
	int bsiSmashes = bsi + 0x488;
	int bsiJumps = bsi + 0x6E4;
	const int bsiBillboardAlloc = 45;
	const int bsiSmashAlloc = 75;
	const int bsiJumpAlloc = 15;
	int bsiBillboardCount = 0;
	int bsiSmashCount = 0;
	int bsiJumpCount = 0;
	profile.seek(bsiBillboards + bsiBillboardAlloc * 8);
	profile >> bsiBillboardCount;
	profile.seek(bsiSmashes + bsiSmashAlloc + 8);
	profile >> bsiSmashCount;
	profile.seek(bsiJumps + bsiJumpAlloc * 8);
	profile >> bsiJumpCount;

	// Read island triggers
	readIslandStuntElements(profile, bsiBillboards, bsiBillboardCount); // Island billboards
	readIslandStuntElements(profile, bsiSmashes, bsiSmashCount); // Island smashes
	readIslandStuntElements(profile, bsiJumps, bsiJumpCount); // Island jumps

	profile.close();
}

void Converter::readStuntElements(DataStream& stream, int offset, int count)
{
	uint64_t tmpId = 0;
	stream.seek(offset);
	for (int i = 0; i < count; ++i)
	{
		stream >> tmpId;
		hitTriggerIds.append(tmpId);
	}
}

void Converter::readIslandStuntElements(DataStream& stream, int offset, int count)
{
	uint32_t tmpId = 0;
	stream.seek(offset);
	for (int i = 0; i < count; ++i)
	{
		stream >> tmpId;
		stream.skip(4);
		hitTriggerIds.append((uint64_t)tmpId);
	}
}

// Creates a file with the box regions converted to triangles
// Saved as Matrix 3x3 (MAT3)
Buffer Converter::createGLTFBuffer()
{
	QByteArray binData;
	QBuffer buffer(&binData);
	DataStream dataStream;
	dataStream.setDevice(&buffer);
	dataStream.open(QIODeviceBase::WriteOnly);

	// Write indices to bin data
	// 14 vertices for a box region trigger
	for (int j = 0; j < 14; ++j)
		dataStream << (ushort)(j);

	// Write the vertices to bin data as a triangle strip
	// Position/rotation/dimension is set up in nodes
	writeBoxRegion(dataStream); 

	dataStream.close();

	// Write to buffer
	Buffer gltfBuffer;
	int64_t size = binData.length();
	gltfBuffer.data.resize(size);
	dataStream.open(QIODevice::ReadOnly);
	for (int i = 0; i < size; ++i)
		dataStream >> gltfBuffer.data[i];
	dataStream.close();
	gltfBuffer.name = "Buffer";
	return gltfBuffer;
}

// Writes a 1x1x1 cube to the stream as a triangle strip (14 verts)
// Modified from https://stackoverflow.com/a/70219726
void Converter::writeBoxRegion(DataStream& stream)
{
	stream << 0.5f << 0.5f << -0.5f // Back-top-right
		<< -0.5f << 0.5f << -0.5f // Back-top-left
		<< 0.5f << -0.5f << -0.5f // Back-bottom-right
		<< -0.5f << -0.5f << -0.5f // Back-bottom-left
		<< -0.5f << -0.5f << 0.5f // Front-bottom-left
		<< -0.5f << 0.5f << -0.5f // Back-top-left
		<< -0.5f << 0.5f << 0.5f // Front-top-left
		<< 0.5f << 0.5f << -0.5f // Back-top-right
		<< 0.5f << 0.5f << 0.5f // Front-top-right
		<< 0.5f << -0.5f << -0.5f // Back-bottom-right
		<< 0.5f << -0.5f << 0.5f // Front-bottom-right
		<< -0.5f << -0.5f << 0.5f // Front-bottom-left
		<< 0.5f << 0.5f << 0.5f // Front-top-right
		<< -0.5f << 0.5f << 0.5f; // Front-top-left
}

// Modified from https://stackoverflow.com/a/70462919
Vector4 Converter::EulerToQuatRot(Vector3 euler)
{
	float cy = (float)qCos(euler.z * 0.5);
	float sy = (float)qSin(euler.z * 0.5);
	float cp = (float)qCos(euler.y * 0.5);
	float sp = (float)qSin(euler.y * 0.5);
	float cr = (float)qCos(euler.x * 0.5);
	float sr = (float)qSin(euler.x * 0.5);

	return Vector4(
		sr * cp * cy + cr * sp * sy,
		cr * sp * cy - sr * cp * sy,
		cr * cp * sy + sr * sp * cy,
		cr * cp * cy - sr * sp * sy
	);
}

void Converter::convertTriggersToGLTF()
{
	// GLTF object
	QScopedPointer<Model> model(new Model());

	// Create a default scene
	model->scenes.push_back(Scene());
	model->scenes[0].name = "Scene";
	model->defaultScene = 0;

	// Create a buffer from the trigger data
	model->buffers.push_back(createGLTFBuffer());

	// Create buffer views
	// 0 = indices, 1 = vertices
	for (int i = 0; i < 2; ++i)
	{
		model->bufferViews.push_back(BufferView());
		model->bufferViews[i].buffer = 0;
	}
	model->bufferViews[0].byteLength = 14 * sizeof(ushort);
	model->bufferViews[1].byteLength = 14 * (sizeof(float) * 3);
	model->bufferViews[0].byteOffset = 0;
	model->bufferViews[1].byteOffset = model->bufferViews[0].byteLength;
	model->bufferViews[1].byteStride = sizeof(float) * 3;
	model->bufferViews[0].target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
	model->bufferViews[1].target = TINYGLTF_TARGET_ARRAY_BUFFER;
	model->bufferViews[0].name = "Indices buffer view";
	model->bufferViews[1].name = "Vertices buffer view";

	// Create accessors
	// 0 = indices, 1 = vertices
	for (int i = 0; i < 2; ++i)
	{
		model->accessors.push_back(Accessor());
		model->accessors[i].bufferView = i;
		model->accessors[i].byteOffset = 0;
		model->accessors[i].count = 14;
	}
	model->accessors[0].componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
	model->accessors[1].componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	model->accessors[0].type = TINYGLTF_TYPE_SCALAR;
	model->accessors[1].type = TINYGLTF_TYPE_VEC3;
	model->accessors[1].minValues = { -0.5, -0.5, -0.5 };
	model->accessors[1].maxValues = { 0.5, 0.5, 0.5 };
	model->accessors[0].name = "Indices accessor";
	model->accessors[1].name = "Vertices accessor";

	// Create mesh
	model->meshes.push_back(Mesh());
	model->meshes[0].primitives.push_back(Primitive());
	model->meshes[0].primitives[0].mode = TINYGLTF_MODE_TRIANGLE_STRIP;
	model->meshes[0].primitives[0].indices = 0;
	model->meshes[0].primitives[0].attributes["POSITION"] = 1;
	model->meshes[0].name = "Mesh";

	// Create nodes
	// TriggerRegion derived nodes
	int currentNodeCount = 0;
	
	// Remaining GenericRegion nodes
	int genericRegionNodeIndex = currentNodeCount;
	int currentGenericRegionNode = 0;
	for (int i = 0; i < triggerData->genericRegionCount; ++i)
	{
		if ((uint8_t)triggerData->genericRegions[i].type == typeFilter)
		{
			// Skip gathered collectibles
			if (hitTriggerIds.contains((uint64_t)triggerData->genericRegions[i].id)
				|| hitTriggerIds.contains((uint64_t)triggerData->genericRegions[i].groupId))
				continue;

			model->nodes.push_back(Node());
			model->nodes.back().mesh = 0;
			convertGenericRegion(triggerData->genericRegions[i], model->nodes[genericRegionNodeIndex + currentGenericRegionNode], i);
			model->scenes[0].nodes.push_back(genericRegionNodeIndex + currentGenericRegionNode);
			currentGenericRegionNode++;
			currentNodeCount++;
		}
	}

	// Set up asset
	model->asset.version = "2.0";
	model->asset.generator = "tinygltf";
	
	// Save it to a file
	TinyGLTF gltf;
	gltf.WriteGltfSceneToFile(model.data(), outFileName,
		true, // embedImages
		true, // embedBuffers
		true, // pretty print
		false); // write binary
}

bool Converter::triggerRegionExists(TriggerRegion region, bool checkGenericRegions)
{
	int32_t id = region.id;
	for (int i = 0; i < triggerData->landmarkCount; ++i)
	{
		if (id == triggerData->landmarks[i].id)
			return true;
	}
	for (int i = 0; i < triggerData->blackspotCount; ++i)
	{
		if (id == triggerData->blackspots[i].id)
			return true;
	}
	for (int i = 0; i < triggerData->vfxBoxRegionCount; ++i)
	{
		if (id == triggerData->vfxBoxRegions[i].id)
			return true;
	}
	if (checkGenericRegions)
	{
		for (int i = 0; i < triggerData->genericRegionCount; ++i)
		{
			if (id == triggerData->genericRegions[i].id)
				return true;
		}
		return false;
	}
	for (int i = 0; i < triggerData->signatureStuntCount; ++i)
	{
		for (int j = 0; j < triggerData->signatureStunts[i].stuntElementCount; ++j)
		{
			if (id == triggerData->signatureStunts[i].getStuntElement(j).id)
				return true;
		}
	}
	for (int i = 0; i < triggerData->killzoneCount; ++i)
	{
		for (int j = 0; j < triggerData->killzones[i].triggerCount; ++j)
		{
			if (id == triggerData->killzones[i].getTrigger(j).id)
				return true;
		}
	}
	return false;
}

void Converter::addTriggerRegionFields(TriggerRegion region, Value::Object& extras)
{
	extras["TriggerRegion ID"] = Value(region.id);
	extras["TriggerRegion region index"] = Value(region.regionIndex);
	extras["TriggerRegion type"] = Value((uint8_t)region.type);
	extras["TriggerRegion unknown 0"] = Value(region.unk0);
}

void Converter::convertLandmark(Landmark landmark, Node& node, int index)
{
	addBoxRegionTransform(landmark, node);

	Value::Object extras;
	addTriggerRegionFields(landmark, extras);
	extras["Design index"] = Value(landmark.designIndex);
	extras["District"] = Value(landmark.district);
	extras["Is online"] = Value((bool)(((uint8_t)landmark.flags & (uint8_t)Landmark::Flags::isOnline) != 0));
	node.extras = Value(extras);

	node.name = "Landmark " + std::to_string(index) + " (" + std::to_string(landmark.id) + ")";
}

void Converter::convertStartingGrid(StartingGrid grid, Node& node, int index)
{
	for (int i = 0; i < 8; ++i)
		addPointTransform(grid.startingPositions[i], grid.startingDirections[i], node);
}

void Converter::convertBlackspot(Blackspot blackspot, Node& node, int index)
{
	addBoxRegionTransform(blackspot, node);

	Value::Object extras;
	addTriggerRegionFields(blackspot, extras);
	extras["Score type"] = Value((uint8_t)blackspot.scoreType);
	extras["Score amount"] = Value(blackspot.scoreAmount);
	node.extras = Value(extras);

	node.name = "Blackspot " + std::to_string(index) + " (" + std::to_string(blackspot.id) + ")";
}

void Converter::convertVfxBoxRegion(VFXBoxRegion vfxBoxRegion, Node& node, int index)
{
	addBoxRegionTransform(vfxBoxRegion, node);

	node.name = "VFXBoxRegion " + std::to_string(index) + " (" + std::to_string(vfxBoxRegion.id) + ")";
}

void Converter::convertSignatureStunt(SignatureStunt signatureStunt, Node& node, int index)
{
	Value::Object extras;
	extras["ID"] = Value((int)signatureStunt.id);
	extras["Camera"] = Value((int)signatureStunt.camera);
	node.extras = Value(extras);

	node.name = "SignatureStunt " + std::to_string(index) + " (" + std::to_string(signatureStunt.id) + ")";
}

void Converter::convertKillzone(Killzone killzone, Node& node, int index)
{
	Value::Array regionIds;
	for (int i = 0; i < killzone.regionIdCount; ++i)
		regionIds.push_back(Value((int)killzone.regionIds[i]));

	Value::Object extras;
	extras["Region IDs"] = Value(regionIds);
	node.extras = Value(extras);

	node.name = "Killzone " + std::to_string(index);
}

void Converter::convertGenericRegion(GenericRegion region, Node& node, int index)
{
	addBoxRegionTransform(region, node);

	Value::Object extras;
	addTriggerRegionFields(region, extras);
	extras["Group ID"] = Value(region.groupId);
	extras["Camera cut 1"] = Value(region.cameraCut1);
	extras["Camera cut 2"] = Value(region.cameraCut2);
	extras["Camera type 1"] = Value((int16_t)region.cameraType1);
	extras["Camera type 2"] = Value((int16_t)region.cameraType2);
	extras["Type"] = Value((uint8_t)region.type);
	extras["Is one way"] = Value((bool)region.isOneWay);
	node.extras = Value(extras);

	uint64_t id = region.groupId;
	if (id == 0)
		id = region.id;
	node.name = "GenericRegion " + std::to_string(index) + " (" + std::to_string(id) + ")";
}

void Converter::convertTriggerRegion(TriggerRegion triggerRegion, Node& node, int index)
{
	addBoxRegionTransform(triggerRegion, node);

	node.name = "TriggerRegion " + std::to_string(index) + " (" + std::to_string(triggerRegion.id) + ")";
}

void Converter::convertRoamingLocation(RoamingLocation location, Node& node, int index)
{
	addPointTransform(location.position, node);

	Value::Object extras;
	extras["District index"] = Value(location.districtIndex);
	node.extras = Value(extras);

	node.name = "RoamingLocation " + std::to_string(index);
}

void Converter::convertSpawnLocation(SpawnLocation location, Node& node, int index)
{
	addPointTransform(location.position, location.direction, node);

	Value::Object extras;
	extras["Junkyard ID"] = Value((int)location.junkyardId);
	extras["Type"] = Value((uint8_t)location.type);
	node.extras = Value(extras);

	node.name = "SpawnLocation " + std::to_string(index);
}

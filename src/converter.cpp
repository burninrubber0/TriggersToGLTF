#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <converter.h>

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QScopedPointer>

#include <iostream>

Converter::Converter(int argc, char* argv[])
	: triggerData(new BrnTrigger::TriggerData)
{
	result = getArgs(argc, argv);
	if (result != 0)
		return;

	readTriggerData();
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
				return 0;
			else if (platform == "PS3" || platform == "X360")
				inStream.setByteOrder(QDataStream::BigEndian);
			else if (platform == "PS4" || platform == "NX")
				inStream.setIs64Bit(true);
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
	if (argc < 3)
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
	std::cout << "Usage: TriggersToGLTF [-p PLATFORM] <input triggers> <output glTF>\n\n"
		<< "Options:\n -p   File platform. PS3, X360, PC, PS4, or NX. Default: PC";
}

void Converter::readTriggerData()
{
	inStream.open(QIODevice::ReadOnly);
	triggerData->read(inStream);
	inStream.close();
}

// --------------------
// GLTF stuff
// --------------------

using namespace tinygltf;

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
	float cy = (float)qCos(euler.getZ() * 0.5);
	float sy = (float)qSin(euler.getZ() * 0.5);
	float cp = (float)qCos(euler.getY() * 0.5);
	float sp = (float)qSin(euler.getY() * 0.5);
	float cr = (float)qCos(euler.getX() * 0.5);
	float sr = (float)qSin(euler.getX() * 0.5);

	return Vector4(
		sr * cp * cy - cr * sp * sy,
		cr * sp * cy + sr * cp * sy,
		cr * cp * sy - sr * sp * cy,
		cr * cp * cy + sr * sp * sy
	);
}

void Converter::convertTriggersToGLTF()
{
	// Get box region counts
	//
	//int stuntCount = triggerData->getSignatureStuntCount();
	//QList<int> stuntRegionCounts;
	//for (int i = 0; i < stuntCount; ++i)
	//	stuntRegionCounts.append(triggerData->getSignatureStunt(i).getStuntElementCount());
	//int genericRegionCount = triggerData->getGenericRegionCount();
	//int killzoneCount = triggerData->getKillzoneCount();
	//QList<int> killzoneRegionCounts;
	//for (int i = 0; i < killzoneCount; ++i)
	//	killzoneRegionCounts.append(triggerData->getKillzone(i).getTriggerCount());
	//int blackspotCount = triggerData->getBlackspotCount();
	//int vfxBoxRegionCount = triggerData->getVfxBoxRegionCount();
	int regionCount = triggerData->getRegionCount();
	//int totalRegionCount = triggerData->getTotalRegionCount();
	//
	//// Non box region counts
	//QList<int> startingGridCounts;
	//for (int i = 0; i < landmarkCount; ++i)
	//	triggerData->getLandmark(i).getStartingGridCount();
	//int roamingLocationCount = triggerData->getRoamingLocationCount();
	//int spawnLocationCount = triggerData->getSpawnLocationCount();

	// GLTF object
	QScopedPointer<Model> pm(new Model());

	// Create a default scene
	pm->scenes.push_back(Scene());
	pm->scenes[0].name = "Scene";
	pm->defaultScene = 0;

	// Create a buffer from the trigger data
	pm->buffers.push_back(createGLTFBuffer());

	// Create buffer views
	// 0 = indices, 1 = vertices
	for (int i = 0; i < 2; ++i)
	{
		pm->bufferViews.push_back(BufferView());
		pm->bufferViews[i].buffer = 0;
	}
	pm->bufferViews[0].byteLength = 14 * sizeof(ushort);
	pm->bufferViews[1].byteLength = 14 * (sizeof(float) * 3);
	pm->bufferViews[0].byteOffset = 0;
	pm->bufferViews[1].byteOffset = pm->bufferViews[0].byteLength;
	pm->bufferViews[1].byteStride = sizeof(float) * 3;
	pm->bufferViews[0].target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
	pm->bufferViews[1].target = TINYGLTF_TARGET_ARRAY_BUFFER;
	pm->bufferViews[0].name = "Indices buffer view";
	pm->bufferViews[1].name = "Vertices buffer view";

	// Create accessors
	// 0 = indices, 1 = vertices
	for (int i = 0; i < 2; ++i)
	{
		pm->accessors.push_back(Accessor());
		pm->accessors[i].bufferView = i;
		pm->accessors[i].byteOffset = 0;
		pm->accessors[i].count = 14;
	}
	pm->accessors[0].componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
	pm->accessors[1].componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	pm->accessors[0].type = TINYGLTF_TYPE_SCALAR;
	pm->accessors[1].type = TINYGLTF_TYPE_VEC3;
	pm->accessors[1].minValues = { -0.5, -0.5, -0.5 };
	pm->accessors[1].maxValues = { 0.5, 0.5, 0.5 };
	pm->accessors[0].name = "Indices accessor";
	pm->accessors[1].name = "Vertices accessor";

	// Create mesh
	pm->meshes.push_back(Mesh());
	pm->meshes[0].primitives.push_back(Primitive());
	pm->meshes[0].primitives[0].mode = TINYGLTF_MODE_TRIANGLE_STRIP;
	pm->meshes[0].primitives[0].indices = 0;
	pm->meshes[0].primitives[0].attributes["POSITION"] = 1;
	pm->meshes[0].name = "Mesh";

	// Create nodes
	for (int i = 0; i < regionCount; ++i)
	{
		pm->nodes.push_back(Node());
		pm->nodes[i].mesh = 0;
	}

	// TODO: Separate functions to have nodes for:
	// Landmark, Blackspot, VFXBoxRegion (all TriggerRegion derived)
	// SignatureStunt, Killzone (all containing GenericRegion arrays)
	// Remaining GenericRegion triggers (checked against SignatureStunts and Killzones to avoid duplication)
	// Remaining TriggerRegion triggers (checked against all of the above)
	// Add starting grids, roaming locations, and spawn locations afterward

	// Get counts
	int landmarkCount = triggerData->getLandmarkCount();
	int genericRegionCount = triggerData->getGenericRegionCount();
	
	// Update nodes
	for (int i = 0; i < landmarkCount; ++i) // Landmark nodes
		convertLandmark(triggerData->getLandmark(i), pm->nodes[i], i);
	for (int i = 0; i < genericRegionCount; ++i) // Generic region nodes
		convertGenericRegion(triggerData->getGenericRegion(i), pm->nodes[i + landmarkCount], i);

	// Add node indices to scene
	for (int i = 0; i < regionCount; ++i)
		pm->scenes[0].nodes.push_back(i);

	// Set up asset
	pm->asset.version = "2.0";
	pm->asset.generator = "tinygltf";
	
	// Save it to a file
	tinygltf::TinyGLTF gltf;
	gltf.WriteGltfSceneToFile(pm.data(), outFileName,
		true, // embedImages
		true, // embedBuffers
		true, // pretty print
		false); // write binary
}

void Converter::convertLandmark(BrnTrigger::Landmark landmark, Node& node, int index)
{
	addBoxRegionTransform(landmark, node);

	Value::Object extras;

	// TriggerRegion fields
	extras["TriggerRegion ID"] = Value(landmark.getId());
	extras["TriggerRegion region index"] = Value(landmark.getRegionIndex());
	extras["TriggerRegion type"] = Value((uint8_t)landmark.getType());
	extras["TriggerRegion unknown 0"] = Value(landmark.getUnk0());

	// Landmark fields
	extras["Landmark design index"] = Value(landmark.getDesignIndex());
	extras["Landmark district"] = Value(landmark.getDistrict());
	extras["Landmark is online"] = Value((bool)landmark.getFlags() & 1);

	node.extras = Value(extras);

	// Node name
	uint64_t id = landmark.getId();
	node.name = "Landmark " + std::to_string(index) + " (" + std::to_string(id) + ")";
}

void Converter::convertGenericRegion(BrnTrigger::GenericRegion region, tinygltf::Node& node, int index)
{
	addBoxRegionTransform(region, node);

	Value::Object extras;

	// TriggerRegion fields
	extras["TriggerRegion ID"] = Value(region.getId());
	extras["TriggerRegion region index"] = Value(region.getRegionIndex());
	extras["TriggerRegion type"] = Value((uint8_t)region.TriggerRegion::getType());
	extras["TriggerRegion unknown 0"] = Value(region.getUnk0());

	// GenericRegion fields
	extras["GenericRegion group ID"] = Value(region.getGroupId());
	extras["GenericRegion camera cut 1"] = Value(region.getCameraCut1());
	extras["GenericRegion camera cut 2"] = Value(region.getCameraCut2());
	extras["GenericRegion camera type 1"] = Value((int16_t)region.getcameraType1());
	extras["GenericRegion camera type 2"] = Value((int16_t)region.getcameraType2());
	extras["GenericRegion type"] = Value((uint8_t)region.getType());
	extras["GenericRegion is one way"] = Value((bool)region.getIsOneWay());

	node.extras = Value(extras);

	// Node name
	uint64_t id = region.getGroupId();
	if (id == 0)
		id = region.getId();
	node.name = "GenericRegion " + std::to_string(index) + " (" + std::to_string(id) + ")";
}

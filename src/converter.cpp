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
	// 14 vertices per box trigger
	for (int i = 0; i < triggerData->getRegionCount(); ++i)
	{
		for (int j = 0; j < 14; ++j)
			dataStream << (ushort)(j);
	}

	// Write the vertices to bin data as triangle strips
	// Position/rotation/dimension is set up in nodes, these are all the same
	for (int i = 0; i < triggerData->getRegionCount(); ++i)
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
	int landmarkCount = triggerData->getLandmarkCount();
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
	pm->bufferViews.push_back(BufferView());
	pm->bufferViews[0].buffer = 0;
	pm->bufferViews[0].byteLength = regionCount * 14 * 2;
	pm->bufferViews[0].byteOffset = 0;
	pm->bufferViews[0].target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
	pm->bufferViews[0].name = "Indices buffer view";

	pm->bufferViews.push_back(BufferView());
	pm->bufferViews[1].buffer = 0;
	pm->bufferViews[1].byteLength = regionCount * 14 * sizeof(float) * 3;
	pm->bufferViews[1].byteOffset = pm->bufferViews[0].byteLength;
	pm->bufferViews[1].byteStride = 12;
	pm->bufferViews[1].target = TINYGLTF_TARGET_ARRAY_BUFFER;
	pm->bufferViews[1].name = "Vertices buffer view";

	// Create accessors, meshes, and nodes
	pm->accessors.resize(regionCount * 2); // Allocate ahead of usage
	for (int i = 0; i < regionCount; ++i)
	{
		// Create accessors
		pm->accessors[i] = Accessor();
		pm->accessors[i + regionCount] = Accessor();
		pm->accessors[i].bufferView = 0; // Indices
		pm->accessors[i + regionCount].bufferView = 1; // Vertices
		pm->accessors[i].byteOffset = i * 14 * 2;
		pm->accessors[i + regionCount].byteOffset = i * 14 * sizeof(float) * 3;
		pm->accessors[i].componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
		pm->accessors[i + regionCount].componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		pm->accessors[i].type = TINYGLTF_TYPE_SCALAR;
		pm->accessors[i + regionCount].type = TINYGLTF_TYPE_VEC3;
		pm->accessors[i].count = 14;
		pm->accessors[i + regionCount].count = 14;
		pm->accessors[i + regionCount].minValues = { -0.5, -0.5, -0.5 };
		pm->accessors[i + regionCount].maxValues = { 0.5, 0.5, 0.5 };
		pm->accessors[i].name = "Indices accessor " + std::to_string(i);
		pm->accessors[i + regionCount].name = "Vertices accessor " + std::to_string(i);

		// Create meshes
		pm->meshes.push_back(Mesh());
		pm->meshes[i].primitives.push_back(Primitive());
		pm->meshes[i].primitives[0].mode = TINYGLTF_MODE_TRIANGLE_STRIP;
		pm->meshes[i].primitives[0].indices = i;
		pm->meshes[i].primitives[0].attributes["POSITION"] = i + regionCount;
		pm->meshes[i].name = "Mesh " + std::to_string(i);

		// Create nodes
		pm->nodes.push_back(Node());
		pm->nodes[i].mesh = i;
		BrnTrigger::TriggerRegion currentRegion;
		if (i < landmarkCount)
			currentRegion = triggerData->getLandmark(i);
		else
			currentRegion = triggerData->getGenericRegion(i - landmarkCount);
		pm->nodes[i].translation = {
			currentRegion.getBoxRegion().getPosX(),
			currentRegion.getBoxRegion().getPosY(),
			currentRegion.getBoxRegion().getPosZ()
		};
		Vector4 rotation = EulerToQuatRot({
			currentRegion.getBoxRegion().getRotX(),
			currentRegion.getBoxRegion().getRotY(),
			currentRegion.getBoxRegion().getRotZ()
		});
		pm->nodes[i].rotation = {
			rotation.getX(),
			rotation.getY(),
			rotation.getZ(),
			rotation.getW()
		};
		pm->nodes[i].scale = {
			currentRegion.getBoxRegion().getDimX(),
			currentRegion.getBoxRegion().getDimY(),
			currentRegion.getBoxRegion().getDimZ()
		};

		// Add custom properties and name to node
		Value::Object extras;
		// TriggerRegion
		extras["TriggerRegion ID"] = Value(currentRegion.getId());
		extras["TriggerRegion region index"] = Value(currentRegion.getRegionIndex());
		extras["TriggerRegion type"] = Value((uint8_t)currentRegion.getType());
		extras["TriggerRegion unknown 0"] = Value(currentRegion.getUnk0());
		if (i < landmarkCount) // Landmark
		{
			extras["Landmark design index"] = Value(triggerData->getLandmark(i).getDesignIndex());
			extras["Landmark district"] = Value(triggerData->getLandmark(i).getDistrict());
			extras["Landmark is online"] = Value((bool)triggerData->getLandmark(i).getFlags() & 1);

			uint64_t id = currentRegion.getId();
			pm->nodes[i].name = "Landmark " + std::to_string(i) + " (" + std::to_string(id) + ")";
		}
		else // GenericRegion
		{
			int idx = i - landmarkCount;
			extras["GenericRegion group ID"] = Value(triggerData->getGenericRegion(idx).getGroupId());
			extras["GenericRegion camera cut 1"] = Value(triggerData->getGenericRegion(idx).getCameraCut1());
			extras["GenericRegion camera cut 2"] = Value(triggerData->getGenericRegion(idx).getCameraCut2());
			extras["GenericRegion camera type 1"] = Value((int16_t)triggerData->getGenericRegion(idx).getcameraType1());
			extras["GenericRegion camera type 2"] = Value((int16_t)triggerData->getGenericRegion(idx).getcameraType2());
			extras["GenericRegion type"] = Value((uint8_t)triggerData->getGenericRegion(idx).getType());
			extras["GenericRegion is one way"] = Value((bool)triggerData->getGenericRegion(idx).getIsOneWay());

			uint64_t id = triggerData->getGenericRegion(idx).getGroupId();
			if (id == 0)
				id = currentRegion.getId();
			pm->nodes[i].name = "GenericRegion " + std::to_string(idx) + " (" + std::to_string(id) + ")";
		}
		pm->nodes[i].extras = Value(extras);

		// Add nodes to scene
		pm->scenes[0].nodes.push_back(i);
	}

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

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

	// TODO: Separate functions to have nodes for:
	// Landmark, Blackspot, VFXBoxRegion (all TriggerRegion derived)
	// SignatureStunt, Killzone (all containing GenericRegion arrays)
	// Remaining GenericRegion triggers (checked against SignatureStunts and Killzones to avoid duplication)
	// Remaining TriggerRegion triggers (checked against all of the above)
	// Add starting grids, roaming locations, and spawn locations afterward

	// Get counts
	int currentNodeCount = 0;
	//int landmarkCount = triggerData->getLandmarkCount();
	//int signatureStuntCount = triggerData->getSignatureStuntCount();
	//int genericRegionCount = triggerData->getGenericRegionCount();
	//int killzoneCount = triggerData->getKillzoneCount();
	//int blackspotCount = triggerData->getBlackspotCount();
	//int vfxBoxRegionCount = triggerData->getVfxBoxRegionCount();
	//int roamingLocationCount = triggerData->getRoamingLocationCount();
	//int spawnLocationCount = triggerData->getSpawnLocationCount();
	//int regionCount = triggerData->getRegionCount();

	// Create nodes

	// TriggerRegion derived nodes
	int landmarkNodeIndex = currentNodeCount;
	for (int i = 0; i < triggerData->getLandmarkCount(); ++i)
	{
		model->nodes.push_back(Node());
		model->nodes.back().mesh = 0;
		convertLandmark(triggerData->getLandmark(i), model->nodes[i + landmarkNodeIndex], i);
		currentNodeCount++;
	}
	int blackspotNodeIndex = currentNodeCount;
	for (int i = 0; i < triggerData->getBlackspotCount(); ++i)
	{
		model->nodes.push_back(Node());
		model->nodes.back().mesh = 0;
		convertBlackspot(triggerData->getBlackspot(i), model->nodes[blackspotNodeIndex + i], i);
		currentNodeCount++;
	}
	int vfxBoxRegionNodeIndex = currentNodeCount;
	for (int i = 0; i < triggerData->getVfxBoxRegionCount(); ++i)
	{
		model->nodes.push_back(Node());
		model->nodes.back().mesh = 0;
		convertVfxBoxRegion(triggerData->getVfxBoxRegion(i), model->nodes[vfxBoxRegionNodeIndex + i], i);
		currentNodeCount++;
	}

	// Nodes with GenericRegion arrays
	int signatureStuntNodeIndex = currentNodeCount;
	int signatureStuntChildCount = 0;
	for (int i = 0; i < triggerData->getSignatureStuntCount(); ++i)
	{
		model->nodes.push_back(Node());
		for (int j = 0; j < triggerData->getSignatureStunt(i).getStuntElementCount(); ++j)
		{
			model->nodes.push_back(Node());
			model->nodes.back().mesh = 0;
			model->nodes[signatureStuntNodeIndex + i + signatureStuntChildCount].children.push_back(signatureStuntNodeIndex + i + signatureStuntChildCount + j + 1);
			convertGenericRegion(triggerData->getSignatureStunt(i).getStuntElement(j), model->nodes[signatureStuntNodeIndex + i + signatureStuntChildCount + j + 1], j);
			currentNodeCount++;
		}
		convertSignatureStunt(triggerData->getSignatureStunt(i), model->nodes[signatureStuntNodeIndex + i + signatureStuntChildCount], i);
		currentNodeCount++;
		signatureStuntChildCount += triggerData->getSignatureStunt(i).getStuntElementCount();
	}
	int killzoneNodeIndex = currentNodeCount;
	int killzoneChildCount = 0;
	for (int i = 0; i < triggerData->getKillzoneCount(); ++i)
	{
		model->nodes.push_back(Node());
		for (int j = 0; j < triggerData->getKillzone(i).getTriggerCount(); ++j)
		{
			model->nodes.push_back(Node());
			model->nodes.back().mesh = 0;
			model->nodes[killzoneNodeIndex + i + killzoneChildCount].children.push_back(killzoneNodeIndex + i + killzoneChildCount + j + 1);
			convertGenericRegion(triggerData->getKillzone(i).getTrigger(j), model->nodes[killzoneNodeIndex + i + killzoneChildCount + j + 1], j);
			currentNodeCount++;
		}
		convertKillzone(triggerData->getKillzone(i), model->nodes[killzoneNodeIndex + i + killzoneChildCount], i);
		currentNodeCount++;
		killzoneChildCount += triggerData->getKillzone(i).getTriggerCount();
	}

	// Remaining GenericRegion nodes
	int genericRegionNodeIndex = currentNodeCount;
	int currentGenericRegionNode = 0;
	for (int i = 0; i < triggerData->getGenericRegionCount(); ++i)
	{
		if (!triggerRegionExists(triggerData->getGenericRegion(i), false))
		{
			model->nodes.push_back(Node());
			model->nodes.back().mesh = 0;
			convertGenericRegion(triggerData->getGenericRegion(i), model->nodes[genericRegionNodeIndex + currentGenericRegionNode], i);
			currentGenericRegionNode++;
			currentNodeCount++;
		}
	}

	// Remaining TriggerRegion nodes
	int triggerRegionNodeIndex = currentNodeCount;
	int currentTriggerRegionNode = 0;
	for (int i = 0; i < triggerData->getRegionCount(); ++i)
	{
		if (!triggerRegionExists(triggerData->getTriggerRegion(i)))
		{
			model->nodes.push_back(Node());
			model->nodes.back().mesh = 0;
			convertTriggerRegion(triggerData->getTriggerRegion(i), model->nodes[triggerRegionNodeIndex + currentTriggerRegionNode], i);
			currentTriggerRegionNode++;
			currentNodeCount++;
		}
	}

	// Add node indices to scene
	for (int i = 0; i < triggerData->getRegionCount(); ++i)
		model->scenes[0].nodes.push_back(i);

	// Set up asset
	model->asset.version = "2.0";
	model->asset.generator = "tinygltf";
	
	// Save it to a file
	tinygltf::TinyGLTF gltf;
	gltf.WriteGltfSceneToFile(model.data(), outFileName,
		true, // embedImages
		true, // embedBuffers
		true, // pretty print
		false); // write binary
}

bool Converter::triggerRegionExists(BrnTrigger::TriggerRegion region, bool checkGenericRegions)
{
	int32_t id = region.getId();
	for (int i = 0; i < triggerData->getLandmarkCount(); ++i)
	{
		if (id == triggerData->getLandmark(i).getId())
			return true;
	}
	for (int i = 0; i < triggerData->getBlackspotCount(); ++i)
	{
		if (id == triggerData->getBlackspot(i).getId())
			return true;
	}
	for (int i = 0; i < triggerData->getVfxBoxRegionCount(); ++i)
	{
		if (id == triggerData->getVfxBoxRegion(i).getId())
			return true;
	}
	if (checkGenericRegions)
	{
		for (int i = 0; i < triggerData->getGenericRegionCount(); ++i)
		{
			if (id == triggerData->getGenericRegion(i).getId())
				return true;
		}
		return false;
	}
	for (int i = 0; i < triggerData->getSignatureStuntCount(); ++i)
	{
		for (int j = 0; j < triggerData->getSignatureStunt(i).getStuntElementCount(); ++j)
		{
			if (id == triggerData->getSignatureStunt(i).getStuntElement(j).getId())
				return true;
		}
	}
	for (int i = 0; i < triggerData->getKillzoneCount(); ++i)
	{
		for (int j = 0; j < triggerData->getKillzone(i).getTriggerCount(); ++j)
		{
			if (id == triggerData->getKillzone(i).getTrigger(j).getId())
				return true;
		}
	}
	return false;
}

void Converter::addTriggerRegionFields(BrnTrigger::TriggerRegion region, Value::Object& extras)
{
	extras["TriggerRegion ID"] = Value(region.getId());
	extras["TriggerRegion region index"] = Value(region.getRegionIndex());
	extras["TriggerRegion type"] = Value((uint8_t)region.getType());
	extras["TriggerRegion unknown 0"] = Value(region.getUnk0());
}

void Converter::convertLandmark(BrnTrigger::Landmark landmark, Node& node, int index)
{
	addBoxRegionTransform(landmark, node);

	Value::Object extras;
	addTriggerRegionFields(landmark, extras);
	extras["Design index"] = Value(landmark.getDesignIndex());
	extras["District"] = Value(landmark.getDistrict());
	extras["Is online"] = Value(landmark.getIsOnline());
	node.extras = Value(extras);

	node.name = "Landmark " + std::to_string(index) + " (" + std::to_string(landmark.getId()) + ")";
}

void Converter::convertBlackspot(BrnTrigger::Blackspot blackspot, tinygltf::Node& node, int index)
{
	addBoxRegionTransform(blackspot, node);

	Value::Object extras;
	addTriggerRegionFields(blackspot, extras);
	extras["Score type"] = Value((uint8_t)blackspot.getScoreType());
	extras["Score amount"] = Value(blackspot.getScoreAmount());
	node.extras = Value(extras);

	node.name = "Blackspot " + std::to_string(index) + " (" + std::to_string(blackspot.getId()) + ")";
}

void Converter::convertVfxBoxRegion(BrnTrigger::VFXBoxRegion vfxBoxRegion, tinygltf::Node& node, int index)
{
	addBoxRegionTransform(vfxBoxRegion, node);

	node.name = "VFXBoxRegion " + std::to_string(index) + " (" + std::to_string(vfxBoxRegion.getId()) + ")";
}

void Converter::convertSignatureStunt(BrnTrigger::SignatureStunt signatureStunt, tinygltf::Node& node, int index)
{
	Value::Object extras;
	extras["ID"] = Value((int)signatureStunt.getId());
	extras["Camera"] = Value((int)signatureStunt.getCamera());
	node.extras = Value(extras);

	node.name = "SignatureStunt " + std::to_string(index) + " (" + std::to_string(signatureStunt.getId()) + ")";
}

void Converter::convertKillzone(BrnTrigger::Killzone killzone, tinygltf::Node& node, int index)
{
	Value::Object extras;
	for (int i = 0; i < killzone.getRegionIdCount(); ++i)
		extras["Region ID " + std::to_string(i)] = Value((int)killzone.getRegionId(i));
	node.extras = Value(extras);

	node.name = "Killzone " + std::to_string(index);
}

void Converter::convertGenericRegion(BrnTrigger::GenericRegion region, tinygltf::Node& node, int index)
{
	addBoxRegionTransform(region, node);

	Value::Object extras;
	addTriggerRegionFields(region, extras);
	extras["GenericRegion group ID"] = Value(region.getGroupId());
	extras["GenericRegion camera cut 1"] = Value(region.getCameraCut1());
	extras["GenericRegion camera cut 2"] = Value(region.getCameraCut2());
	extras["GenericRegion camera type 1"] = Value((int16_t)region.getcameraType1());
	extras["GenericRegion camera type 2"] = Value((int16_t)region.getcameraType2());
	extras["GenericRegion type"] = Value((uint8_t)region.getType());
	extras["GenericRegion is one way"] = Value((bool)region.getIsOneWay());
	node.extras = Value(extras);

	uint64_t id = region.getGroupId();
	if (id == 0)
		id = region.getId();
	node.name = "GenericRegion " + std::to_string(index) + " (" + std::to_string(id) + ")";
}

void Converter::convertTriggerRegion(BrnTrigger::TriggerRegion triggerRegion, tinygltf::Node& node, int index)
{
	addBoxRegionTransform(triggerRegion, node);

	node.name = "TriggerRegion " + std::to_string(index) + " (" + std::to_string(triggerRegion.getId()) + ")";
}

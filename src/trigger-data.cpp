#include <trigger-data.h>

using namespace BrnTrigger;

void TriggerData::read(DataStream& file)
{
	file >> versionNumber;
	file >> size;
	file.skip(0x8);
	playerStartPosition.read(file);
	playerStartDirection.read(file);
	file >> landmarks;
	file >> landmarkCount;
	file >> onlineLandmarkCount;
	file >> signatureStunts;
	file.skipIf64(0x4);
	file >> signatureStuntCount;
	file >> genericRegions;
	file >> genericRegionCount;
	file.skipIf64(0x4);
	file >> killzones;
	file >> killzoneCount;
	file.skipIf64(0x4);
	file >> blackspots;
	file >> blackspotCount;
	file.skipIf64(0x4);
	file >> vfxBoxRegions;
	file >> vfxBoxRegionCount;
	file.skipIf64(0x4);
	file >> roamingLocations;
	file >> roamingLocationCount;
	file.skipIf64(0x4);
	file >> spawnLocations;
	file >> spawnLocationCount;
	file.skipIf64(0x4);
	file >> regions;
	file >> regionCount;
	file.skip(0x4);

	// Allocate and read each trigger chunk
	file.cAllocAndCustomRead(landmarks, landmarkCount);
	file.cAllocAndCustomRead(signatureStunts, signatureStuntCount);
	file.cAllocAndCustomRead(genericRegions, genericRegionCount);
	file.cAllocAndCustomRead(killzones, killzoneCount);
	file.cAllocAndCustomRead(blackspots, blackspotCount);
	file.cAllocAndCustomRead(vfxBoxRegions, vfxBoxRegionCount);
	file.cAllocAndCustomRead(roamingLocations, roamingLocationCount);
	file.cAllocAndCustomRead(spawnLocations, spawnLocationCount);
	file.cAllocAndQtRead(regions, regionCount);
	for (int i = 0; i < regionCount; ++i)
		file.cAllocAndCustomRead(regions[i], 1);
}

void BoxRegion::read(DataStream& file)
{
	file >> positionX;
	file >> positionY;
	file >> positionZ;
	file >> rotationX;
	file >> rotationY;
	file >> rotationZ;
	file >> dimensionX;
	file >> dimensionY;
	file >> dimensionZ;
}

void TriggerRegion::read(DataStream& file)
{
	boxRegion.read(file);
	file >> id;
	file >> regionIndex;
	file >> type;
	file >> unk0;
}

void Landmark::read(DataStream& file)
{
	TriggerRegion::read(file);
	file.skipIf64(0x4);
	file >> startingGrids;
	file >> startingGridCount;
	file >> designIndex;
	file >> district;
	file >> flags;
	file.skipIf64(0x4);

	qint64 nextLandmark = file.pos();

	// Allocate and read starting grids
	file.cAllocAndCustomRead(startingGrids, startingGridCount);

	file.seek(nextLandmark);
}

StartingGrid::StartingGrid()
{
	for (int i = 0; i < 8; ++i)
	{
		startingPositions[i] = Vector3(true);
		startingDirections[i] = Vector3(true);
	}
}

void StartingGrid::read(DataStream& file)
{
	for (int i = 0; i < 8; ++i)
		startingPositions[i].read(file);
	for (int i = 0; i < 8; ++i)
		startingDirections[i].read(file);
}

void SignatureStunt::read(DataStream& file)
{
	file >> id;
	file >> camera;
	file >> stuntElements;
	file >> stuntElementCount;
	file.skipIf64(0x4);
	qint64 nextSignatureStunt = file.pos(); // Save offset to return to it later

	// Allocate and read generic region pointers and generic regions
	file.cAllocAndQtRead(stuntElements, stuntElementCount);
	for (int i = 0; i < stuntElementCount; ++i)
		file.cAllocAndCustomRead(stuntElements[i], 1);

	file.seek(nextSignatureStunt);
}

void GenericRegion::read(DataStream& file)
{
	TriggerRegion::read(file);
	file >> groupId;
	file >> cameraCut1;
	file >> cameraCut2;
	file >> cameraType1;
	file >> cameraType2;
	file >> type;
	file >> isOneWay;
}

void Killzone::read(DataStream& file)
{
	file >> triggers;
	file >> triggerCount;
	file.skipIf64(0x4);
	file >> regionIds;
	file >> regionIdCount;
	file.skipIf64(0x4);
	qint64 nextKillzone = file.pos(); // Save offset to return to it later

	// Allocate and read generic region pointers, generic regions, and region IDs
	file.cAllocAndQtRead(triggers, triggerCount);
	for (int i = 0; i < triggerCount; ++i)
		file.cAllocAndCustomRead(triggers[i], 1);
	file.cAllocAndQtRead(regionIds, regionIdCount);

	file.seek(nextKillzone);
}

void Blackspot::read(DataStream& file)
{
	TriggerRegion::read(file);
	file >> scoreType;
	file.skip(0x3);
	file >> scoreAmount;
}

void VFXBoxRegion::read(DataStream& file)
{
	TriggerRegion::read(file);
}

void RoamingLocation::read(DataStream& file)
{
	position.read(file);
	file >> districtIndex;
	file.skip(0xF);
}

void SpawnLocation::read(DataStream& file)
{
	position.read(file);
	direction.read(file);
	file >> junkyardId;
	file >> type;
	file.skip(0x7);
}

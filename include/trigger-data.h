#pragma once

#include <types.h>

namespace BrnTrigger
{
	// Transform for box triggers.
	// Sphere and line types are not supported.
	struct BoxRegion
	{
		void read(DataStream& file);
		void write(DataStream& file);

		float32_t positionX = 0;
		float32_t positionY = 0;
		float32_t positionZ = 0;
		float32_t rotationX = 0;
		float32_t rotationY = 0;
		float32_t rotationZ = 0;
		float32_t dimensionX = 0;
		float32_t dimensionY = 0;
		float32_t dimensionZ = 0;
	};

	// General container for box triggers.
	struct TriggerRegion
	{
		enum class Type : uint8_t
		{
			landmark,
			blackspot,
			genericRegion,
			vfxBoxRegion
		};

		void read(DataStream& file);
		void write(DataStream& file);

		BoxRegion boxRegion;
		int32_t id = 0;
		int16_t regionIndex = 0;
		Type type = (Type)0;
		uint8_t unk0 = 0; // Flags? 0 or 1. No longer padding. TODO: Find out what version introduced this
	};

	// Starting grid for race events. Unused in retail.
	struct StartingGrid
	{
	public:
		StartingGrid();

		void read(DataStream& file);
		void write(DataStream& file);

		Vector3 startingPositions[8];
		Vector3 startingDirections[8];
	};

	// Used for finish lines and checkpoints (but not event starts).
	// Big Surf Island has an overabundance of Landmarks.
	struct Landmark : public TriggerRegion
	{
		enum class Flags : uint8_t
		{
			isOnline = 1 << 0
		};

		void read(DataStream& file);
		void write(DataStream& file);

		StartingGrid* startingGrids;
		int8_t startingGridCount = 0;
		uint8_t designIndex = 0; // Landmark-only index
		uint8_t district = 0;
		Flags flags = (Flags)0;
	};

	// Generic box trigger. This is the main trigger type used by the game.
	struct GenericRegion : public TriggerRegion
	{
		enum class StuntCameraType : int8_t
		{
			noCuts,
			custom,
			normal
		};

		enum class Type : uint8_t
		{
			junkyard,
			gasStation,
			autoRepair,
			paintShop,
			carPark,
			signatureTakedown,
			killzone,
			jump,
			smash,
			signatureCrash,
			signatureCrashCamera,
			roadLimit,
			overdriveBoost,
			overdriveStrength,
			overdriveSpeed,
			overdriveControl,
			tireShop,
			tuningShop,
			pictureParadise,
			tunnel,
			overpass,
			bridge,
			warehouse,
			largeOverheadObject,
			narrowAlley,
			passTunnel,
			passOverpass,
			passBridge,
			passWarehouse,
			passLargeOverheadObject,
			passNarrowAlley,
			ramp
		};

		void read(DataStream& file);
		void write(DataStream& file);

		int32_t groupId = 0; // GameDB ID
		int16_t cameraCut1 = 0;
		int16_t cameraCut2 = 0;
		StuntCameraType cameraType1 = (StuntCameraType)0;
		StuntCameraType cameraType2 = (StuntCameraType)0;
		Type type = (Type)0;
		int8_t isOneWay = 0;
	};

	// Accident blackspot (crash mode). Unused in retail.
	struct Blackspot : public TriggerRegion
	{
		enum class ScoreType : uint8_t
		{
			distance,
			carCount
		};

		void read(DataStream& file);
		void write(DataStream& file);

		ScoreType scoreType = (ScoreType)0;
		int32_t scoreAmount = 0;
	};

	// VFX region. Unused in retail.
	struct VFXBoxRegion : public TriggerRegion
	{
		void read(DataStream& file);
		void write(DataStream& file);
	};

	// TODO: Description
	struct SignatureStunt
	{
		void read(DataStream& file);
		void write(DataStream& file);

		GenericRegion getStuntElement(int index) { return stuntElements[index][0]; }
		void setStuntElement(GenericRegion region, int index) { stuntElements[index][0] = region; }

		CgsID id = 0;
		int64_t camera = 0;
		GenericRegion** stuntElements = nullptr;
		int32_t stuntElementCount = 0;
	};

	// TODO: Description
	struct Killzone
	{
		void read(DataStream& file);
		void write(DataStream& file);

		GenericRegion getTrigger(int index) { return triggers[index][0]; }
		void setTrigger(GenericRegion region, int index) { triggers[index][0] = region; }

		GenericRegion** triggers = nullptr;
		int32_t triggerCount = 0;
		CgsID* regionIds = nullptr; // GameDB IDs
		int32_t regionIdCount = 0;
	};

	// Spawn locations for roaming rivals (shutdown cars)
	struct RoamingLocation
	{
		void read(DataStream& file);
		void write(DataStream& file);

		Vector3 position = Vector3(true);
		uint8_t districtIndex = 0;
	};

	// Vehicle spawn locations in and outside each Junkyard
	struct SpawnLocation
	{
		enum class Type : uint8_t
		{
			playerSpawn,
			carSelectLeft,
			carSelectRight,
			carUnlock
		};

		void read(DataStream& file);
		void write(DataStream& file);

		Vector3 position = Vector3(true);
		Vector3 direction = Vector3(true);
		CgsID junkyardId = 0; // GameDB ID
		Type type = (Type)0;
	};

	// The header for the TriggerData resource.
	// Lists all relevant offsets and counts.
	struct TriggerData
	{
		void read(DataStream& file);
		void write(DataStream& file);

		TriggerRegion getRegion(int index) { return regions[index][0]; }
		void setRegion(TriggerRegion region, int index) { regions[index][0] = region; }

		int32_t versionNumber = 0;
		uint32_t size = 0;
		Vector3 playerStartPosition = Vector3(true);
		Vector3 playerStartDirection = Vector3(true);
		Landmark* landmarks = nullptr;
		int32_t landmarkCount = 0;
		int32_t onlineLandmarkCount = 0;
		SignatureStunt* signatureStunts = nullptr;
		int32_t signatureStuntCount = 0;
		GenericRegion* genericRegions = nullptr;
		int32_t genericRegionCount = 0;
		Killzone* killzones = nullptr;
		int32_t killzoneCount = 0;
		Blackspot* blackspots = nullptr;
		int32_t blackspotCount = 0;
		VFXBoxRegion* vfxBoxRegions = nullptr;
		int32_t vfxBoxRegionCount = 0;
		RoamingLocation* roamingLocations = nullptr;
		int32_t roamingLocationCount = 0;
		SpawnLocation* spawnLocations = nullptr;
		int32_t spawnLocationCount = 0;
		TriggerRegion** regions = nullptr;
		int32_t regionCount = 0;
	};
};

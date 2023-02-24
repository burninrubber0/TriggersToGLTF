#pragma once

#include <types.h>

namespace BrnTrigger
{
	// Forward declare classes
	class Landmark;
	class SignatureStunt;
	class GenericRegion;
	class Killzone;
	class Blackspot;
	class VFXBoxRegion;
	class RoamingLocation;
	class SpawnLocation;
	class TriggerRegion;
	class StartingGrid;

	// The header for the TriggerData resource.
	// Lists all relevant offsets and counts.
	class TriggerData
	{
	public:
		void read(DataStream& file);

		Landmark getLandmark(int index);
		SignatureStunt getSignatureStunt(int index);
		GenericRegion getGenericRegion(int index);
		Killzone getKillzone(int index);
		Blackspot getBlackspot(int index);
		VFXBoxRegion getVfxBoxRegion(int index);
		RoamingLocation getRoamingLocation(int index);
		SpawnLocation getSpawnLocation(int index);
		TriggerRegion getTriggerRegion(int index);

		int32_t getLandmarkCount() { return landmarkCount; }
		int32_t getSignatureStuntCount() { return signatureStuntCount; }
		int32_t getGenericRegionCount() { return genericRegionCount; }
		int32_t getKillzoneCount() { return killzoneCount; }
		int32_t getBlackspotCount() { return blackspotCount; }
		int32_t getVfxBoxRegionCount() { return vfxBoxRegionCount; }
		int32_t getRoamingLocationCount() { return roamingLocationCount; }
		int32_t getSpawnLocationCount() { return spawnLocationCount; }
		int32_t getRegionCount() { return regionCount; }
		int32_t getTotalRegionCount();

	private:
		int32_t versionNumber = 0;
		uint32_t size = 0;
		Vector3 playerStartPosition;
		Vector3 playerStartDirection;
		Landmark* landmarks = nullptr;
		int32_t landmarkCount = 0;
		int32_t onlineLandmarkCount = 0;
		SignatureStunt* signatureStunts = nullptr;
		int32_t signatureStuntCount = 0;
		GenericRegion* genericRegions = nullptr;
		int32_t genericRegionCount = 0;
		Killzone* killzones = nullptr;
		int32_t killzoneCount = 0;
		Blackspot* fileBlackspots = nullptr;
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

	// BoxRegions are cuboid triggers. They are the only type used.
	class BoxRegion
	{
	public:
		void read(DataStream& file);

		float getPosX() { return positionX; }
		float getPosY() { return positionY; }
		float getPosZ() { return positionZ; }
		float getRotX() { return rotationX; }
		float getRotY() { return rotationY; }
		float getRotZ() { return rotationZ; }
		float getDimX() { return dimensionX; }
		float getDimY() { return dimensionY; }
		float getDimZ() { return dimensionZ; }

	private:
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

	// The actual data for the trigger. Used on nearly all types.
	class TriggerRegion
	{
		enum class Type : uint8_t;

	public:
		void read(DataStream& file);

		BoxRegion getBoxRegion() { return boxRegion; }
		int32_t getId() { return id; }
		int16_t getRegionIndex() { return regionIndex; }
		Type getType() { return type; }
		uint8_t getUnk0() { return unk0; }

	private:
		// The type of trigger region.
		enum class Type : uint8_t
		{
			landmark,
			blackspot,
			genericRegion,
			vfxBoxRegion
		};

		BoxRegion boxRegion;
		int32_t id = 0;
		int16_t regionIndex = 0; // Landmarks come after GenericRegions
		Type type = (Type)0;
		uint8_t unk0 = 0; // Flags? 0 or 1. No longer padding
	};

	// Used for finish lines and event starts. Big Surf Island has an
	// overabundance of Landmarks.
	class Landmark : public TriggerRegion
	{
		enum class Flags : uint8_t;

	public:
		void read(DataStream& file);

		StartingGrid getStartingGrid(int index);
		uint8_t getDesignIndex() { return designIndex; }
		uint8_t getDistrict() { return district; }
		Flags getFlags() { return flags; }
		bool getIsOnline() { return ((uint8_t)flags & (uint8_t)Flags::isOnline) != 0; };

		int8_t getStartingGridCount() { return startingGridCount; }

	private:
		// The flags field in the Landmark structure.
		enum class Flags : uint8_t
		{
			isOnline = 1 << 0
		};

		StartingGrid* startingGrids;
		int8_t startingGridCount = 0;
		uint8_t designIndex = 0; // Landmark-only index
		uint8_t district = 0;
		Flags flags = (Flags)0;
	};

	// Starting grid for race events. Data exists but is unused in retail.
	class StartingGrid
	{
	public:
		void read(DataStream& file);

		Vector3 getStartingPosition(int index) { return startingPositions[index]; }
		Vector3 getStartingDirection(int index) { return startingDirections[index]; }

	private:
		Vector3 startingPositions[8];
		Vector3 startingDirections[8];
	};

	// TODO: Description
	class SignatureStunt
	{
	public:
		void read(DataStream& file);

		CgsID getId() { return id; }
		int64_t getCamera() { return camera; }
		GenericRegion getStuntElement(int index);

		int32_t getStuntElementCount() { return stuntElementCount; }

	private:
		CgsID id = 0;
		int64_t camera = 0;
		GenericRegion** stuntElements = nullptr;
		int32_t stuntElementCount = 0;
	};

	// Generic box trigger. The main trigger type used. Used for many things.
	class GenericRegion : public TriggerRegion
	{
		enum class StuntCameraType : int8_t;
		enum class Type : uint8_t;

	public:
		void read(DataStream& file);

		int32_t getGroupId() { return groupId; }
		int16_t getCameraCut1() { return cameraCut1; }
		int16_t getCameraCut2() { return cameraCut2; }
		StuntCameraType getcameraType1() { return cameraType1; }
		StuntCameraType getcameraType2() { return cameraType2; }
		Type getType() { return type; }
		int8_t getIsOneWay() { return isOneWay; }

	private:
		// Camera type used on GenericRegion stunts.
		enum class StuntCameraType : int8_t
		{
			noCuts,
			custom,
			normal
		};

		// The type of generic region. Uses include audio, drivethru, and camera
		// triggers, among others.
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

		int32_t groupId = 0; // GameDB ID
		int16_t cameraCut1 = 0;
		int16_t cameraCut2 = 0;
		StuntCameraType cameraType1 = (StuntCameraType)0;
		StuntCameraType cameraType2 = (StuntCameraType)0;
		Type type = (Type)0;
		int8_t isOneWay = 0;
	};

	// TODO: Description
	class Killzone
	{
	public:
		void read(DataStream& file);

		GenericRegion getTrigger(int index) { return triggers[index][0]; }
		CgsID getRegionId(int index) { return regionIds[index]; }

		int32_t getTriggerCount() { return triggerCount; }
		int32_t getRegionIdCount() { return regionIdCount; }

	private:
		GenericRegion** triggers = nullptr;
		int32_t triggerCount = 0;
		CgsID* regionIds = nullptr; // GameDB IDs
		int32_t regionIdCount = 0;
	};

	// Accident blackspot (crash mode). Unused in retail.
	class Blackspot : public TriggerRegion
	{
		enum class ScoreType : uint8_t;

	public:
		void read(DataStream& file);

		ScoreType getScoreType() { return scoreType; }
		int32_t getScoreAmount() { return scoreAmount; }

	private:
		// The Blackspot scoring system. Note that both are used in Showtime.
		enum class ScoreType : uint8_t
		{
			distance,
			carCount
		};

		ScoreType scoreType = (ScoreType)0;
		int32_t scoreAmount = 0;
	};

	// VFX region. Unused in retail.
	class VFXBoxRegion : public TriggerRegion
	{
	public:
		void read(DataStream& file);

	private:

	};

	// TODO: Description
	class RoamingLocation
	{
	public:
		void read(DataStream& file);

		Vector3 getPosition() { return position; }
		uint8_t getDistrictIndex() { return districtIndex; }

	private:
		Vector3 position;
		uint8_t districtIndex = 0;
	};

	// TODO: Description
	class SpawnLocation
	{
		enum class Type : uint8_t;

	public:
		void read(DataStream& file);

		Vector3 getPosition() { return position; }
		Vector3 getDirection() { return direction; }
		CgsID getJunkyardId() { return junkyardId; }
		Type getType() { return type; }

	private:
		// The type of spawn at a spawn location.
		enum class Type : uint8_t
		{
			playerSpawn,
			carSelectLeft,
			carSelectRight,
			carUnlock
		};

		Vector3 position;
		Vector3 direction;
		CgsID junkyardId = 0; // GameDB ID
		Type type = (Type)0;
	};
};

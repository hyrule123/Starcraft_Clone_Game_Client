#pragma once
#include <cstdint>
#include <type_traits>
#include <limits>


namespace SCSim
{
	struct Context;	// Forward

	// sprite_t::flags 원본 비트 (1.16.1 배치). 아래에 없는 비트는 계약에 포함되지 않음
	// 0x8(selected)은 OpenBW에서 설정되지 않아 항상 0 — 선택 정보로 사용 금지
	namespace SpriteFlags
	{
		enum : uint32_t
		{
			None = 0,
			Turret = 0x10,
			Hidden = 0x20,
			Burrowed = 0x40,
			IscriptNoBrk = 0x80,	// iscript nobrkcodestart~end 구간 (애니메이션 중단 불가)
		};
	}

	struct SpriteData
	{
		uint32_t index;				// sprite_t::index (슬롯 번호, 0도 유효)
		int32_t  owner;				// sprite_t::owner (0~11). 색상은 SCSim_GetPlayers로 조회
		int32_t  elevation_level;	// sprite_t::elevation_level
		int32_t  x;					// sprite_t::position
		int32_t  y;
		uint32_t first_image;		// SpriteSnapshot::images 내 시작 위치
		uint32_t image_count;		// OpenBW 목록 순서(앞 = 맨 위층)로 연속 배치. 숨김 이미지 포함
		uint32_t flags;				// SpriteFlags
	};
	static_assert(sizeof(SpriteData) == 32);

	// image_t::flags 원본 비트 (1.16.1 배치). 아래에 없는 비트는 계약에 포함되지 않음
	// 0x1(redraw)은 OpenBW에서 해제되지 않아 의미 없음 — 사용 금지
	namespace ImageFlags
	{
		enum : uint32_t
		{
			None = 0,
			HorizontallyFlipped = 0x2,
			YFrozen = 0x4,
			HasDirectionalFrames = 0x8,
			HasIscriptAnimations = 0x10,
			Clickable = 0x20,
			Hidden = 0x40,
			UsesSpecialOffset = 0x80,
		};
	}

	struct ImageData
	{
		uint32_t index;			// image_t::index (슬롯 번호, 0도 유효)
		uint32_t sprite_index;	// 소속 sprite_t::index
		uint32_t image_type;	// image_type->id (images.dat 번호)
		uint32_t frame;			// image_t::frame_index
		int32_t  offset_x;		// image_t::offset (스프라이트 기준 로컬 좌표)
		int32_t  offset_y;
		int32_t  modifier;		// image_t::modifier
		uint32_t flags;			// ImageFlags
	};
	static_assert(sizeof(ImageData) == 32);
	constexpr uint32_t kInvalidSpriteIndex = std::numeric_limits<int32_t>::max();

	struct PlayerData
	{
		int32_t color;			// player_t::color (LoadMap에서 0~15 보장)
	};
	static_assert(sizeof(PlayerData) == 4);

	static_assert(std::is_trivially_copyable_v<PlayerData>&& std::is_standard_layout_v<PlayerData>);
	static_assert(std::is_trivially_copyable_v<SpriteData>&& std::is_standard_layout_v<SpriteData>);
	static_assert(std::is_trivially_copyable_v<ImageData>&& std::is_standard_layout_v<ImageData>);

	struct SpriteSnapshot
	{
		const SpriteData* sprites;
		const ImageData* images;
		int32_t sprite_count;
		int32_t image_count;
	};

	struct UnitData {
		uint32_t id;
		int32_t  x;          // 시뮬레이션 위치(u->position)
		int32_t  y;
		uint16_t sprite_index;
		uint32_t flags;      // SCSimUnitFlags
	};
	static_assert(sizeof(UnitData) == 20, "UnitData layout changed. Must update FillUnit()");

	namespace UnitFlags {
		enum : uint32_t {
			None = 0,
			Hidden = 1u << 0,
		};
	}  // namespace UnitFlags


	constexpr uint32_t kInvalidUnitId = 0;
	constexpr size_t kMaxUnits = 1700;

	constexpr size_t kMaxMapTiles = 256;		// 1.16.1 맵 최대 크기(타일), OpenBW tile_region_index도 256x256 고정
	constexpr int32_t kMaxPlayerColors = 16;	// 1.16.1 플레이어 색상 수(tunit.pcx 16행)

	enum class Player : int32_t {
		_1 = 0,
		_2 = 1,
		_3 = 2,
		_4 = 3,
		_5 = 4,
		_6 = 5,
		_7 = 6,
		_8 = 7,
		_9 = 8,
		_10 = 9,
		_11 = 10,
		Neutral = 11,
		kCount
	};

	constexpr int kMaxActionPlayers = 8;  // action_state::selection 크기
	constexpr int kMaxSelection = 12;     // 선택 가능한 최대값(부대지정)

	constexpr uint32_t kMaxSprites = 2500;	// OpenBW sprites_container 상한
	constexpr uint32_t kMaxImages = 5000;	// OpenBW images_container 상한

	enum class UnitType : int32_t {
		TerranMarine = 0,
		TerranGhost = 1,
		TerranVulture = 2,
		TerranGoliath = 3,
		TerranGoliathTurret = 4,
		TerranSiegeTankTankMode = 5,
		TerranSiegeTankTankModeTurret = 6,
		TerranSCV = 7,
		TerranWraith = 8,
		TerranScienceVessel = 9,
		HeroGuiMontag = 10,
		TerranDropship = 11,
		TerranBattlecruiser = 12,
		TerranVultureSpiderMine = 13,
		TerranNuclearMissile = 14,
		TerranCivilian = 15,
		HeroSarahKerrigan = 16,
		HeroAlanSchezar = 17,
		HeroAlanSchezarTurret = 18,
		HeroJimRaynorVulture = 19,
		HeroJimRaynorMarine = 20,
		HeroTomKazansky = 21,
		HeroMagellan = 22,
		HeroEdmundDukeTankMode = 23,
		HeroEdmundDukeTankModeTurret = 24,
		HeroEdmundDukeSiegeMode = 25,
		HeroEdmundDukeSiegeModeTurret = 26,
		HeroArcturusMengsk = 27,
		HeroHyperion = 28,
		HeroNoradII = 29,
		TerranSiegeTankSiegeMode = 30,
		TerranSiegeTankSiegeModeTurret = 31,
		TerranFirebat = 32,
		SpellScannerSweep = 33,
		TerranMedic = 34,
		ZergLarva = 35,
		ZergEgg = 36,
		ZergZergling = 37,
		ZergHydralisk = 38,
		ZergUltralisk = 39,
		ZergBroodling = 40,
		ZergDrone = 41,
		ZergOverlord = 42,
		ZergMutalisk = 43,
		ZergGuardian = 44,
		ZergQueen = 45,
		ZergDefiler = 46,
		ZergScourge = 47,
		HeroTorrasque = 48,
		HeroMatriarch = 49,
		ZergInfestedTerran = 50,
		HeroInfestedKerrigan = 51,
		HeroUncleanOne = 52,
		HeroHunterKiller = 53,
		HeroDevouringOne = 54,
		HeroKukulzaMutalisk = 55,
		HeroKukulzaGuardian = 56,
		HeroYggdrasill = 57,
		TerranValkyrie = 58,
		ZergCocoon = 59,
		ProtossCorsair = 60,
		ProtossDarkTemplar = 61,
		ZergDevourer = 62,
		ProtossDarkArchon = 63,
		ProtossProbe = 64,
		ProtossZealot = 65,
		ProtossDragoon = 66,
		ProtossHighTemplar = 67,
		ProtossArchon = 68,
		ProtossShuttle = 69,
		ProtossScout = 70,
		ProtossArbiter = 71,
		ProtossCarrier = 72,
		ProtossInterceptor = 73,
		HeroDarkTemplar = 74,
		HeroZeratul = 75,
		HeroTassadarZeratulArchon = 76,
		HeroFenixZealot = 77,
		HeroFenixDragoon = 78,
		HeroTassadar = 79,
		HeroMojo = 80,
		HeroWarbringer = 81,
		HeroGantrithor = 82,
		ProtossReaver = 83,
		ProtossObserver = 84,
		ProtossScarab = 85,
		HeroDanimoth = 86,
		HeroAldaris = 87,
		HeroArtanis = 88,
		CritterRhynadon = 89,
		CritterBengalaas = 90,
		SpecialCargoShip = 91,
		SpecialMercenaryGunship = 92,
		CritterScantid = 93,
		CritterKakaru = 94,
		CritterRagnasaur = 95,
		CritterUrsadon = 96,
		ZergLurkerEgg = 97,
		HeroRaszagal = 98,
		HeroSamirDuran = 99,
		HeroAlexeiStukov = 100,
		SpecialMapRevealer = 101,
		HeroGerardDuGalle = 102,
		ZergLurker = 103,
		HeroInfestedDuran = 104,
		SpellDisruptionWeb = 105,
		TerranCommandCenter = 106,
		TerranComsatStation = 107,
		TerranNuclearSilo = 108,
		TerranSupplyDepot = 109,
		TerranRefinery = 110,
		TerranBarracks = 111,
		TerranAcademy = 112,
		TerranFactory = 113,
		TerranStarport = 114,
		TerranControlTower = 115,
		TerranScienceFacility = 116,
		TerranCovertOps = 117,
		TerranPhysicsLab = 118,
		UnusedTerran1 = 119,
		TerranMachineShop = 120,
		UnusedTerran2 = 121,
		TerranEngineeringBay = 122,
		TerranArmory = 123,
		TerranMissileTurret = 124,
		TerranBunker = 125,
		SpecialCrashedNoradII = 126,
		SpecialIonCannon = 127,
		PowerupUrajCrystal = 128,
		PowerupKhalisCrystal = 129,
		ZergInfestedCommandCenter = 130,
		ZergHatchery = 131,
		ZergLair = 132,
		ZergHive = 133,
		ZergNydusCanal = 134,
		ZergHydraliskDen = 135,
		ZergDefilerMound = 136,
		ZergGreaterSpire = 137,
		ZergQueensNest = 138,
		ZergEvolutionChamber = 139,
		ZergUltraliskCavern = 140,
		ZergSpire = 141,
		ZergSpawningPool = 142,
		ZergCreepColony = 143,
		ZergSporeColony = 144,
		UnusedZerg1 = 145,
		ZergSunkenColony = 146,
		SpecialOvermindWithShell = 147,
		SpecialOvermind = 148,
		ZergExtractor = 149,
		SpecialMatureChrysalis = 150,
		SpecialCerebrate = 151,
		SpecialCerebrateDaggoth = 152,
		UnusedZerg2 = 153,
		ProtossNexus = 154,
		ProtossRoboticsFacility = 155,
		ProtossPylon = 156,
		ProtossAssimilator = 157,
		UnusedProtoss1 = 158,
		ProtossObservatory = 159,
		ProtossGateway = 160,
		UnusedProtoss2 = 161,
		ProtossPhotonCannon = 162,
		ProtossCitadelOfAdun = 163,
		ProtossCyberneticsCore = 164,
		ProtossTemplarArchives = 165,
		ProtossForge = 166,
		ProtossStargate = 167,
		SpecialStasisCellPrison = 168,
		ProtossFleetBeacon = 169,
		ProtossArbiterTribunal = 170,
		ProtossRoboticsSupportBay = 171,
		ProtossShieldBattery = 172,
		SpecialKhaydarinCrystalForm = 173,
		SpecialProtossTemple = 174,
		SpecialXelNagaTemple = 175,
		ResourceMineralField = 176,
		ResourceMineralFieldType2 = 177,
		ResourceMineralFieldType3 = 178,
		UnusedCave = 179,
		UnusedCaveIn = 180,
		UnusedCantina = 181,
		UnusedMiningPlatform = 182,
		UnusedIndependantCommandCenter = 183,
		SpecialIndependantStarport = 184,
		UnusedIndependantJumpGate = 185,
		UnusedRuins = 186,
		UnusedKhaydarinCrystalFormation = 187,
		ResourceVespeneGeyser = 188,
		SpecialWarpGate = 189,
		SpecialPsiDisrupter = 190,
		UnusedZergMarker = 191,
		UnusedTerranMarker = 192,
		UnusedProtossMarker = 193,
		SpecialZergBeacon = 194,
		SpecialTerranBeacon = 195,
		SpecialProtossBeacon = 196,
		SpecialZergFlagBeacon = 197,
		SpecialTerranFlagBeacon = 198,
		SpecialProtossFlagBeacon = 199,
		SpecialPowerGenerator = 200,
		SpecialOvermindCocoon = 201,
		SpellDarkSwarm = 202,
		SpecialFloorMissileTrap = 203,
		SpecialFloorHatch = 204,
		SpecialUpperLevelDoor = 205,
		SpecialRightUpperLevelDoor = 206,
		SpecialPitDoor = 207,
		SpecialRightPitDoor = 208,
		SpecialFloorGunTrap = 209,
		SpecialWallMissileTrap = 210,
		SpecialWallFlameTrap = 211,
		SpecialRightWallMissileTrap = 212,
		SpecialRightWallFlameTrap = 213,
		SpecialStartLocation = 214,
		PowerupFlag = 215,
		PowerupYoungChrysalis = 216,
		PowerupPsiEmitter = 217,
		PowerupDataDisk = 218,
		PowerupKhaydarinCrystal = 219,
		PowerupMineralClusterType1 = 220,
		PowerupMineralClusterType2 = 221,
		PowerupProtossGasOrbType1 = 222,
		PowerupProtossGasOrbType2 = 223,
		PowerupZergGasSacType1 = 224,
		PowerupZergGasSacType2 = 225,
		PowerupTerranGasTankType1 = 226,
		PowerupTerranGasTankType2 = 227,
		None = 228,
		kCount = None
	};

	enum class OrderType : int32_t {
		Die = 0,
		Stop = 1,
		Guard = 2,
		PlayerGuard = 3,
		TurretGuard = 4,
		BunkerGuard = 5,
		Move = 6,
		ReaverStop = 7,
		AttackDefault = 8,
		MoveToAttack = 9,
		AttackUnit = 10,
		AttackFixedRange = 11,
		AttackTile = 12,
		Hover = 13,
		AttackMove = 14,
		InfestedCommandCenter = 15,
		UnusedNothing = 16,
		UnusedPowerup = 17,
		TowerGuard = 18,
		TowerAttack = 19,
		SpiderMine = 20,
		StayInRange = 21,
		TurretAttack = 22,
		Nothing = 23,
		NothingWait = 24,
		DroneStartBuild = 25,
		DroneBuild = 26,
		CastInfestation = 27,
		MoveToInfest = 28,
		InfestingCommandCenter = 29,
		PlaceBuilding = 30,
		PlaceProtossBuilding = 31,
		CreateProtossBuilding = 32,
		ConstructingBuilding = 33,
		Repair = 34,
		MoveToRepair = 35,
		PlaceAddon = 36,
		BuildAddon = 37,
		Train = 38,
		RallyPointUnit = 39,
		RallyPointTile = 40,
		ZergBirth = 41,
		ZergUnitMorph = 42,
		ZergBuildingMorph = 43,
		IncompleteBuilding = 44,
		IncompleteMorphing = 45,
		BuildNydusExit = 46,
		EnterNydusCanal = 47,
		IncompleteWarping = 48,
		Follow = 49,
		Carrier = 50,
		ReaverCarrierMove = 51,
		CarrierStop = 52,
		CarrierAttack = 53,
		CarrierMoveToAttack = 54,
		CarrierIgnore2 = 55,
		CarrierFight = 56,
		CarrierHoldPosition = 57,
		Reaver = 58,
		ReaverAttack = 59,
		ReaverMoveToAttack = 60,
		ReaverFight = 61,
		ReaverHoldPosition = 62,
		TrainFighter = 63,
		InterceptorAttack = 64,
		ScarabAttack = 65,
		RechargeShieldsUnit = 66,
		RechargeShieldsBattery = 67,
		ShieldBattery = 68,
		InterceptorReturn = 69,
		DroneLand = 70,
		BuildingLand = 71,
		BuildingLiftoff = 72,
		DroneLiftOff = 73,
		LiftingOff = 74,
		ResearchTech = 75,
		Upgrade = 76,
		Larva = 77,
		SpawningLarva = 78,
		Harvest1 = 79,
		Harvest2 = 80,
		MoveToGas = 81,
		WaitForGas = 82,
		HarvestGas = 83,
		ReturnGas = 84,
		MoveToMinerals = 85,
		WaitForMinerals = 86,
		MiningMinerals = 87,
		GatheringInterrupted = 88,
		GatherWaitInterrupted = 89,
		ReturnMinerals = 90,
		RechargeShieldsUnitRemoveOverlay = 91,
		EnterTransport = 92,
		PickupIdle = 93,
		PickupTransport = 94,
		PickupBunker = 95,
		Pickup4 = 96,
		PowerupIdle = 97,
		Sieging = 98,
		Unsieging = 99,
		WatchTarget = 100,
		InitCreepGrowth = 101,
		SpreadCreep = 102,
		StoppingCreepGrowth = 103,
		GuardianAspect = 104,
		ArchonWarp = 105,
		CompletingArchonSummon = 106,
		HoldPosition = 107,
		QueenHoldPosition = 108,
		Cloak = 109,
		Decloak = 110,
		Unload = 111,
		MoveUnload = 112,
		FireYamatoGun = 113,
		MoveToFireYamatoGun = 114,
		CastLockdown = 115,
		Burrowing = 116,
		Burrowed = 117,
		Unburrowing = 118,
		CastDarkSwarm = 119,
		CastParasite = 120,
		CastSpawnBroodlings = 121,
		CastEMPShockwave = 122,
		NukeWait = 123,
		NukeTrain = 124,
		NukeLaunch = 125,
		NukePaint = 126,
		NukeUnit = 127,
		CastNuclearStrike = 128,
		NukeTrack = 129,
		InitializeArbiter = 130,
		CloakNearbyUnits = 131,
		PlaceMine = 132,
		RightClickAction = 133,
		SuicideUnit = 134,
		SuicideLocation = 135,
		SuicideHoldPosition = 136,
		CastRecall = 137,
		Teleport = 138,
		CastScannerSweep = 139,
		Scanner = 140,
		CastDefensiveMatrix = 141,
		CastPsionicStorm = 142,
		CastIrradiate = 143,
		CastPlague = 144,
		CastConsume = 145,
		CastEnsnare = 146,
		CastStasisField = 147,
		CastHallucination = 148,
		Hallucination2 = 149,
		ResetCollision = 150,
		ResetHarvestCollision = 151,
		Patrol = 152,
		CTFCOPInit = 153,
		CTFCOPStarted = 154,
		CTFCOP2 = 155,
		ComputerAI = 156,
		AtkMoveEP = 157,
		HarassMove = 158,
		AIPatrol = 159,
		GuardPost = 160,
		RescuePassive = 161,
		Neutral = 162,
		ComputerReturn = 163,
		InitializePsiProvider = 164,
		SelfDestructing = 165,
		Critter = 166,
		HiddenGun = 167,
		OpenDoor = 168,
		CloseDoor = 169,
		HideTrap = 170,
		RevealTrap = 171,
		EnableDoodad = 172,
		DisableDoodad = 173,
		WarpIn = 174,
		MedicIdle = 175,
		MedicHeal = 176,
		HealMove = 177,
		MedicHoldPosition = 178,
		MedicHealToIdle = 179,
		CastRestoration = 180,
		CastDisruptionWeb = 181,
		CastMindControl = 182,
		DarkArchonMeld = 183,
		CastFeedback = 184,
		CastOpticalFlare = 185,
		CastMaelstrom = 186,
		JunkYardDog = 187,
		Fatal = 188,
		None = 189,
		kCount = None
	};

}


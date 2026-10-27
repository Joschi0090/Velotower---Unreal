#pragma once

#include "CoreMinimal.h"
#include "VelocityTowerTypes.generated.h"

UENUM(BlueprintType)
enum class EVTBootFlowState : uint8
{
	EngineSplash UMETA(DisplayName = "Unreal Engine 5.8 Splash & Loading"),
	TeaserIntro UMETA(DisplayName = "3D Cinematic Teaser Intro"),
	AccountGate UMETA(DisplayName = "Account Login & Guest Gate"),
	Ready UMETA(DisplayName = "Main Lobby & Gameplay")
};

UENUM(BlueprintType)
enum class EVTGameState : uint8
{
	Menu UMETA(DisplayName = "Main Menu"),
	Play UMETA(DisplayName = "In Game"),
	GameOver UMETA(DisplayName = "Game Over")
};

UENUM(BlueprintType)
enum class EVTControlMode : uint8
{
	VirtualJoystick UMETA(DisplayName = "Virtueller Joystick + Jump"),
	ClassicButtons UMETA(DisplayName = "Klassisches D-Pad + Jump"),
	OneThumbSlide UMETA(DisplayName = "1-Daumen Slide & Release"),
	FullscreenSplit UMETA(DisplayName = "Fullscreen Split-Touch"),
	SwipeGestures UMETA(DisplayName = "Swipe & Flick Gesten"),
	AutoBounce UMETA(DisplayName = "Auto-Bounce (Auto-Sprung)")
};

UENUM(BlueprintType)
enum class EVTGameModeType : uint8
{
	Arcade UMETA(DisplayName = "Arcade (Classic Climb)"),
	Hyper UMETA(DisplayName = "Hyper (1.25x Turbo)"),
	Blitz60s UMETA(DisplayName = "Blitz 60s (Time Attack)"),
	Zen UMETA(DisplayName = "Zen (No Death Cam)"),
	Overdrive2x UMETA(DisplayName = "Overdrive 2x Fever"),
	Rival1v1 UMETA(DisplayName = "1v1 Rival Duel")
};

UENUM(BlueprintType)
enum class EVTClimberId : uint8
{
	KidFlex UMETA(DisplayName = "01 // KID FLEX"),
	DiscoDave UMETA(DisplayName = "02 // DISCO DAVE"),
	PixelPenny UMETA(DisplayName = "03 // PIXEL PENNY"),
	FrostRunner UMETA(DisplayName = "04 // FROST RUNNER"),
	NeonVortex UMETA(DisplayName = "05 // NEON VORTEX"),
	CyberNinja UMETA(DisplayName = "06 // CYBER NINJA"),
	AlpenKing UMETA(DisplayName = "07 // ALPEN KING"),
	BlazeStriker UMETA(DisplayName = "08 // BLAZE STRIKER"),
	ValkyrieVex UMETA(DisplayName = "09 // VALKYRIE VEX"),
	ToxicViper UMETA(DisplayName = "10 // TOXIC VIPER"),
	AstroNova UMETA(DisplayName = "11 // ASTRO NOVA"),
	MechaTitan UMETA(DisplayName = "12 // MECHA TITAN"),
	PhantomWraith UMETA(DisplayName = "13 // PHANTOM WRAITH"),
	RoninZero UMETA(DisplayName = "14 // RONIN ZERO"),
	ChromeSiren UMETA(DisplayName = "15 // CHROME SIREN"),
	ThunderVolt UMETA(DisplayName = "16 // THUNDER VOLT"),
	MagmaRex UMETA(DisplayName = "17 // MAGMA REX"),
	CelestialOracle UMETA(DisplayName = "18 // CELESTIAL ORACLE"),
	QuantumGlitch UMETA(DisplayName = "19 // QUANTUM GLITCH"),
	ApexEmperor UMETA(DisplayName = "20 // APEX EMPEROR")
};

UENUM(BlueprintType)
enum class EVTPlatformType : uint8
{
	Normal UMETA(DisplayName = "Normal Cyber-Ice"),
	Checkpoint UMETA(DisplayName = "Checkpoint Golden Bridge"),
	Boost UMETA(DisplayName = "Super Launch Boost Pad"),
	MirrorIce UMETA(DisplayName = "Mirror Ice Slide"),
	QuantumPhase UMETA(DisplayName = "Quantum Phase Platform")
};

UENUM(BlueprintType)
enum class EVTItemType : uint8
{
	None UMETA(DisplayName = "None"),
	Core UMETA(DisplayName = "Neon Core (+1)"),
	Magnet UMETA(DisplayName = "Core Magnet (9s)"),
	Spring UMETA(DisplayName = "3x Spring Boots"),
	Freeze UMETA(DisplayName = "Chrono Freeze (6s)"),
	Shield UMETA(DisplayName = "Aegis Shield")
};

UENUM(BlueprintType)
enum class EVTHudPreset : uint8
{
	ArcadeSplit UMETA(DisplayName = "Arcade Split"),
	RightHanded UMETA(DisplayName = "Rechtshaender"),
	LeftHanded UMETA(DisplayName = "Linkshaender"),
	FullscreenZones UMETA(DisplayName = "Fullscreen Zones")
};

UENUM(BlueprintType)
enum class EVTHudStyleMode : uint8
{
	NeonArcade UMETA(DisplayName = "Subtil Cyber-Glass"),
	Minimalist UMETA(DisplayName = "Minimalistisch"),
	Invisible UMETA(DisplayName = "Unsichtbar (Pro)")
};

USTRUCT(BlueprintType)
struct FVTBiomeInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BiomeIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name = TEXT("CRYO NEON ICE");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor TopColor = FLinearColor(0.0f, 0.961f, 0.831f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor BodyColor = FLinearColor(0.008f, 0.518f, 0.780f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor BgColor1 = FLinearColor(0.016f, 0.027f, 0.067f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor BgColor2 = FLinearColor(0.027f, 0.075f, 0.149f, 1.0f);
};

USTRUCT(BlueprintType)
struct FVTClimberInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EVTClimberId Id = EVTClimberId::KidFlex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RosterNumber = 1; // 1..20

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name = TEXT("KID FLEX");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Rarity = TEXT("STARTER");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor RarityColor = FLinearColor(0.0f, 0.961f, 0.831f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Style = TEXT("Streetwear // 120 BPM");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description = TEXT("Der originale Rookie von Neo-Vice. Ausgewogener Rhythmus und perfektes Handling.");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BaseBpm = 120;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CostCores = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor HoodieColor = FLinearColor(0.0f, 0.961f, 0.831f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor PantsColor = FLinearColor(0.118f, 0.161f, 0.231f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ShoesColor = FLinearColor(0.969f, 0.145f, 0.522f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ScarfColor = FLinearColor(0.969f, 0.145f, 0.522f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor VisorColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<float> ScaleNotes;
};

USTRUCT(BlueprintType)
struct FVTTrailInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id = TEXT("cyan");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name = TEXT("CYAN ICE GLOW");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description = TEXT("Klassische Neon-Cyan Spur");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color = FLinearColor(0.0f, 0.961f, 0.831f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CostCores = 0;
};

UENUM(BlueprintType)
enum class EVTGameThemeId : uint8
{
	CyberpunkDefault UMETA(DisplayName = "01 // CYBERPUNK VICE & ICE"),
	RetroSynthwave   UMETA(DisplayName = "02 // RETRO SYNTHWAVE 80S"),
	AncientRuins     UMETA(DisplayName = "03 // ANCIENT MYSTICAL RUINS"),
	DeepSea          UMETA(DisplayName = "04 // BIOLUMINESCENT DEEP SEA"),
	PapercraftToybox UMETA(DisplayName = "05 // HAND-CRAFTED PAPERCRAFT"),
	IndustrialForge  UMETA(DisplayName = "06 // INDUSTRIAL FORGE & LAVA"),
	DeepSpaceOrbit   UMETA(DisplayName = "07 // DEEP SPACE ORBIT"),
	CandyLand        UMETA(DisplayName = "08 // CANDY LAND DESSERT")
};

USTRUCT(BlueprintType)
struct FVTGameThemeInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EVTGameThemeId Id = EVTGameThemeId::CyberpunkDefault;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ThemeNumber = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name = TEXT("CYBERPUNK VICE & ICE");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Subtitle = TEXT("Neon Apex Core");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description = TEXT("Dunkle Megastruktur, leuchtendes Cyan- & Pink-Neon, holografischer Sound-Equalizer.");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor PrimaryColor = FLinearColor(0.0f, 0.961f, 0.831f, 1.0f); // Top Deck & Rails

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor SecondaryColor = FLinearColor(0.02f, 0.52f, 0.78f, 1.0f); // Accent & Body

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor WallBaseColor = FLinearColor(0.03f, 0.05f, 0.12f, 1.0f); // Pillars & Backplate

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor WallEmissiveColor = FLinearColor(0.0f, 0.961f, 0.831f, 1.0f); // Rails & Runes

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor LightColor = FLinearColor(0.0f, 0.961f, 0.831f, 1.0f); // Tower Main Light

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LightIntensity = 18000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor EqualizerColor = FLinearColor(0.0f, 0.961f, 0.831f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CostCores = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUnlockedInitially = true;
};

USTRUCT(BlueprintType)
struct FVTHudConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EVTHudPreset Preset = EVTHudPreset::ArcadeSplit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SizePct = 100; // 65..150

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OpacityPct = 42; // Subtil als Standard (15..95)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EVTHudStyleMode StyleMode = EVTHudStyleMode::NeonArcade;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BottomOffsetPx = 28; // 0..240

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 HorizontalSpacingPx = 22; // 0..180

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LeftClusterOffsetX = 24.0f; // Button LINKS X (from left)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LeftClusterOffsetY = 24.0f; // Button LINKS Y (from bottom)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RightSteerOffsetX = 108.0f; // Button RECHTS X (from left)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RightSteerOffsetY = 24.0f; // Button RECHTS Y (from bottom)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RightClusterOffsetX = 24.0f; // Button JUMP X (from right)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RightClusterOffsetY = 24.0f; // Button JUMP Y (from bottom)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSwapLeftRight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bJumpGuide = true;
};

USTRUCT(BlueprintType)
struct FVTAchievement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RewardCores = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUnlocked = false;
};

USTRUCT(BlueprintType)
struct FVTQuest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Target = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Progress = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RewardCores = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bClaimed = false;
};

USTRUCT(BlueprintType)
struct FVTFriendEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Avatar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxFloor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BestScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Status = TEXT("Online");
};

USTRUCT(BlueprintType)
struct FVTFloorScaling
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpeedMult = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ProgressiveMult = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WidthPct = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CrackDuration = 1.35f;
};

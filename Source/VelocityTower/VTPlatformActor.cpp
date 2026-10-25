#include "VTPlatformActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

AVTPlatformActor::AVTPlatformActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	PlatformMesh->SetupAttachment(SceneRoot);
	PlatformMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlatformMesh->SetCastShadow(true);

	TopNeonTrimMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopNeonTrimMesh"));
	TopNeonTrimMesh->SetupAttachment(SceneRoot);
	TopNeonTrimMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TopNeonTrimMesh->SetCastShadow(false);

	CollectibleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CollectibleMesh"));
	CollectibleMesh->SetupAttachment(SceneRoot);
	CollectibleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollectibleMesh->SetVisibility(false);

	NeonLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("NeonLight"));
	NeonLight->SetupAttachment(SceneRoot);
	NeonLight->SetIntensity(2800.0f);
	NeonLight->SetAttenuationRadius(340.0f);
	NeonLight->SetCastShadows(false);

	FloorLabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("FloorLabelText"));
	FloorLabelText->SetupAttachment(SceneRoot);
	FloorLabelText->SetHorizontalAlignment(EHTA_Center);
	FloorLabelText->SetVerticalAlignment(EVRTA_TextCenter);
	FloorLabelText->SetWorldSize(24.0f);
	FloorLabelText->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	FloorLabelText->SetVisibility(false);

	// Load fallback engine cube/sphere or custom imported Blender meshes if available
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CustomNormalFinder(TEXT("/Game/VelocityTower/Meshes/SM_Platform_Normal.SM_Platform_Normal"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EngineCubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CustomCoreFinder(TEXT("/Game/VelocityTower/Meshes/SM_Collectible_Core.SM_Collectible_Core"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EngineSphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	if (CustomNormalFinder.Succeeded())
	{
		PlatformMesh->SetStaticMesh(CustomNormalFinder.Object);
	}
	else if (EngineCubeFinder.Succeeded())
	{
		PlatformMesh->SetStaticMesh(EngineCubeFinder.Object);
	}

	if (EngineCubeFinder.Succeeded())
	{
		TopNeonTrimMesh->SetStaticMesh(EngineCubeFinder.Object);
	}

	if (CustomCoreFinder.Succeeded())
	{
		CollectibleMesh->SetStaticMesh(CustomCoreFinder.Object);
	}
	else if (EngineSphereFinder.Succeeded())
	{
		CollectibleMesh->SetStaticMesh(EngineSphereFinder.Object);
	}
}

void AVTPlatformActor::InitializePlatform(
	int32 InFloor,
	float InX,
	float InZ,
	float InWidth,
	float InVx,
	EVTPlatformType InPlatType,
	bool bInCheckpoint,
	EVTItemType InItemType,
	const FVTBiomeInfo& InBiome,
	float InCrackDuration)
{
	FloorIndex = InFloor;
	PlatformX = InX;
	PlatformZ = InZ;
	PlatformWidth = InWidth;
	VelocityX = InVx;
	PlatType = InPlatType;
	bIsCheckpoint = bInCheckpoint;
	ItemType = InItemType;
	bItemCollected = (InItemType == EVTItemType::None);
	BiomeInfo = InBiome;
	CrackDuration = FMath::Max(0.4f, InCrackDuration);
	bTouched = false;
	bBroken = false;
	CrackTime = 0.0f;

	// Razor-Sharp Sleek Cyber-Arcade Deck (Crisp 10cm deck thickness, 52cm sleek depth)
	if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		PlatformMesh->SetStaticMesh(CubeMesh);
		TopNeonTrimMesh->SetStaticMesh(CubeMesh);
	}

	// Scale platform along X to match PlatformWidth (base mesh is 100cm wide)
	const float ScaleX = PlatformWidth / 100.0f;
	PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -5.0f));
	PlatformMesh->SetRelativeScale3D(FVector(ScaleX, 0.52f, 0.10f));

	// Vibrant Front Neon Edge Rail
	TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -25.0f, 0.5f));
	TopNeonTrimMesh->SetRelativeScale3D(FVector(ScaleX * 0.99f, 0.05f, 0.035f));

	// Determine Top & Body Colors matching HTML logic
	FLinearColor TopCol = bIsCheckpoint ? FLinearColor(1.0f, 0.78f, 0.05f) : BiomeInfo.TopColor;
	if (PlatType == EVTPlatformType::Boost) TopCol = FLinearColor(1.0f, 0.08f, 0.54f);
	else if (PlatType == EVTPlatformType::MirrorIce) TopCol = FLinearColor(0.0f, 0.96f, 0.88f);
	else if (PlatType == EVTPlatformType::QuantumPhase) TopCol = FLinearColor(0.68f, 0.28f, 1.0f);

	UMaterialInterface* CustomNeonMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VelocityTower/Materials/M_NeonEmissive_Master.M_NeonEmissive_Master"));
	UMaterialInterface* CustomMegastructureMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VelocityTower/Materials/M_TowerMegastructure_Master.M_TowerMegastructure_Master"));
	UMaterialInterface* FallbackMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInterface* DeckMat = CustomMegastructureMat ? CustomMegastructureMat : (CustomNeonMat ? CustomNeonMat : FallbackMat);

	if (DeckMat)
	{
		BodyMID = UMaterialInstanceDynamic::Create(DeckMat, this);
		const FLinearColor ObsidianDeckCol(0.035f, 0.055f, 0.095f, 1.0f);
		BodyMID->SetVectorParameterValue(TEXT("BaseColor"), ObsidianDeckCol);
		BodyMID->SetVectorParameterValue(TEXT("Color"), ObsidianDeckCol);
		BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), TopCol * (bIsCheckpoint ? 3.0f : 1.2f));
		for (int32 MatIdx = 0; MatIdx < PlatformMesh->GetNumMaterials(); ++MatIdx)
		{
			PlatformMesh->SetMaterial(MatIdx, BodyMID);
		}
	}

	if (CustomNeonMat || FallbackMat)
	{
		TrimMID = UMaterialInstanceDynamic::Create(CustomNeonMat ? CustomNeonMat : FallbackMat, this);
		TrimMID->SetVectorParameterValue(TEXT("BaseColor"), TopCol);
		TrimMID->SetVectorParameterValue(TEXT("Color"), TopCol);
		TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), TopCol * (bIsCheckpoint ? 18.0f : 14.0f));
		TopNeonTrimMesh->SetMaterial(0, TrimMID);
	}

	NeonLight->SetLightColor(TopCol);
	NeonLight->SetIntensity(bIsCheckpoint ? 6000.0f : 2800.0f);
	NeonLight->SetRelativeLocation(FVector(0.0f, -35.0f, 10.0f));

	// Floor milestone holographic badge every 10 floors
	if (FloorIndex > 0 && FloorIndex % 10 == 0)
	{
		FloorLabelText->SetText(FText::FromString(FString::Printf(TEXT("FL %d"), FloorIndex)));
		FloorLabelText->SetTextRenderColor(TopCol.ToFColor(true));
		FloorLabelText->SetRelativeLocation(FVector(0.0f, -62.0f, -10.0f));
		FloorLabelText->SetVisibility(true);
	}
	else
	{
		FloorLabelText->SetVisibility(false);
	}

	// Setup Collectible / Power-Up 3D Mesh
	if (ItemType != EVTItemType::None)
	{
		FString ItemMeshPath = (ItemType == EVTItemType::Core)
			? TEXT("/Game/VelocityTower/Meshes/SM_Collectible_Core.SM_Collectible_Core")
			: TEXT("/Game/VelocityTower/Meshes/SM_PowerUp_Orb.SM_PowerUp_Orb");
		if (UStaticMesh* LoadedItemMesh = LoadObject<UStaticMesh>(nullptr, *ItemMeshPath))
		{
			CollectibleMesh->SetStaticMesh(LoadedItemMesh);
		}

		FLinearColor ItemCol = FLinearColor(0.0f, 0.961f, 0.831f);
		if (ItemType == EVTItemType::Magnet) ItemCol = FLinearColor(0.969f, 0.145f, 0.522f);
		else if (ItemType == EVTItemType::Spring) ItemCol = FLinearColor(1.0f, 0.820f, 0.400f);
		else if (ItemType == EVTItemType::Freeze) ItemCol = FLinearColor(0.220f, 0.741f, 0.973f);
		else if (ItemType == EVTItemType::Shield) ItemCol = FLinearColor(0.0f, 0.961f, 0.831f);

		if (CustomNeonMat || FallbackMat)
		{
			ItemMID = UMaterialInstanceDynamic::Create(CustomNeonMat ? CustomNeonMat : FallbackMat, this);
			ItemMID->SetVectorParameterValue(TEXT("BaseColor"), ItemCol);
			ItemMID->SetVectorParameterValue(TEXT("Color"), ItemCol);
			ItemMID->SetVectorParameterValue(TEXT("EmissiveColor"), ItemCol * 16.0f);
			for (int32 MatIdx = 0; MatIdx < CollectibleMesh->GetNumMaterials(); ++MatIdx)
			{
				CollectibleMesh->SetMaterial(MatIdx, ItemMID);
			}
		}

		ItemWorldLocation = FVector(PlatformX, 0.0f, PlatformZ + 38.0f);
		CollectibleMesh->SetWorldLocation(ItemWorldLocation);
		CollectibleMesh->SetRelativeScale3D(FVector(0.95f));
		CollectibleMesh->SetVisibility(true);
	}
	else
	{
		CollectibleMesh->SetVisibility(false);
	}

	SetActorLocation(FVector(PlatformX, 0.0f, PlatformZ));
}

void AVTPlatformActor::TriggerLandingTouch()
{
	if (!bTouched && !bIsCheckpoint)
	{
		bTouched = true;
	}
}

void AVTPlatformActor::ShatterPlatform()
{
	bBroken = true;
	SetActorHiddenInGame(true);
}

void AVTPlatformActor::CollectItem()
{
	bItemCollected = true;
	if (CollectibleMesh)
	{
		CollectibleMesh->SetVisibility(false);
	}
}

bool AVTPlatformActor::IsSolidAtTime(float CurrentTimeSeconds) const
{
	if (bBroken) return false;
	if (PlatType == EVTPlatformType::QuantumPhase)
	{
		return FMath::Sin(CurrentTimeSeconds * 3.5f + float(FloorIndex)) > -0.25f;
	}
	return true;
}

void AVTPlatformActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bBroken) return;

	const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	// Visual crack shake & color shift when touched
	float ShakeOffsetX = 0.0f;
	if (bTouched && !bIsCheckpoint)
	{
		const float DmgRatio = FMath::Clamp(CrackTime / FMath::Max(0.1f, CrackDuration), 0.0f, 1.0f);
		ShakeOffsetX = FMath::RandRange(-1.0f, 1.0f) * DmgRatio * 9.0f;

		const FLinearColor WarnColor = (DmgRatio > 0.62f)
			? FLinearColor(0.969f, 0.145f, 0.522f)
			: FLinearColor(1.0f, 0.745f, 0.043f);

		if (TrimMID)
		{
			TrimMID->SetVectorParameterValue(TEXT("BaseColor"), WarnColor);
			TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), WarnColor * 15.0f);
		}
		if (BodyMID)
		{
			BodyMID->SetScalarParameterValue(TEXT("CrackDamage"), DmgRatio);
		}
		NeonLight->SetLightColor(WarnColor);
	}

	// Quantum Phase Platform pulsing visibility
	if (PlatType == EVTPlatformType::QuantumPhase)
	{
		const bool bSolid = IsSolidAtTime(TimeSec);
		PlatformMesh->SetVisibility(bSolid, false);
		NeonLight->SetIntensity(bSolid ? 3000.0f : 400.0f);
	}

	SetActorLocation(FVector(PlatformX + ShakeOffsetX, 0.0f, PlatformZ));

	// Animate floating 3D Collectible
	if (ItemType != EVTItemType::None && !bItemCollected && CollectibleMesh)
	{
		const float BobZ = FMath::Sin(TimeSec * 6.0f + float(FloorIndex)) * 7.5f;
		CollectibleMesh->SetWorldLocation(FVector(ItemWorldLocation.X, 0.0f, ItemWorldLocation.Z + BobZ));
		CollectibleMesh->AddLocalRotation(FRotator(0.0f, 140.0f * DeltaTime, 35.0f * DeltaTime));
	}
}

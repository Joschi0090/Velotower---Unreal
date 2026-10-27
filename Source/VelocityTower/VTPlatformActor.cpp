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

void AVTPlatformActor::ApplyTheme(const FVTGameThemeInfo& Theme)
{
	CurrentThemeId = Theme.Id;

	FLinearColor TopCol = bIsCheckpoint ? FLinearColor(1.0f, 0.78f, 0.05f) : Theme.PrimaryColor;
	if (PlatType == EVTPlatformType::Boost) TopCol = FLinearColor(1.0f, 0.08f, 0.54f);
	else if (PlatType == EVTPlatformType::MirrorIce) TopCol = Theme.SecondaryColor;
	else if (PlatType == EVTPlatformType::QuantumPhase) TopCol = FLinearColor(0.68f, 0.28f, 1.0f);

	if (BodyMID)
	{
		BodyMID->SetVectorParameterValue(TEXT("BaseColor"), Theme.WallBaseColor);
		BodyMID->SetVectorParameterValue(TEXT("Color"), Theme.WallBaseColor);
		BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), TopCol * (bIsCheckpoint ? 3.0f : 1.2f));
	}
	if (TrimMID)
	{
		TrimMID->SetVectorParameterValue(TEXT("BaseColor"), TopCol);
		TrimMID->SetVectorParameterValue(TEXT("Color"), TopCol);
		TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), TopCol * (bIsCheckpoint ? 18.0f : 14.0f));
	}
	if (NeonLight)
	{
		NeonLight->SetLightColor(TopCol);
	}
	if (FloorLabelText)
	{
		FloorLabelText->SetTextRenderColor(TopCol.ToFColor(true));
	}

	// -------------------------------------------------------------------------
	// Procedural 3D Theme Geometry Sub-Structures (Matching Reference Images!)
	// -------------------------------------------------------------------------
	for (UStaticMeshComponent* Comp : ThemeDecorations)
	{
		if (Comp)
		{
			Comp->DestroyComponent();
		}
	}
	ThemeDecorations.Empty();
	ThemeDecorationMIDs.Empty();

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* ConeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));

	UMaterialInterface* CustomNeonMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VelocityTower/Materials/M_NeonEmissive_Master.M_NeonEmissive_Master"));
	UMaterialInterface* CustomWallMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VelocityTower/Materials/M_TowerMegastructure_Master.M_TowerMegastructure_Master"));
	UMaterialInterface* FallbackMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInterface* BaseMat = CustomNeonMat ? CustomNeonMat : (CustomWallMat ? CustomWallMat : FallbackMat);

	auto AddDeco = [this, BaseMat](UStaticMesh* InMesh, const FVector& RelLoc, const FRotator& RelRot, const FVector& RelScale, const FLinearColor& BaseCol, const FLinearColor& EmissCol) -> UStaticMeshComponent*
	{
		if (!InMesh) return nullptr;
		UStaticMeshComponent* Deco = NewObject<UStaticMeshComponent>(this);
		Deco->SetupAttachment(SceneRoot);
		Deco->SetStaticMesh(InMesh);
		Deco->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Deco->SetCastShadow(false);
		Deco->SetRelativeLocation(RelLoc);
		Deco->SetRelativeRotation(RelRot);
		Deco->SetRelativeScale3D(RelScale);
		if (BaseMat)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("BaseColor"), BaseCol);
				MID->SetVectorParameterValue(TEXT("Color"), BaseCol);
				MID->SetVectorParameterValue(TEXT("EmissiveColor"), EmissCol);
				Deco->SetMaterial(0, MID);
				ThemeDecorationMIDs.Add(MID);
			}
		}
		Deco->RegisterComponent();
		ThemeDecorations.Add(Deco);
		return Deco;
	};

	// 1. HAND-CRAFTED PAPERCRAFT (Wooden Popsicle Sticks / Eisstiele + Craft Tape)
	if (Theme.Id == EVTGameThemeId::PapercraftToybox)
	{
		TopNeonTrimMesh->SetVisibility(false);
		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -8.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.46f, 0.05f));
		if (BodyMID)
		{
			const FLinearColor CardboardCol(0.72f, 0.54f, 0.35f);
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), CardboardCol);
			BodyMID->SetVectorParameterValue(TEXT("Color"), CardboardCol);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor::Black);
		}

		// 5 colorful wooden craft sticks laid horizontally
		const int32 NumSticks = 5;
		const float StickW = (PlatformWidth * 0.94f) / float(NumSticks);
		const FLinearColor StickCols[5] = {
			FLinearColor(0.96f, 0.22f, 0.26f), // Cherry Red
			FLinearColor(0.98f, 0.82f, 0.12f), // Sun Yellow
			FLinearColor(0.24f, 0.84f, 0.36f), // Fresh Green
			FLinearColor(0.62f, 0.24f, 0.92f), // Grape Purple
			FLinearColor(0.98f, 0.50f, 0.10f)  // Orange
		};
		for (int32 s = 0; s < NumSticks; ++s)
		{
			const float StickX = -PlatformWidth * 0.47f + (float(s) + 0.5f) * StickW;
			AddDeco(CylinderMesh, FVector(StickX, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 90.0f),
				FVector(0.40f, (StickW - 4.0f) / 100.0f, 0.08f), StickCols[s], StickCols[s] * 0.3f);
		}
		// 2 Washi-Tape craft strips on left and right
		const FLinearColor TapeCol(0.88f, 0.94f, 0.98f);
		AddDeco(CubeMesh, FVector(-PlatformWidth * 0.35f, 0.0f, 3.5f), FRotator(0.0f, 15.0f, 0.0f),
			FVector(0.20f, 0.48f, 0.02f), TapeCol, FLinearColor::Black);
		AddDeco(CubeMesh, FVector(PlatformWidth * 0.35f, 0.0f, 3.5f), FRotator(0.0f, -12.0f, 0.0f),
			FVector(0.20f, 0.48f, 0.02f), TapeCol, FLinearColor::Black);
	}
	// 2. CANDY LAND DESSERT (Marshmallow Pillows & Chocolate Wafer Bar)
	else if (Theme.Id == EVTGameThemeId::CandyLand)
	{
		TopNeonTrimMesh->SetVisibility(false);
		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -8.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.48f, 0.08f));
		if (BodyMID)
		{
			const FLinearColor WaferCol(0.35f, 0.18f, 0.10f); // Chocolate Wafer
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), WaferCol);
			BodyMID->SetVectorParameterValue(TEXT("Color"), WaferCol);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor::Black);
		}
		// White cream filling stripe along wafer front
		AddDeco(CubeMesh, FVector(0.0f, -23.0f, -8.0f), FRotator::ZeroRotator,
			FVector(PlatformWidth / 100.0f * 0.98f, 0.04f, 0.025f), FLinearColor(0.98f, 0.96f, 0.90f), FLinearColor(0.98f, 0.96f, 0.90f) * 0.4f);

		// 4 Soft Marshmallow Pillows
		const int32 NumMallows = 4;
		const float MallowW = (PlatformWidth * 0.88f) / float(NumMallows);
		const FLinearColor MallowCols[4] = {
			FLinearColor(1.0f, 0.65f, 0.78f), // Strawberry Pink
			FLinearColor(0.98f, 0.97f, 0.94f), // Vanilla White
			FLinearColor(1.0f, 0.90f, 0.55f), // Lemon Yellow
			FLinearColor(0.65f, 0.95f, 0.88f)  // Mint Cyan
		};
		for (int32 m = 0; m < NumMallows; ++m)
		{
			const float MX = -PlatformWidth * 0.44f + (float(m) + 0.5f) * MallowW;
			AddDeco(CylinderMesh, FVector(MX, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 90.0f),
				FVector(0.38f, (MallowW - 5.0f) / 100.0f, 0.15f), MallowCols[m], MallowCols[m] * 0.6f);
		}
	}
	// 3. ANCIENT MYSTICAL RUINS (Weathered Basalt Stone Slab + Moss + Glowing Rune Glyph)
	else if (Theme.Id == EVTGameThemeId::AncientRuins)
	{
		TopNeonTrimMesh->SetVisibility(true);
		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -8.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.54f, 0.14f));
		if (BodyMID)
		{
			const FLinearColor BasaltCol(0.12f, 0.14f, 0.16f);
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), BasaltCol);
			BodyMID->SetVectorParameterValue(TEXT("Color"), BasaltCol);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor::Black);
		}
		// Moss top surface
		TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		TopNeonTrimMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f * 0.98f, 0.52f, 0.03f));
		if (TrimMID)
		{
			const FLinearColor MossCol(0.08f, 0.38f, 0.14f);
			TrimMID->SetVectorParameterValue(TEXT("BaseColor"), MossCol);
			TrimMID->SetVectorParameterValue(TEXT("Color"), MossCol);
			TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), MossCol * 0.8f);
		}
		// Glowing magical ancient rune seal hovering on the platform!
		AddDeco(CylinderMesh, FVector(0.0f, 0.0f, 2.5f), FRotator::ZeroRotator,
			FVector(0.70f, 0.70f, 0.015f), Theme.WallEmissiveColor, Theme.WallEmissiveColor * 12.0f);
		// Amethyst / magical crystal clusters protruding from the ends
		AddDeco(ConeMesh, FVector(-PlatformWidth * 0.44f, -12.0f, 6.0f), FRotator(15.0f, 25.0f, -20.0f),
			FVector(0.14f, 0.14f, 0.28f), Theme.SecondaryColor, Theme.SecondaryColor * 14.0f);
		AddDeco(ConeMesh, FVector(PlatformWidth * 0.44f, 12.0f, 6.0f), FRotator(-15.0f, -25.0f, 20.0f),
			FVector(0.14f, 0.14f, 0.28f), Theme.SecondaryColor, Theme.SecondaryColor * 14.0f);
	}
	// 4. BIOLUMINESCENT DEEP SEA (Oceanic Coral Ledge + Glowing Algae Rim)
	else if (Theme.Id == EVTGameThemeId::DeepSea)
	{
		TopNeonTrimMesh->SetVisibility(true);
		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -6.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.56f, 0.11f));
		if (BodyMID)
		{
			const FLinearColor AbyssalRock(0.02f, 0.08f, 0.14f);
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), AbyssalRock);
			BodyMID->SetVectorParameterValue(TEXT("Color"), AbyssalRock);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), Theme.PrimaryColor * 1.0f);
		}
		// Undulating glowing coral rim
		TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -24.0f, 0.0f));
		TopNeonTrimMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f * 0.98f, 0.08f, 0.04f));
		if (TrimMID)
		{
			const FLinearColor AlgaeCol(0.0f, 0.94f, 0.84f);
			TrimMID->SetVectorParameterValue(TEXT("BaseColor"), AlgaeCol);
			TrimMID->SetVectorParameterValue(TEXT("Color"), AlgaeCol);
			TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), AlgaeCol * 12.0f);
		}
		// Sea anemones & glowing coral bulbs on corners
		AddDeco(SphereMesh, FVector(-PlatformWidth * 0.42f, 0.0f, -1.0f), FRotator::ZeroRotator,
			FVector(0.24f, 0.35f, 0.12f), FLinearColor(0.85f, 0.20f, 0.75f), FLinearColor(0.85f, 0.20f, 0.75f) * 10.0f);
		AddDeco(SphereMesh, FVector(PlatformWidth * 0.42f, 0.0f, -1.0f), FRotator::ZeroRotator,
			FVector(0.24f, 0.35f, 0.12f), FLinearColor(0.85f, 0.20f, 0.75f), FLinearColor(0.85f, 0.20f, 0.75f) * 10.0f);
	}
	// 5. INDUSTRIAL FORGE & LAVA (Perforated Iron Grate + Molten Heat Core + Chains)
	else if (Theme.Id == EVTGameThemeId::IndustrialForge)
	{
		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -8.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.52f, 0.13f));
		if (BodyMID)
		{
			const FLinearColor CastIron(0.08f, 0.07f, 0.06f);
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), CastIron);
			BodyMID->SetVectorParameterValue(TEXT("Color"), CastIron);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor::Black);
		}
		// Glowing molten heat core inside the grate
		TopNeonTrimMesh->SetVisibility(true);
		TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -2.0f));
		TopNeonTrimMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f * 0.92f, 0.38f, 0.04f));
		if (TrimMID)
		{
			const FLinearColor MoltenCol(1.0f, 0.35f, 0.02f);
			TrimMID->SetVectorParameterValue(TEXT("BaseColor"), MoltenCol);
			TrimMID->SetVectorParameterValue(TEXT("Color"), MoltenCol);
			TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), MoltenCol * 18.0f);
		}
		// 4 dark cast-iron grate bars over the molten core
		for (int32 g = 0; g < 4; ++g)
		{
			const float GX = -PlatformWidth * 0.35f + float(g) * (PlatformWidth * 0.70f / 3.0f);
			AddDeco(CubeMesh, FVector(GX, 0.0f, 0.5f), FRotator::ZeroRotator,
				FVector(0.06f, 0.46f, 0.035f), FLinearColor(0.12f, 0.10f, 0.09f), FLinearColor::Black);
		}
		// Heavy suspension brackets on left & right
		AddDeco(CylinderMesh, FVector(-PlatformWidth * 0.46f, 0.0f, 12.0f), FRotator::ZeroRotator,
			FVector(0.06f, 0.06f, 0.24f), FLinearColor(0.18f, 0.16f, 0.15f), FLinearColor::Black);
		AddDeco(CylinderMesh, FVector(PlatformWidth * 0.46f, 0.0f, 12.0f), FRotator::ZeroRotator,
			FVector(0.06f, 0.06f, 0.24f), FLinearColor(0.18f, 0.16f, 0.15f), FLinearColor::Black);
	}
	// 6. DEEP SPACE ORBIT (Holographic Hard-Light Bridge + Anti-Grav Thrusters)
	else if (Theme.Id == EVTGameThemeId::DeepSpaceOrbit)
	{
		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -5.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.50f, 0.06f));
		if (BodyMID)
		{
			const FLinearColor HardLightCol(0.04f, 0.40f, 0.85f);
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), HardLightCol);
			BodyMID->SetVectorParameterValue(TEXT("Color"), HardLightCol);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), HardLightCol * 6.0f);
		}
		TopNeonTrimMesh->SetVisibility(true);
		TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -24.0f, 0.0f));
		TopNeonTrimMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f * 0.99f, 0.05f, 0.035f));
		if (TrimMID)
		{
			TrimMID->SetVectorParameterValue(TEXT("BaseColor"), Theme.PrimaryColor);
			TrimMID->SetVectorParameterValue(TEXT("Color"), Theme.PrimaryColor);
			TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), Theme.PrimaryColor * 16.0f);
		}
		// Twin anti-grav thruster nozzles underneath emitting blue plasma
		AddDeco(ConeMesh, FVector(-PlatformWidth * 0.32f, 0.0f, -12.0f), FRotator(180.0f, 0.0f, 0.0f),
			FVector(0.12f, 0.12f, 0.16f), Theme.PrimaryColor, Theme.PrimaryColor * 16.0f);
		AddDeco(ConeMesh, FVector(PlatformWidth * 0.32f, 0.0f, -12.0f), FRotator(180.0f, 0.0f, 0.0f),
			FVector(0.12f, 0.12f, 0.16f), Theme.PrimaryColor, Theme.PrimaryColor * 16.0f);
		// Titanium gantry brackets on left & right
		AddDeco(CubeMesh, FVector(-PlatformWidth * 0.48f, 0.0f, -3.0f), FRotator::ZeroRotator,
			FVector(0.08f, 0.54f, 0.10f), FLinearColor(0.85f, 0.88f, 0.95f), FLinearColor::Black);
		AddDeco(CubeMesh, FVector(PlatformWidth * 0.48f, 0.0f, -3.0f), FRotator::ZeroRotator,
			FVector(0.08f, 0.54f, 0.10f), FLinearColor(0.85f, 0.88f, 0.95f), FLinearColor::Black);
	}
	// 7. RETRO SYNTHWAVE 80s (Outrun Neon Wireframe Grid Deck)
	else if (Theme.Id == EVTGameThemeId::RetroSynthwave)
	{
		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -5.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.52f, 0.08f));
		if (BodyMID)
		{
			const FLinearColor SynthDeck(0.03f, 0.01f, 0.06f);
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), SynthDeck);
			BodyMID->SetVectorParameterValue(TEXT("Color"), SynthDeck);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), Theme.PrimaryColor * 2.0f);
		}
		TopNeonTrimMesh->SetVisibility(true);
		TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -25.0f, 0.0f));
		TopNeonTrimMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f * 0.99f, 0.05f, 0.04f));
		if (TrimMID)
		{
			TrimMID->SetVectorParameterValue(TEXT("BaseColor"), Theme.WallEmissiveColor);
			TrimMID->SetVectorParameterValue(TEXT("Color"), Theme.WallEmissiveColor);
			TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), Theme.WallEmissiveColor * 18.0f);
		}
		// Glowing neon grid crossbars
		for (int32 w = 0; w < 3; ++w)
		{
			const float WX = -PlatformWidth * 0.30f + float(w) * (PlatformWidth * 0.60f / 2.0f);
			AddDeco(CubeMesh, FVector(WX, 0.0f, 0.5f), FRotator::ZeroRotator,
				FVector(0.03f, 0.50f, 0.015f), Theme.PrimaryColor, Theme.PrimaryColor * 10.0f);
		}
	}
	// 8. CYBERPUNK VICE & ICE (Default Sleek Cybernetic Deck)
	else
	{
		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -5.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.52f, 0.10f));
		if (BodyMID)
		{
			const FLinearColor CyberDeck(0.035f, 0.055f, 0.095f);
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), CyberDeck);
			BodyMID->SetVectorParameterValue(TEXT("Color"), CyberDeck);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), Theme.PrimaryColor * 1.5f);
		}
		TopNeonTrimMesh->SetVisibility(true);
		TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -25.0f, 0.5f));
		TopNeonTrimMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f * 0.99f, 0.05f, 0.035f));
		if (TrimMID)
		{
			TrimMID->SetVectorParameterValue(TEXT("BaseColor"), Theme.PrimaryColor);
			TrimMID->SetVectorParameterValue(TEXT("Color"), Theme.PrimaryColor);
			TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), Theme.PrimaryColor * 14.0f);
		}
		// Side heatsink fins
		AddDeco(CubeMesh, FVector(-PlatformWidth * 0.49f, 0.0f, -5.0f), FRotator::ZeroRotator,
			FVector(0.04f, 0.54f, 0.08f), Theme.SecondaryColor, Theme.SecondaryColor * 6.0f);
		AddDeco(CubeMesh, FVector(PlatformWidth * 0.49f, 0.0f, -5.0f), FRotator::ZeroRotator,
			FVector(0.04f, 0.54f, 0.08f), Theme.SecondaryColor, Theme.SecondaryColor * 6.0f);
	}
}

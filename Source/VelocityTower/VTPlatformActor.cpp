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
	if (PlatType == EVTPlatformType::QuantumPhase && CurrentThemeId != EVTGameThemeId::PapercraftToybox)
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

	// 1. HAND-CRAFTED PAPERCRAFT 1:1 (Exact 10 Popsicle Stick & Xylophone Constructions from Reference Image)
	if (Theme.Id == EVTGameThemeId::PapercraftToybox)
	{
		TopNeonTrimMesh->SetVisibility(false);
		if (NeonLight) NeonLight->SetIntensity(0.0f);
		if (FloorLabelText) FloorLabelText->SetVisibility(false);

		// Style collectible as a matte 3D Craft Sewing Button instead of a glowing neon orb
		if (CollectibleMesh && ItemMID)
		{
			const FLinearColor CraftItemCol = (ItemType == EVTItemType::Core)
				? FLinearColor(0.92f, 0.22f, 0.24f)
				: FLinearColor(0.22f, 0.52f, 0.88f);
			ItemMID->SetVectorParameterValue(TEXT("BaseColor"), CraftItemCol);
			ItemMID->SetVectorParameterValue(TEXT("Color"), CraftItemCol);
			ItemMID->SetVectorParameterValue(TEXT("EmissiveColor"), CraftItemCol * 0.25f);
		}

		// Hide base dark deck mesh; we build the exact rounded-end wooden popsicle stick (Eisstiel) geometry
		PlatformMesh->SetVisibility(false);

		UMaterialInterface* MatteMat = FallbackMat ? FallbackMat : BaseMat;
		auto AddCraftDeco = [&](UStaticMesh* InMesh, const FVector& RelLoc, const FRotator& RelRot, const FVector& RelScale, const FLinearColor& Col) -> UStaticMeshComponent*
		{
			if (!InMesh) return nullptr;
			UStaticMeshComponent* Deco = NewObject<UStaticMeshComponent>(this);
			Deco->SetupAttachment(SceneRoot);
			Deco->SetStaticMesh(InMesh);
			Deco->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Deco->SetCastShadow(true);
			Deco->SetRelativeLocation(RelLoc);
			Deco->SetRelativeRotation(RelRot);
			Deco->SetRelativeScale3D(RelScale);
			if (MatteMat)
			{
				UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(MatteMat, this);
				if (MID)
				{
					MID->SetVectorParameterValue(TEXT("BaseColor"), Col);
					MID->SetVectorParameterValue(TEXT("Color"), Col);
					MID->SetVectorParameterValue(TEXT("EmissiveColor"), Col * 0.14f);
					Deco->SetMaterial(0, MID);
					ThemeDecorationMIDs.Add(MID);
				}
			}
			Deco->RegisterComponent();
			ThemeDecorations.Add(Deco);
			return Deco;
		};

		const int32 PatternIdx = FMath::Abs(FloorIndex) % 10;
		const FLinearColor WoodEdgeCol(0.76f, 0.58f, 0.38f);
		const FLinearColor MaskingTapeCol(0.91f, 0.84f, 0.68f);

		// Helper to build a horizontal rounded-end Popsicle Stick spanning [LeftX .. RightX] with top surface at Z = 0
		auto BuildPopsicleStick = [&](float LeftX, float RightX, float StickHeightCm, const FLinearColor& PaintCol)
		{
			const float SpanW = FMath::Max(60.0f, RightX - LeftX);
			const float MidX = (LeftX + RightX) * 0.5f;
			const float CapRadius = StickHeightCm * 0.5f;
			const float CoreWidth = FMath::Max(10.0f, SpanW - StickHeightCm);
			const float CenterZ = -CapRadius;
			const float DepthY = 38.0f;

			// 1. Natural wood backing / underside bevel
			AddCraftDeco(CubeMesh, FVector(MidX, 2.0f, CenterZ - 3.0f), FRotator::ZeroRotator,
				FVector(CoreWidth / 100.0f, DepthY / 100.0f, StickHeightCm / 100.0f), WoodEdgeCol);

			// 2. Main painted popsicle blade
			AddCraftDeco(CubeMesh, FVector(MidX, 0.0f, CenterZ), FRotator::ZeroRotator,
				FVector(CoreWidth / 100.0f, DepthY / 100.0f, StickHeightCm / 100.0f), PaintCol);

			// 3. Left & Right rounded popsicle semicircle tips (Roll=90 puts cylinder circle in X-Z screen plane)
			const float TipScaleXY = StickHeightCm / 100.0f;
			const float TipScaleZ = DepthY / 100.0f;
			AddCraftDeco(CylinderMesh, FVector(MidX - CoreWidth * 0.5f, 0.0f, CenterZ), FRotator(0.0f, 0.0f, 90.0f),
				FVector(TipScaleXY, TipScaleXY, TipScaleZ), PaintCol);
			AddCraftDeco(CylinderMesh, FVector(MidX + CoreWidth * 0.5f, 0.0f, CenterZ), FRotator(0.0f, 0.0f, 90.0f),
				FVector(TipScaleXY, TipScaleXY, TipScaleZ), PaintCol);

			// 4. Subtle painted wood-grain highlight & shadow streaks across the front face
			const FLinearColor HighlightCol = FMath::Lerp(PaintCol, FLinearColor::White, 0.14f);
			const FLinearColor GrainShadowCol = PaintCol * 0.82f;
			AddCraftDeco(CubeMesh, FVector(MidX, -19.2f, CenterZ + StickHeightCm * 0.18f), FRotator::ZeroRotator,
				FVector((CoreWidth * 0.86f) / 100.0f, 0.01f, 0.035f), HighlightCol);
			AddCraftDeco(CubeMesh, FVector(MidX, -19.2f, CenterZ - StickHeightCm * 0.16f), FRotator::ZeroRotator,
				FVector((CoreWidth * 0.90f) / 100.0f, 0.01f, 0.025f), GrainShadowCol);
		};

		// 10-Floor 1:1 Reference Layout (Note: Screen Left = +X, Screen Right = -X)
		if (PatternIdx == 0)
		{
			// Floor 0 (Bottom): Deep Ocean Blue Popsicle Stick attached to right wall
			const FLinearColor BlueCol(0.11f, 0.40f, 0.64f);
			BuildPopsicleStick(-PlatformWidth * 0.5f, PlatformWidth * 0.5f, 34.0f, BlueCol);
			AddCraftDeco(CylinderMesh, FVector(-PlatformWidth * 0.36f, 12.0f, 12.0f), FRotator(0.0f, 0.0f, 90.0f),
				FVector(0.26f, 0.26f, 0.20f), FLinearColor(0.82f, 0.58f, 0.14f));
		}
		else if (PatternIdx == 1)
		{
			// Floor 1: Red-Orange Popsicle Stick with masking tape bracket on right wall
			const FLinearColor RedOrangeCol(0.86f, 0.27f, 0.21f);
			BuildPopsicleStick(-PlatformWidth * 0.5f, PlatformWidth * 0.5f, 32.0f, RedOrangeCol);
			AddCraftDeco(CubeMesh, FVector(-PlatformWidth * 0.48f, -8.0f, -14.0f), FRotator(0.0f, 0.0f, -6.0f),
				FVector(0.18f, 0.42f, 0.48f), MaskingTapeCol);
		}
		else if (PatternIdx == 2)
		{
			// Floor 2: Full-width Yellow Popsicle Stick wrapped in center-left with Blue Painter's Tape + Green Tape
			const FLinearColor YellowCol(0.92f, 0.75f, 0.18f);
			const FLinearColor BlueTapeCol(0.28f, 0.40f, 0.76f);
			const FLinearColor BlueTapeGridCol(0.35f, 0.48f, 0.84f);
			const FLinearColor MintTapeCol(0.46f, 0.68f, 0.36f);

			BuildPopsicleStick(-PlatformWidth * 0.5f, PlatformWidth * 0.5f, 32.0f, YellowCol);

			// Tilted mint-green masking tape strip behind the left side (Screen Left = +X)
			AddCraftDeco(CubeMesh, FVector(PlatformWidth * 0.28f, 10.0f, 4.0f), FRotator(0.0f, 0.0f, -22.0f),
				FVector(0.55f, 0.18f, 0.85f), MintTapeCol);

			// Vertical green craft strip glued above right end (Screen Right = -X)
			AddCraftDeco(CubeMesh, FVector(-PlatformWidth * 0.34f, 12.0f, 36.0f), FRotator::ZeroRotator,
				FVector(0.42f, 0.16f, 0.68f), FLinearColor(0.32f, 0.58f, 0.28f));

			// Wide Blue Painter's Tape wrapped around the center-left of the yellow stick
			const float TapeCenterX = PlatformWidth * 0.08f;
			const float TapeW = PlatformWidth * 0.42f;
			AddCraftDeco(CubeMesh, FVector(TapeCenterX, -1.0f, -16.0f), FRotator::ZeroRotator,
				FVector(TapeW / 100.0f, 0.42f, 0.35f), BlueTapeCol);
			for (int32 t = -2; t <= 2; ++t)
			{
				AddCraftDeco(CubeMesh, FVector(TapeCenterX + float(t) * (TapeW * 0.18f), -21.5f, -16.0f), FRotator::ZeroRotator,
					FVector(0.025f, 0.01f, 0.34f), BlueTapeGridCol);
			}
			// Right wall masking tape tab
			AddCraftDeco(CubeMesh, FVector(-PlatformWidth * 0.48f, -8.0f, 6.0f), FRotator(0.0f, 0.0f, 12.0f),
				FVector(0.22f, 0.40f, 0.45f), MaskingTapeCol);
		}
		else if (PatternIdx == 3)
		{
			// Floor 3: Bubblegum Pink / Magenta Popsicle Stick with two cardboard spacer blocks on top at left & right ends
			const FLinearColor PinkCol(0.84f, 0.28f, 0.56f);
			const FLinearColor SpacerTopCol(0.86f, 0.74f, 0.56f);
			const FLinearColor SpacerEdgeCol(0.54f, 0.38f, 0.22f);

			BuildPopsicleStick(-PlatformWidth * 0.5f, PlatformWidth * 0.5f, 22.0f, PinkCol);

			// Left & Right cardboard end-spacers glued on top of the pink stick
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				const float EndX = float(Side) * (PlatformWidth * 0.38f);
				AddCraftDeco(CubeMesh, FVector(EndX, 0.0f, 5.0f), FRotator::ZeroRotator,
					FVector(0.68f, 0.38f, 0.09f), SpacerTopCol);
				AddCraftDeco(CubeMesh, FVector(EndX, -19.5f, 2.0f), FRotator::ZeroRotator,
					FVector(0.68f, 0.02f, 0.04f), SpacerEdgeCol);
			}
		}
		else if (PatternIdx == 4)
		{
			// Floor 4: Thin Green Stick on Left (+X) + 6-Bar Colorful Vertical Popsicle Stick Xylophone on Right (-X)!
			const FLinearColor GreenStickCol(0.26f, 0.68f, 0.30f);
			const float LeftEdgeX = PlatformWidth * 0.5f;
			const float SplitX = -PlatformWidth * 0.02f;

			// 1. Thin green horizontal stick on the left half
			BuildPopsicleStick(SplitX, LeftEdgeX, 16.0f, GreenStickCol);

			// Left wall masking tape bracket
			AddCraftDeco(CubeMesh, FVector(LeftEdgeX - 8.0f, -6.0f, -6.0f), FRotator::ZeroRotator,
				FVector(0.24f, 0.42f, 0.65f), MaskingTapeCol);

			// Center horizontal wooden twin connector tabs where green stick meets the xylophone
			AddCraftDeco(CubeMesh, FVector(SplitX + 12.0f, 4.0f, 6.0f), FRotator::ZeroRotator,
				FVector(0.48f, 0.32f, 0.11f), MaskingTapeCol);
			AddCraftDeco(CubeMesh, FVector(SplitX + 12.0f, 4.0f, -18.0f), FRotator::ZeroRotator,
				FVector(0.48f, 0.32f, 0.11f), MaskingTapeCol);

			// 2. 6 Vertical Colorful Popsicle Sticks (Xylophone) on the right half (from SplitX to -PlatformWidth*0.5f)
			// Order from Center-Left to Right in reference image: Red, Green, Cyan-Blue, Red, Yellow, Blue (ascending in height!)
			const FLinearColor XyloCols[6] = {
				FLinearColor(0.88f, 0.22f, 0.24f), // 1. Red (shortest)
				FLinearColor(0.26f, 0.68f, 0.30f), // 2. Green
				FLinearColor(0.16f, 0.58f, 0.82f), // 3. Cyan-Blue
				FLinearColor(0.88f, 0.22f, 0.24f), // 4. Red
				FLinearColor(0.95f, 0.78f, 0.16f), // 5. Yellow
				FLinearColor(0.14f, 0.52f, 0.82f)  // 6. Blue (tallest)
			};
			const float BarHeights[6] = { 96.0f, 114.0f, 130.0f, 148.0f, 170.0f, 200.0f };
			const float BarBottomZ = -58.0f;
			const float RightSpan = PlatformWidth * 0.46f;
			const float BarSpacing = RightSpan / 6.0f;
			const float BarWidth = BarSpacing * 0.88f;

			for (int32 b = 0; b < 6; ++b)
			{
				// Moving from SplitX toward -PlatformWidth*0.5f (Screen Right)
				const float BarX = SplitX - (float(b) + 0.5f) * BarSpacing;
				const float TotalH = BarHeights[b];
				const float CoreH = FMath::Max(20.0f, TotalH - BarWidth);
				const float BarMidZ = BarBottomZ + TotalH * 0.5f;

				// Main vertical stick blade
				AddCraftDeco(CubeMesh, FVector(BarX, -4.0f, BarMidZ), FRotator::ZeroRotator,
					FVector(BarWidth / 100.0f, 0.38f, CoreH / 100.0f), XyloCols[b]);

				// Top & Bottom rounded popsicle ends
				AddCraftDeco(CylinderMesh, FVector(BarX, -4.0f, BarMidZ + CoreH * 0.5f), FRotator(0.0f, 0.0f, 90.0f),
					FVector(BarWidth / 100.0f, BarWidth / 100.0f, 0.38f), XyloCols[b]);
				AddCraftDeco(CylinderMesh, FVector(BarX, -4.0f, BarMidZ - CoreH * 0.5f), FRotator(0.0f, 0.0f, 90.0f),
					FVector(BarWidth / 100.0f, BarWidth / 100.0f, 0.38f), XyloCols[b]);

				// Unpainted wooden stick tips poking out below bars 1..4 (matching reference image 1:1!)
				if (b >= 1 && b <= 4)
				{
					AddCraftDeco(CubeMesh, FVector(BarX, 4.0f, BarBottomZ - 16.0f), FRotator::ZeroRotator,
						FVector((BarWidth * 0.48f) / 100.0f, 0.24f, 0.34f), MaskingTapeCol);
					AddCraftDeco(CylinderMesh, FVector(BarX, 4.0f, BarBottomZ - 32.0f), FRotator(0.0f, 0.0f, 90.0f),
						FVector((BarWidth * 0.48f) / 100.0f, (BarWidth * 0.48f) / 100.0f, 0.24f), MaskingTapeCol);
				}
			}
			// Right wall masking tape tab behind the tallest blue stick
			AddCraftDeco(CubeMesh, FVector(SplitX - 5.5f * BarSpacing - 18.0f, 6.0f, BarBottomZ + 12.0f), FRotator::ZeroRotator,
				FVector(0.36f, 0.36f, 0.32f), MaskingTapeCol);
		}
		else if (PatternIdx == 5)
		{
			// Floor 5: Tangerine Orange Popsicle Stick taped to left wall
			const FLinearColor OrangeCol(0.86f, 0.36f, 0.15f);
			BuildPopsicleStick(-PlatformWidth * 0.5f, PlatformWidth * 0.5f, 24.0f, OrangeCol);
			AddCraftDeco(CubeMesh, FVector(PlatformWidth * 0.48f, -6.0f, -10.0f), FRotator::ZeroRotator,
				FVector(0.22f, 0.42f, 0.48f), MaskingTapeCol);
		}
		else if (PatternIdx == 6)
		{
			// Floor 6: Royal Purple Popsicle Stick taped to right wall
			const FLinearColor PurpleCol(0.42f, 0.23f, 0.66f);
			BuildPopsicleStick(-PlatformWidth * 0.5f, PlatformWidth * 0.5f, 26.0f, PurpleCol);
			AddCraftDeco(CubeMesh, FVector(-PlatformWidth * 0.48f, -6.0f, -8.0f), FRotator::ZeroRotator,
				FVector(0.22f, 0.42f, 0.48f), MaskingTapeCol);
		}
		else if (PatternIdx == 7)
		{
			// Floor 7: Forest Green Popsicle Stick taped to left wall
			const FLinearColor ForestGreenCol(0.22f, 0.56f, 0.24f);
			BuildPopsicleStick(-PlatformWidth * 0.5f, PlatformWidth * 0.5f, 30.0f, ForestGreenCol);
			AddCraftDeco(CubeMesh, FVector(PlatformWidth * 0.48f, -6.0f, -12.0f), FRotator::ZeroRotator,
				FVector(0.22f, 0.42f, 0.50f), MaskingTapeCol);
		}
		else if (PatternIdx == 8)
		{
			// Floor 8: Warm Golden Yellow Popsicle Stick taped to left wall
			const FLinearColor GoldenYellowCol(0.91f, 0.72f, 0.15f);
			BuildPopsicleStick(-PlatformWidth * 0.5f, PlatformWidth * 0.5f, 32.0f, GoldenYellowCol);
			AddCraftDeco(CubeMesh, FVector(PlatformWidth * 0.48f, -6.0f, -12.0f), FRotator::ZeroRotator,
				FVector(0.22f, 0.42f, 0.50f), MaskingTapeCol);
		}
		else
		{
			// Floor 9 (Top): Warm Pink-Red Rounded Popsicle Stick in upper center
			const FLinearColor PinkRedCol(0.86f, 0.28f, 0.36f);
			BuildPopsicleStick(-PlatformWidth * 0.5f, PlatformWidth * 0.5f, 34.0f, PinkRedCol);
		}
	}
	// 2. CANDY LAND DESSERT 1:1 (Gummy Cubes, Striped Licorice Twists, 3-Piece Marshmallows, Chocolate Wafer Cake & Frosted Biscuits)
	else if (Theme.Id == EVTGameThemeId::CandyLand)
	{
		TopNeonTrimMesh->SetVisibility(false);
		PlatformMesh->SetVisibility(false);
		if (NeonLight) NeonLight->SetIntensity(0.0f);
		if (FloorLabelText) FloorLabelText->SetVisibility(false);

		const int32 PatternIdx = FMath::Abs(FloorIndex) % 10;
		auto AddCandyDeco = [&](UStaticMesh* InMesh, const FVector& RelLoc, const FRotator& RelRot, const FVector& RelScale, const FLinearColor& Col)
		{
			return AddDeco(InMesh, RelLoc, RelRot, RelScale, Col, Col * 0.22f);
		};

		if (PatternIdx == 0)
		{
			// Floor 0: Two big translucent pink sugar-coated gummy cubes side by side
			const float CubeW = (PlatformWidth * 0.44f);
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				const float CX = float(Side) * (CubeW * 0.56f);
				AddCandyDeco(CubeMesh, FVector(CX, 0.0f, -24.0f), FRotator::ZeroRotator,
					FVector(CubeW / 100.0f, 0.46f, 0.48f), FLinearColor(0.98f, 0.36f, 0.65f));
				AddCandyDeco(CubeMesh, FVector(CX, -23.5f, -24.0f), FRotator::ZeroRotator,
					FVector((CubeW * 0.84f) / 100.0f, 0.02f, 0.38f), FLinearColor(1.0f, 0.58f, 0.80f));
			}
		}
		else if (PatternIdx == 1 || PatternIdx == 2)
		{
			// Floors 1 & 2: Pink-and-dark-chocolate diagonal-striped twisted candy stick / licorice rod
			const int32 Segs = 8;
			const float SegW = PlatformWidth / float(Segs);
			for (int32 s = 0; s < Segs; ++s)
			{
				const float SX = -PlatformWidth * 0.5f + (float(s) + 0.5f) * SegW;
				const FLinearColor StripeCol = (s % 2 == 0) ? FLinearColor(0.24f, 0.12f, 0.16f) : FLinearColor(0.98f, 0.52f, 0.72f);
				AddCandyDeco(CylinderMesh, FVector(SX, 0.0f, -12.0f), FRotator(0.0f, 0.0f, 90.0f),
					FVector(0.26f, 0.26f, (SegW + 2.0f) / 100.0f), StripeCol);
			}
		}
		else if (PatternIdx == 3)
		{
			// Floor 3: Split platform (3-piece pastel marshmallow on Left +X, layered chocolate wafer cake on Right -X)
			const float HalfW = PlatformWidth * 0.46f;
			const FLinearColor MallowCols[3] = {
				FLinearColor(1.0f, 0.64f, 0.78f),
				FLinearColor(1.0f, 0.92f, 0.60f),
				FLinearColor(0.99f, 0.98f, 0.96f)
			};
			const float MW = HalfW / 3.0f;
			for (int32 m = 0; m < 3; ++m)
			{
				const float MX = PlatformWidth * 0.04f + (float(m) + 0.5f) * MW;
				AddCandyDeco(CylinderMesh, FVector(MX, 0.0f, -14.0f), FRotator::ZeroRotator,
					FVector((MW * 0.92f) / 100.0f, 0.42f, 0.28f), MallowCols[m]);
			}
			// Chocolate wafer bar on right half (-X)
			const float WaferCenter = -PlatformWidth * 0.26f;
			AddCandyDeco(CubeMesh, FVector(WaferCenter, 0.0f, -14.0f), FRotator::ZeroRotator,
				FVector(HalfW / 100.0f, 0.44f, 0.28f), FLinearColor(0.36f, 0.17f, 0.10f));
			AddCandyDeco(CubeMesh, FVector(WaferCenter, -22.5f, -10.0f), FRotator::ZeroRotator,
				FVector(HalfW / 100.0f, 0.02f, 0.05f), FLinearColor(0.99f, 0.94f, 0.88f));
			AddCandyDeco(CubeMesh, FVector(WaferCenter, -22.5f, -18.0f), FRotator::ZeroRotator,
				FVector(HalfW / 100.0f, 0.02f, 0.05f), FLinearColor(1.0f, 0.68f, 0.80f));
		}
		else if (PatternIdx == 5 || PatternIdx == 7 || PatternIdx == 9)
		{
			// Frosted Pink Biscuit Bar with thick white dripping vanilla icing
			AddCandyDeco(CubeMesh, FVector(0.0f, 0.0f, -14.0f), FRotator::ZeroRotator,
				FVector(PlatformWidth / 100.0f, 0.44f, 0.18f), FLinearColor(0.96f, 0.54f, 0.72f));
			AddCandyDeco(CubeMesh, FVector(0.0f, -2.0f, -3.0f), FRotator::ZeroRotator,
				FVector((PlatformWidth * 1.02f) / 100.0f, 0.48f, 0.09f), FLinearColor(0.99f, 0.98f, 0.96f));
			for (int32 d = -2; d <= 2; ++d)
			{
				AddCandyDeco(SphereMesh, FVector(float(d) * (PlatformWidth * 0.18f), -22.0f, -8.0f), FRotator::ZeroRotator,
					FVector(0.18f, 0.12f, 0.22f), FLinearColor(0.99f, 0.98f, 0.96f));
			}
		}
		else
		{
			// 3 Chubby Cylindrical Pastel Marshmallows (Strawberry Pink, Cream Yellow, Vanilla White)
			const int32 NumMallows = 3;
			const float MallowW = (PlatformWidth * 0.94f) / float(NumMallows);
			const FLinearColor MallowCols[3] = {
				FLinearColor(1.0f, 0.65f, 0.78f),
				FLinearColor(1.0f, 0.92f, 0.62f),
				FLinearColor(0.98f, 0.97f, 0.95f)
			};
			for (int32 m = 0; m < NumMallows; ++m)
			{
				const float MX = -PlatformWidth * 0.47f + (float(m) + 0.5f) * MallowW;
				AddCandyDeco(CylinderMesh, FVector(MX, 0.0f, -15.0f), FRotator::ZeroRotator,
					FVector((MallowW - 4.0f) / 100.0f, 0.46f, 0.30f), MallowCols[m]);
			}
		}
	}
	// 3. ANCIENT MYSTICAL RUINS 1:1 (Carved Stone Slabs, Rune Staircases & Swirling Cyan/Gold Arcane Wisps)
	else if (Theme.Id == EVTGameThemeId::AncientRuins)
	{
		const int32 PatternIdx = FMath::Abs(FloorIndex) % 10;
		const FLinearColor StoneBase(0.24f, 0.26f, 0.30f);
		const FLinearColor StoneHighlight(0.34f, 0.37f, 0.42f);
		const FLinearColor CyanMagic(0.0f, 0.88f, 1.0f);
		const FLinearColor GoldMagic(1.0f, 0.72f, 0.12f);

		TopNeonTrimMesh->SetVisibility(false);
		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -14.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.54f, 0.28f));
		if (BodyMID)
		{
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), StoneBase);
			BodyMID->SetVectorParameterValue(TEXT("Color"), StoneBase);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor::Black);
		}
		if (FloorLabelText) FloorLabelText->SetVisibility(false);

		// Chiseled stone block segments along the front face
		const int32 NumBlocks = 4;
		const float BlockW = (PlatformWidth * 0.96f) / float(NumBlocks);
		for (int32 b = 0; b < NumBlocks; ++b)
		{
			const float BX = -PlatformWidth * 0.48f + (float(b) + 0.5f) * BlockW;
			AddDeco(CubeMesh, FVector(BX, -26.0f, -13.0f), FRotator::ZeroRotator,
				FVector((BlockW - 5.0f) / 100.0f, 0.04f, 0.24f), StoneHighlight, FLinearColor::Black);
		}

		if (PatternIdx == 0)
		{
			// Floor 0: 4-tier stone staircase leading up to the slab with glowing multicolor runes on the risers!
			const FLinearColor RuneCols[4] = {
				FLinearColor(0.95f, 0.30f, 0.75f),
				FLinearColor(1.0f, 0.65f, 0.15f),
				FLinearColor(0.20f, 0.95f, 0.55f),
				FLinearColor(0.0f, 0.88f, 1.0f)
			};
			for (int32 s = 0; s < 4; ++s)
			{
				const float StepZ = -28.0f - float(s) * 26.0f;
				const float StepW = PlatformWidth * (0.72f - float(s) * 0.04f);
				AddDeco(CubeMesh, FVector(-PlatformWidth * 0.06f, -6.0f, StepZ), FRotator::ZeroRotator,
					FVector(StepW / 100.0f, 0.44f, 0.22f), StoneBase, FLinearColor::Black);
				// Glowing rune glyph on the front riser
				AddDeco(CubeMesh, FVector(-PlatformWidth * 0.06f + float(s % 2 - 0.5f) * 35.0f, -29.0f, StepZ), FRotator(0.0f, 0.0f, 18.0f),
					FVector(0.24f, 0.02f, 0.08f), RuneCols[s], RuneCols[s] * 14.0f);
			}
		}
		else if (PatternIdx == 2)
		{
			// Floor 2: 5-step ancient stone staircase on the left half (+X) + glowing cyan magic wisps
			for (int32 s = 1; s <= 5; ++s)
			{
				const float StepW = PlatformWidth * 0.46f;
				const float StepX = PlatformWidth * 0.22f;
				const float StepZ = float(s) * 16.0f - 8.0f;
				AddDeco(CubeMesh, FVector(StepX, 4.0f, StepZ), FRotator::ZeroRotator,
					FVector((StepW - float(s) * 8.0f) / 100.0f, 0.40f, 0.14f), StoneHighlight, FLinearColor::Black);
			}
			AddDeco(CylinderMesh, FVector(PlatformWidth * 0.25f, -22.0f, -22.0f), FRotator(0.0f, 0.0f, 84.0f),
				FVector(0.06f, 0.06f, (PlatformWidth * 0.5f) / 100.0f), CyanMagic, CyanMagic * 16.0f);
		}
		else if (PatternIdx == 1 || PatternIdx == 3 || PatternIdx == 4 || PatternIdx == 5 || PatternIdx == 7)
		{
			// Swirling Cyan & Golden-Amber Magical Energy Ribbons wrapping the stone slab
			const bool bCyanPrimary = (PatternIdx != 3);
			const FLinearColor Wisps1 = bCyanPrimary ? CyanMagic : GoldMagic;
			const FLinearColor Wisps2 = bCyanPrimary ? GoldMagic : CyanMagic;
			AddDeco(CylinderMesh, FVector(0.0f, -26.0f, -4.0f), FRotator(6.0f, 0.0f, 86.0f),
				FVector(0.055f, 0.055f, (PlatformWidth * 0.88f) / 100.0f), Wisps1, Wisps1 * 18.0f);
			AddDeco(CylinderMesh, FVector(0.0f, -26.0f, -22.0f), FRotator(-5.0f, 0.0f, 94.0f),
				FVector(0.045f, 0.045f, (PlatformWidth * 0.78f) / 100.0f), Wisps2, Wisps2 * 15.0f);
			if (NeonLight)
			{
				NeonLight->SetLightColor(Wisps1);
				NeonLight->SetIntensity(2200.0f);
			}
		}
		else if (PatternIdx == 6)
		{
			// Glowing cyan orb under the right edge of the slab
			AddDeco(SphereMesh, FVector(-PlatformWidth * 0.32f, -12.0f, -28.0f), FRotator::ZeroRotator,
				FVector(0.32f, 0.32f, 0.18f), CyanMagic, CyanMagic * 14.0f);
		}
	}
	// 4. BIOLUMINESCENT DEEP SEA 1:1 (Coral Reefs, Hanging Glowing Jellyfish & Waving Kelp Vines)
	else if (Theme.Id == EVTGameThemeId::DeepSea)
	{
		const int32 PatternIdx = FMath::Abs(FloorIndex) % 10;
		const FLinearColor ReefRock(0.05f, 0.16f, 0.24f);
		const FLinearColor CyanGlow(0.0f, 0.95f, 0.92f);
		const FLinearColor PinkCoral(0.98f, 0.36f, 0.62f);
		const FLinearColor KelpGreen(0.14f, 0.46f, 0.22f);

		TopNeonTrimMesh->SetVisibility(true);
		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -12.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.54f, 0.22f));
		if (BodyMID)
		{
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), ReefRock);
			BodyMID->SetVectorParameterValue(TEXT("Color"), ReefRock);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), CyanGlow * 0.45f);
		}
		TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -22.0f, -1.0f));
		TopNeonTrimMesh->SetRelativeScale3D(FVector((PlatformWidth * 0.96f) / 100.0f, 0.12f, 0.05f));
		if (TrimMID)
		{
			TrimMID->SetVectorParameterValue(TEXT("BaseColor"), CyanGlow);
			TrimMID->SetVectorParameterValue(TEXT("Color"), CyanGlow);
			TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), CyanGlow * 8.0f);
		}
		if (FloorLabelText) FloorLabelText->SetVisibility(false);

		if (PatternIdx == 0 || PatternIdx == 1)
		{
			// Floors 0 & 1: Reef ledge overgrown with tall waving green kelp/seaweed stalks
			for (int32 k = 0; k < 5; ++k)
			{
				const float KX = -PlatformWidth * 0.38f + float(k) * (PlatformWidth * 0.19f);
				AddDeco(CylinderMesh, FVector(KX, -18.0f, -45.0f), FRotator(0.0f, 0.0f, float(k % 2 == 0 ? 12 : -14)),
					FVector(0.08f, 0.08f, 1.15f), KelpGreen, KelpGreen * 0.6f);
			}
		}
		else if (PatternIdx == 4 || PatternIdx == 5)
		{
			// Floors 4 & 5: Pink coral tufts on top + Giant Translucent Glowing Cyan Jellyfish hanging underneath!
			AddDeco(SphereMesh, FVector(0.0f, -14.0f, 6.0f), FRotator::ZeroRotator,
				FVector(0.48f, 0.28f, 0.24f), PinkCoral, PinkCoral * 6.0f);

			const float JellyX = (PatternIdx == 4) ? (PlatformWidth * 0.08f) : (-PlatformWidth * 0.05f);
			// Glowing Jellyfish Bell
			AddDeco(SphereMesh, FVector(JellyX, -10.0f, -48.0f), FRotator::ZeroRotator,
				FVector(0.95f, 0.65f, 0.58f), CyanGlow, CyanGlow * 14.0f);
			// 5 Trailing Bioluminescent Tentacles
			for (int32 t = -2; t <= 2; ++t)
			{
				AddDeco(CylinderMesh, FVector(JellyX + float(t) * 14.0f, -10.0f, -105.0f), FRotator(0.0f, 0.0f, float(t) * 6.0f - 8.0f),
					FVector(0.04f, 0.04f, 1.10f), CyanGlow, CyanGlow * 10.0f);
			}
		}
		else
		{
			// Glowing Pink Acropora Coral & Neon-Green Bioluminescent Polyps
			AddDeco(SphereMesh, FVector(PlatformWidth * 0.34f, -14.0f, 8.0f), FRotator::ZeroRotator,
				FVector(0.42f, 0.28f, 0.32f), PinkCoral, PinkCoral * 6.0f);
			AddDeco(SphereMesh, FVector(-PlatformWidth * 0.30f, -18.0f, -6.0f), FRotator::ZeroRotator,
				FVector(0.36f, 0.24f, 0.16f), FLinearColor(0.0f, 1.0f, 0.65f), FLinearColor(0.0f, 1.0f, 0.65f) * 8.0f);
		}
	}
	// 5. INDUSTRIAL FORGE & LAVA 1:1 (Perforated Steel Grate Box-Girders + Blazing Molten Orange Furnace Cores)
	else if (Theme.Id == EVTGameThemeId::IndustrialForge)
	{
		if (FloorLabelText) FloorLabelText->SetVisibility(false);
		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -12.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.52f, 0.24f));
		if (BodyMID)
		{
			const FLinearColor CastIron(0.11f, 0.08f, 0.07f);
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), CastIron);
			BodyMID->SetVectorParameterValue(TEXT("Color"), CastIron);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(1.0f, 0.28f, 0.02f) * 0.8f);
		}
		// Intense glowing molten orange lattice core along the front & underside
		TopNeonTrimMesh->SetVisibility(true);
		TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -24.0f, -12.0f));
		TopNeonTrimMesh->SetRelativeScale3D(FVector((PlatformWidth * 0.96f) / 100.0f, 0.06f, 0.16f));
		const FLinearColor MoltenCol(1.0f, 0.38f, 0.02f);
		if (TrimMID)
		{
			TrimMID->SetVectorParameterValue(TEXT("BaseColor"), MoltenCol);
			TrimMID->SetVectorParameterValue(TEXT("Color"), MoltenCol);
			TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), MoltenCol * 18.0f);
		}
		// Vertical steel grate ribs over the molten core
		const int32 NumRibs = 7;
		for (int32 g = 0; g < NumRibs; ++g)
		{
			const float GX = -PlatformWidth * 0.44f + float(g) * (PlatformWidth * 0.88f / float(NumRibs - 1));
			AddDeco(CubeMesh, FVector(GX, -26.5f, -12.0f), FRotator::ZeroRotator,
				FVector(0.04f, 0.03f, 0.22f), FLinearColor(0.14f, 0.11f, 0.10f), FLinearColor::Black);
		}
	}
	// 6. DEEP SPACE ORBIT 1:1 (5-Cell Translucent Holographic Solar-Grid Platforms in Cyan & Violet-Magenta)
	else if (Theme.Id == EVTGameThemeId::DeepSpaceOrbit)
	{
		if (FloorLabelText) FloorLabelText->SetVisibility(false);
		const int32 PatternIdx = FMath::Abs(FloorIndex) % 10;
		const bool bVioletGrid = (PatternIdx == 2 || PatternIdx == 4 || PatternIdx == 7 || PatternIdx == 8);
		const FLinearColor HoloCol = bVioletGrid ? FLinearColor(0.82f, 0.26f, 1.0f) : FLinearColor(0.0f, 0.90f, 1.0f);

		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -10.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.48f, 0.18f));
		if (BodyMID)
		{
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), HoloCol * 0.25f);
			BodyMID->SetVectorParameterValue(TEXT("Color"), HoloCol * 0.25f);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), HoloCol * 3.2f);
		}
		// Top & Bottom glowing wireframe borders
		TopNeonTrimMesh->SetVisibility(true);
		TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -25.0f, -1.0f));
		TopNeonTrimMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.04f, 0.035f));
		if (TrimMID)
		{
			TrimMID->SetVectorParameterValue(TEXT("BaseColor"), HoloCol);
			TrimMID->SetVectorParameterValue(TEXT("Color"), HoloCol);
			TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), HoloCol * 18.0f);
		}
		AddDeco(CubeMesh, FVector(0.0f, -25.0f, -19.0f), FRotator::ZeroRotator,
			FVector(PlatformWidth / 100.0f, 0.04f, 0.035f), HoloCol, HoloCol * 16.0f);

		// 5-Cell vertical holographic grid dividers matching Reference C: DEEP SPACE ORBIT 1:1
		for (int32 c = 0; c <= 5; ++c)
		{
			const float CellX = -PlatformWidth * 0.48f + float(c) * (PlatformWidth * 0.96f / 5.0f);
			AddDeco(CubeMesh, FVector(CellX, -25.5f, -10.0f), FRotator::ZeroRotator,
				FVector(0.03f, 0.03f, 0.19f), HoloCol, HoloCol * 16.0f);
		}
		if (PatternIdx == 4 || PatternIdx == 6)
		{
			// Bright cyan-white energy core orb
			const FLinearColor OrbCol(0.45f, 0.96f, 1.0f);
			AddDeco(SphereMesh, FVector(0.0f, -26.0f, -10.0f), FRotator::ZeroRotator,
				FVector(0.28f, 0.16f, 0.28f), OrbCol, OrbCol * 22.0f);
		}
	}
	// 6B. BIO-LUMEN XENOVERSE ARK 1:1 (ref_08_bio_lumen_xenoverse_ark_1793214803988.jpg)
	else if (Theme.Id == EVTGameThemeId::BioLumenArk)
	{
		if (FloorLabelText) FloorLabelText->SetVisibility(false);
		const int32 PatternIdx = FMath::Abs(FloorIndex) % 10;
		const bool bVioletPlat = (PatternIdx == 2 || PatternIdx == 3 || PatternIdx == 7);
		const FLinearColor CyanLumen(0.0f, 0.95f, 1.0f);
		const FLinearColor VioletLumen(0.78f, 0.35f, 1.0f);
		const FLinearColor SproutPink(1.0f, 0.42f, 0.78f);
		const FLinearColor EmeraldFlora(0.12f, 0.95f, 0.52f);
		const FLinearColor PlatColor = bVioletPlat ? VioletLumen : CyanLumen;

		if (PatternIdx == 0)
		{
			// Floor 0: Heavy metallic starship bio-dock platform with warning hazard striping & 4 incubator pods
			PlatformMesh->SetVisibility(true);
			PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -14.0f));
			PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.58f, 0.28f));
			if (BodyMID)
			{
				const FLinearColor DeckGrey(0.12f, 0.15f, 0.22f);
				BodyMID->SetVectorParameterValue(TEXT("BaseColor"), DeckGrey);
				BodyMID->SetVectorParameterValue(TEXT("Color"), DeckGrey);
				BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), CyanLumen * 0.6f);
			}
			TopNeonTrimMesh->SetVisibility(true);
			TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -28.0f, -1.0f));
			TopNeonTrimMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.06f, 0.04f));
			if (TrimMID)
			{
				const FLinearColor HazardYellow(1.0f, 0.78f, 0.08f);
				TrimMID->SetVectorParameterValue(TEXT("BaseColor"), HazardYellow);
				TrimMID->SetVectorParameterValue(TEXT("Color"), HazardYellow);
				TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), HazardYellow * 12.0f);
			}

			// 4 Cylindrical Bio-Incubator Pods (2 Violet Coral on left, 2 Emerald Flora on right)
			// Left Pod 1 & 2 (Violet)
			AddDeco(CylinderMesh, FVector(PlatformWidth * 0.24f, -4.0f, 26.0f), FRotator::ZeroRotator,
				FVector(0.24f, 0.24f, 0.56f), FLinearColor(0.18f, 0.22f, 0.28f), VioletLumen * 14.0f);
			AddDeco(CylinderMesh, FVector(PlatformWidth * 0.14f, 8.0f, 32.0f), FRotator::ZeroRotator,
				FVector(0.22f, 0.22f, 0.62f), FLinearColor(0.18f, 0.22f, 0.28f), VioletLumen * 14.0f);
			// Right Pod 3 & 4 (Emerald)
			AddDeco(CylinderMesh, FVector(-PlatformWidth * 0.24f, -4.0f, 26.0f), FRotator::ZeroRotator,
				FVector(0.24f, 0.24f, 0.56f), FLinearColor(0.18f, 0.22f, 0.28f), EmeraldFlora * 14.0f);
			AddDeco(CylinderMesh, FVector(-PlatformWidth * 0.14f, 8.0f, 32.0f), FRotator::ZeroRotator,
				FVector(0.22f, 0.22f, 0.62f), FLinearColor(0.18f, 0.22f, 0.28f), EmeraldFlora * 14.0f);
		}
		else
		{
			// Floors 1..9: Translucent bioluminescent glass/hologram slab with central glowing node & circuits
			PlatformMesh->SetVisibility(true);
			PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -8.0f));
			PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.46f, 0.16f));
			if (BodyMID)
			{
				BodyMID->SetVectorParameterValue(TEXT("BaseColor"), PlatColor * 0.22f);
				BodyMID->SetVectorParameterValue(TEXT("Color"), PlatColor * 0.22f);
				BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), PlatColor * 3.8f);
			}
			TopNeonTrimMesh->SetVisibility(true);
			TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -24.0f, -1.0f));
			TopNeonTrimMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.045f, 0.035f));
			if (TrimMID)
			{
				TrimMID->SetVectorParameterValue(TEXT("BaseColor"), PlatColor);
				TrimMID->SetVectorParameterValue(TEXT("Color"), PlatColor);
				TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), PlatColor * 18.0f);
			}

			// Central glowing circular node
			AddDeco(SphereMesh, FVector(0.0f, -24.0f, -8.0f), FRotator::ZeroRotator,
				FVector(0.16f, 0.12f, 0.16f), PlatColor, PlatColor * 22.0f);

			// Internal cybernetic root circuits radiating outward
			AddDeco(CylinderMesh, FVector(-PlatformWidth * 0.22f, -18.0f, -8.0f), FRotator(0.0f, 0.0f, 86.0f),
				FVector(0.02f, 0.02f, (PlatformWidth * 0.38f) / 100.0f), PlatColor, PlatColor * 14.0f);
			AddDeco(CylinderMesh, FVector(PlatformWidth * 0.22f, -18.0f, -8.0f), FRotator(0.0f, 0.0f, 94.0f),
				FVector(0.02f, 0.02f, (PlatformWidth * 0.38f) / 100.0f), PlatColor, PlatColor * 14.0f);

			// Hanging bioluminescent root-vines underneath (for Cyan platforms)
			if (!bVioletPlat)
			{
				for (int32 v = -2; v <= 2; ++v)
				{
					const float VineX = float(v) * (PlatformWidth * 0.18f);
					const float VineLen = (v % 2 == 0) ? 0.45f : 0.70f;
					AddDeco(CylinderMesh, FVector(VineX, -16.0f, -24.0f), FRotator(0.0f, 0.0f, float(v * 4)),
						FVector(0.025f, 0.025f, VineLen), CyanLumen, CyanLumen * 12.0f);
				}
			}

			// Sprouting alien flora on top
			if (PatternIdx == 1 || PatternIdx == 5 || PatternIdx == 8)
			{
				// Cyan glowing bulb sprouts on left
				AddDeco(SphereMesh, FVector(PlatformWidth * 0.32f, -10.0f, 14.0f), FRotator::ZeroRotator,
					FVector(0.14f, 0.14f, 0.24f), CyanLumen, CyanLumen * 16.0f);
				AddDeco(SphereMesh, FVector(-PlatformWidth * 0.28f, -10.0f, 12.0f), FRotator::ZeroRotator,
					FVector(0.12f, 0.12f, 0.18f), SproutPink, SproutPink * 16.0f);
			}
			else if (bVioletPlat)
			{
				// Violet/magenta crystal flower clusters on top
				AddDeco(ConeMesh, FVector(PlatformWidth * 0.30f, -10.0f, 12.0f), FRotator(180.0f, 0.0f, 0.0f),
					FVector(0.15f, 0.15f, 0.28f), VioletLumen, VioletLumen * 18.0f);
				AddDeco(SphereMesh, FVector(-PlatformWidth * 0.26f, -10.0f, 10.0f), FRotator::ZeroRotator,
					FVector(0.12f, 0.12f, 0.16f), SproutPink, SproutPink * 16.0f);
			}
		}
	}
	// 7. RETRO SYNTHWAVE 80s 1:1 (Extruded Hot-Magenta & Electric-Cyan Neon Bars + Floor 4 Equalizer Staircase)
	else if (Theme.Id == EVTGameThemeId::RetroSynthwave)
	{
		if (FloorLabelText) FloorLabelText->SetVisibility(false);
		const int32 PatternIdx = FMath::Abs(FloorIndex) % 10;
		const bool bCyanBar = (PatternIdx == 1 || PatternIdx == 4 || PatternIdx == 6);
		const FLinearColor NeonCol = bCyanBar ? FLinearColor(0.0f, 0.92f, 1.0f) : FLinearColor(1.0f, 0.05f, 0.65f);

		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -10.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.48f, 0.20f));
		if (BodyMID)
		{
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), NeonCol * 0.35f);
			BodyMID->SetVectorParameterValue(TEXT("Color"), NeonCol * 0.35f);
			BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), NeonCol * 4.5f);
		}
		TopNeonTrimMesh->SetVisibility(true);
		TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -25.0f, -1.0f));
		TopNeonTrimMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.05f, 0.04f));
		if (TrimMID)
		{
			TrimMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::White);
			TrimMID->SetVectorParameterValue(TEXT("Color"), FLinearColor::White);
			TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), NeonCol * 20.0f);
		}

		// Floor 4 in Panel D: NEON SYNTHWAVE has the 3-column stacked Cyan Neon Equalizer Staircase on the right (-X)!
		if (PatternIdx == 4)
		{
			const float ColW = (PlatformWidth * 0.36f) / 3.0f;
			for (int32 col = 0; col < 3; ++col)
			{
				const float ColX = -PlatformWidth * 0.14f - float(col) * ColW;
				const int32 NumBars = 3 + col * 2;
				for (int32 b = 0; b < NumBars; ++b)
				{
					AddDeco(CubeMesh, FVector(ColX, -10.0f, 8.0f + float(b) * 14.0f), FRotator::ZeroRotator,
						FVector((ColW * 0.88f) / 100.0f, 0.34f, 0.09f), NeonCol, NeonCol * 14.0f);
				}
			}
		}
	}
	// 8. 8-BIT PIXEL DUNGEON 1:1 (Voxel Chiseled Stone Blocks with Dripping Pixel Moss & Timber Beams with Iron Chains)
	else if (Theme.Id == EVTGameThemeId::PixelDungeon)
	{
		if (FloorLabelText) FloorLabelText->SetVisibility(false);
		if (NeonLight) NeonLight->SetIntensity(0.0f);
		TopNeonTrimMesh->SetVisibility(false);
		PlatformMesh->SetVisibility(false);

		const int32 PatternIdx = FMath::Abs(FloorIndex) % 10;
		const bool bWoodenBeam = (PatternIdx == 2 || PatternIdx == 6 || PatternIdx == 8 || PatternIdx == 9);
		const FLinearColor StoneCol(0.32f, 0.34f, 0.39f);
		const FLinearColor StoneDark(0.20f, 0.21f, 0.25f);
		const FLinearColor WoodBeamCol(0.52f, 0.33f, 0.17f);
		const FLinearColor PixelMossCol(0.22f, 0.72f, 0.24f);

		auto AddPixelBlock = [&](const FVector& Loc, const FVector& Scale, const FLinearColor& Col)
		{
			return AddDeco(CubeMesh, Loc, FRotator::ZeroRotator, Scale, Col, Col * 0.18f);
		};

		if (bWoodenBeam)
		{
			// Thick pixel-art wooden beam with dark iron end-straps
			AddPixelBlock(FVector(0.0f, 0.0f, -14.0f), FVector(PlatformWidth / 100.0f, 0.44f, 0.28f), WoodBeamCol);
			AddPixelBlock(FVector(-PlatformWidth * 0.42f, -22.5f, -14.0f), FVector(0.12f, 0.03f, 0.30f), StoneDark);
			AddPixelBlock(FVector(PlatformWidth * 0.42f, -22.5f, -14.0f), FVector(0.12f, 0.03f, 0.30f), StoneDark);
		}
		else
		{
			// 3-Block stepped pixel stone slab with mortar seams & dripping green pixel moss
			const int32 NumBlocks = 3;
			const float BW = PlatformWidth / float(NumBlocks);
			for (int32 b = 0; b < NumBlocks; ++b)
			{
				const float BX = -PlatformWidth * 0.5f + (float(b) + 0.5f) * BW;
				AddPixelBlock(FVector(BX, 0.0f, -16.0f), FVector((BW - 4.0f) / 100.0f, 0.46f, 0.32f), (b % 2 == 0) ? StoneCol : StoneCol * 0.88f);
			}
			// Dripping pixel moss blocks along the top & edges
			if (PatternIdx == 3 || PatternIdx == 4 || PatternIdx == 5)
			{
				for (int32 m = 0; m < 4; ++m)
				{
					const float MX = -PlatformWidth * 0.36f + float(m) * (PlatformWidth * 0.24f);
					AddPixelBlock(FVector(MX, -23.5f, -4.0f), FVector(0.28f, 0.04f, 0.12f), PixelMossCol);
					AddPixelBlock(FVector(MX + 6.0f, -23.5f, -20.0f), FVector(0.10f, 0.04f, 0.24f), PixelMossCol);
				}
			}
			// Hanging pixel iron chain under right platforms
			if (PatternIdx == 1 || PatternIdx == 3)
			{
				for (int32 ch = 0; ch < 5; ++ch)
				{
					AddPixelBlock(FVector(0.0f, 8.0f, -40.0f - float(ch) * 18.0f), FVector(0.08f, 0.08f, 0.14f), StoneDark);
				}
			}
		}
	}
	// 9. WHIMSICAL TOYBOX 1:1 (Yellow Wooden Rulers, Interlocking Plastic LEGO Bricks & Corrugated Cardboard Strips)
	else if (Theme.Id == EVTGameThemeId::WhimsicalToybox)
	{
		if (FloorLabelText) FloorLabelText->SetVisibility(false);
		if (NeonLight) NeonLight->SetIntensity(0.0f);
		TopNeonTrimMesh->SetVisibility(false);
		PlatformMesh->SetVisibility(false);

		const int32 PatternIdx = FMath::Abs(FloorIndex) % 10;
		auto AddToyMesh = [&](UStaticMesh* InMesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale, const FLinearColor& Col)
		{
			return AddDeco(InMesh, Loc, Rot, Scale, Col, Col * 0.16f);
		};

		if (PatternIdx == 4 || PatternIdx == 7)
		{
			// Interlocking Plastic LEGO Brick Platforms with raised cylindrical top studs!
			const int32 NumBricks = (PatternIdx == 4) ? 3 : 2;
			const float BrickW = PlatformWidth / float(NumBricks);
			const FLinearColor BrickCols[3] = {
				(PatternIdx == 4) ? FLinearColor(0.14f, 0.45f, 0.88f) : FLinearColor(0.90f, 0.16f, 0.15f),
				(PatternIdx == 4) ? FLinearColor(0.90f, 0.16f, 0.15f) : FLinearColor(0.22f, 0.72f, 0.26f),
				FLinearColor(0.96f, 0.82f, 0.10f)
			};
			for (int32 b = 0; b < NumBricks; ++b)
			{
				const float BX = -PlatformWidth * 0.5f + (float(b) + 0.5f) * BrickW;
				AddToyMesh(CubeMesh, FVector(BX, 0.0f, -15.0f), FRotator::ZeroRotator,
					FVector((BrickW - 2.0f) / 100.0f, 0.44f, 0.30f), BrickCols[b]);
				// 3 cylindrical LEGO studs on top of each brick
				for (int32 st = -1; st <= 1; ++st)
				{
					AddToyMesh(CylinderMesh, FVector(BX + float(st) * (BrickW * 0.28f), 0.0f, 4.0f), FRotator::ZeroRotator,
						FVector(0.20f, 0.20f, 0.09f), BrickCols[b]);
				}
			}
		}
		else if (PatternIdx == 0 || PatternIdx == 3)
		{
			// Yellow Wooden School Ruler with black centimeter tick marks along the top edge
			const FLinearColor RulerYellow(0.95f, 0.80f, 0.26f);
			AddToyMesh(CubeMesh, FVector(0.0f, 0.0f, -12.0f), FRotator::ZeroRotator,
				FVector(PlatformWidth / 100.0f, 0.42f, 0.24f), RulerYellow);
			const int32 NumTicks = 12;
			for (int32 t = 0; t < NumTicks; ++t)
			{
				const float TX = -PlatformWidth * 0.45f + float(t) * (PlatformWidth * 0.90f / float(NumTicks - 1));
				const float TickH = (t % 2 == 0) ? 0.14f : 0.08f;
				AddToyMesh(CubeMesh, FVector(TX, -21.5f, -5.0f), FRotator::ZeroRotator,
					FVector(0.02f, 0.02f, TickH), FLinearColor(0.12f, 0.10f, 0.08f));
			}
		}
		else if (PatternIdx == 9)
		{
			// Purple/magenta plastic toy bar with round button detail underneath
			const FLinearColor PurpleToy(0.72f, 0.32f, 0.84f);
			AddToyMesh(CubeMesh, FVector(0.0f, 0.0f, -12.0f), FRotator::ZeroRotator,
				FVector(PlatformWidth / 100.0f, 0.42f, 0.24f), PurpleToy);
			AddToyMesh(CylinderMesh, FVector(-PlatformWidth * 0.30f, -10.0f, -26.0f), FRotator(0.0f, 0.0f, 90.0f),
				FVector(0.28f, 0.28f, 0.20f), FLinearColor(0.48f, 0.24f, 0.68f));
		}
		else
		{
			// Corrugated Cardboard Strip showing wavy inner fluting along its front face
			const FLinearColor CardboardCol(0.78f, 0.60f, 0.38f);
			const FLinearColor FluteShadow(0.48f, 0.34f, 0.18f);
			AddToyMesh(CubeMesh, FVector(0.0f, 0.0f, -10.0f), FRotator::ZeroRotator,
				FVector(PlatformWidth / 100.0f, 0.42f, 0.20f), CardboardCol);
			const int32 NumFlutes = 10;
			for (int32 f = 0; f < NumFlutes; ++f)
			{
				const float FX = -PlatformWidth * 0.45f + float(f) * (PlatformWidth * 0.90f / float(NumFlutes - 1));
				AddToyMesh(CylinderMesh, FVector(FX, -21.5f, -10.0f), FRotator(0.0f, 0.0f, 90.0f),
					FVector(0.08f, 0.08f, 0.02f), FluteShadow);
			}
		}
	}
	// 10. CYBERPUNK METROPOLIS 1:1 (Alternating Translucent Cyan Hologram Wireframe Grids & Dark-Steel Tech Girders)
	else
	{
		if (FloorLabelText) FloorLabelText->SetVisibility(false);
		const int32 PatternIdx = FMath::Abs(FloorIndex) % 10;
		const bool bHoloBridge = (PatternIdx == 2 || PatternIdx == 5 || PatternIdx == 7 || PatternIdx == 9);
		const FLinearColor CyanHolo(0.0f, 0.94f, 1.0f);
		const FLinearColor AmberWarn(1.0f, 0.55f, 0.10f);

		PlatformMesh->SetVisibility(true);
		PlatformMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -8.0f));
		PlatformMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.52f, 0.16f));

		if (bHoloBridge)
		{
			if (BodyMID)
			{
				BodyMID->SetVectorParameterValue(TEXT("BaseColor"), CyanHolo * 0.22f);
				BodyMID->SetVectorParameterValue(TEXT("Color"), CyanHolo * 0.22f);
				BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), CyanHolo * 3.8f);
			}
			TopNeonTrimMesh->SetVisibility(true);
			TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -26.0f, -1.0f));
			TopNeonTrimMesh->SetRelativeScale3D(FVector(PlatformWidth / 100.0f, 0.04f, 0.035f));
			if (TrimMID)
			{
				TrimMID->SetVectorParameterValue(TEXT("BaseColor"), CyanHolo);
				TrimMID->SetVectorParameterValue(TEXT("Color"), CyanHolo);
				TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), CyanHolo * 18.0f);
			}
			for (int32 g = -2; g <= 2; ++g)
			{
				AddDeco(CubeMesh, FVector(float(g) * (PlatformWidth * 0.20f), -26.5f, -8.0f), FRotator::ZeroRotator,
					FVector(0.025f, 0.02f, 0.16f), CyanHolo, CyanHolo * 14.0f);
			}
		}
		else
		{
			const FLinearColor SteelGirder(0.06f, 0.09f, 0.14f);
			if (BodyMID)
			{
				BodyMID->SetVectorParameterValue(TEXT("BaseColor"), SteelGirder);
				BodyMID->SetVectorParameterValue(TEXT("Color"), SteelGirder);
				BodyMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor::Black);
			}
			TopNeonTrimMesh->SetVisibility(true);
			TopNeonTrimMesh->SetRelativeLocation(FVector(0.0f, -26.0f, -4.0f));
			TopNeonTrimMesh->SetRelativeScale3D(FVector((PlatformWidth * 0.94f) / 100.0f, 0.04f, 0.04f));
			const FLinearColor LightCol = (PatternIdx == 1 || PatternIdx == 4) ? AmberWarn : CyanHolo;
			if (TrimMID)
			{
				TrimMID->SetVectorParameterValue(TEXT("BaseColor"), LightCol);
				TrimMID->SetVectorParameterValue(TEXT("Color"), LightCol);
				TrimMID->SetVectorParameterValue(TEXT("EmissiveColor"), LightCol * 14.0f);
			}
		}
	}
}

#include "VTClimberCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Engine/World.h"

AVTClimberCharacter::AVTClimberCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	// High-Fidelity Next-Gen Humanoid / MetaHuman Mesh for Climber #01
	SkeletalClimberMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalClimberMesh"));
	SkeletalClimberMesh->SetupAttachment(RootScene);
	SkeletalClimberMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalClimberMesh->SetCastShadow(true);
	SkeletalClimberMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -44.0f));
	SkeletalClimberMesh->SetRelativeScale3D(FVector(0.50f, 0.50f, 0.50f));
	SkeletalClimberMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	SkeletalClimberMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	SkeletalClimberMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMeshFinder(TEXT("/Game/Characters/Manny/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MannyMeshFinder.Succeeded())
	{
		SkeletalClimberMesh->SetSkeletalMeshAsset(MannyMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleFinder(TEXT("/Game/Characters/Manny/Animations/Manny/MM_Idle.MM_Idle"));
	if (IdleFinder.Succeeded()) AnimMannyIdle = IdleFinder.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunFinder(TEXT("/Game/Characters/Manny/Animations/Manny/MM_Run_Fwd.MM_Run_Fwd"));
	if (RunFinder.Succeeded()) AnimMannyRun = RunFinder.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> JumpFinder(TEXT("/Game/Characters/Manny/Animations/Manny/MM_Jump.MM_Jump"));
	if (JumpFinder.Succeeded()) AnimMannyJump = JumpFinder.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> FallFinder(TEXT("/Game/Characters/Manny/Animations/Manny/MM_Fall_Loop.MM_Fall_Loop"));
	if (FallFinder.Succeeded()) AnimMannyFall = FallFinder.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> LandFinder(TEXT("/Game/Characters/Manny/Animations/Manny/MM_Land.MM_Land"));
	if (LandFinder.Succeeded()) AnimMannyLand = LandFinder.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> RollFinder(TEXT("/Game/Characters/Manny/Animations/Manny/MM_Land.MM_Land"));
	if (RollFinder.Succeeded()) AnimMannyRoll = RollFinder.Object;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> EngineSphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EngineCylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EngineCubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EngineConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));

	UStaticMesh* SphereMesh = EngineSphereFinder.Succeeded() ? EngineSphereFinder.Object : nullptr;
	UStaticMesh* CylMesh = EngineCylinderFinder.Succeeded() ? EngineCylinderFinder.Object : nullptr;
	UStaticMesh* CubeMesh = EngineCubeFinder.Succeeded() ? EngineCubeFinder.Object : nullptr;
	UStaticMesh* ConeMesh = EngineConeFinder.Succeeded() ? EngineConeFinder.Object.Get() : CylMesh;

	auto MakePart = [this](const FName& Name, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Comp->SetupAttachment(Parent);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(true);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		return Comp;
	};

	// 1. Sculpted Cyber-Hoodie Torso (Root of articulated character body)
	ClimberMesh = MakePart(TEXT("ClimberMesh"), RootScene, CylMesh, FVector(0.0f, 0.0f, 4.0f), FRotator::ZeroRotator, FVector(0.38f, 0.28f, 0.36f));

	// 2. Sculpted 3D Cyber-Helmet Head & Wrap-Around Glowing Visor + Dual LED Eyes
	HeadMesh = MakePart(TEXT("HeadMesh"), ClimberMesh, SphereMesh, FVector(0.0f, 0.0f, 84.0f), FRotator::ZeroRotator, FVector(0.88f, 1.12f, 0.82f));
	VisorWrapMesh = MakePart(TEXT("VisorWrapMesh"), HeadMesh, CubeMesh, FVector(28.0f, 0.0f, 4.0f), FRotator(0.0f, 0.0f, 0.0f), FVector(0.36f, 0.82f, 0.32f));
	LeftEyeGlowMesh = MakePart(TEXT("LeftEyeGlowMesh"), VisorWrapMesh, SphereMesh, FVector(42.0f, -24.0f, 4.0f), FRotator::ZeroRotator, FVector(0.28f, 0.22f, 0.28f));
	RightEyeGlowMesh = MakePart(TEXT("RightEyeGlowMesh"), VisorWrapMesh, SphereMesh, FVector(42.0f, 24.0f, 4.0f), FRotator::ZeroRotator, FVector(0.28f, 0.22f, 0.28f));

	// 3. Multi-Part 3D Headgear & Accent Crest (customized per character #01..#20)
	HeadgearMesh = MakePart(TEXT("HeadgearMesh"), HeadMesh, ConeMesh, FVector(0.0f, 0.0f, 48.0f), FRotator::ZeroRotator, FVector(0.55f, 0.55f, 0.42f));
	HeadgearAccentMesh = MakePart(TEXT("HeadgearAccentMesh"), HeadMesh, CylMesh, FVector(16.0f, 0.0f, 34.0f), FRotator(-12.0f, 0.0f, 0.0f), FVector(0.75f, 0.85f, 0.12f));

	// 4. Glowing Chest Arc-Reactor Core & Tech-Belt
	ChestCoreMesh = MakePart(TEXT("ChestCoreMesh"), ClimberMesh, CylMesh, FVector(38.0f, 0.0f, 16.0f), FRotator(90.0f, 0.0f, 0.0f), FVector(0.38f, 0.38f, 0.18f));
	PelvisBeltMesh = MakePart(TEXT("PelvisBeltMesh"), ClimberMesh, CylMesh, FVector(0.0f, 0.0f, -44.0f), FRotator::ZeroRotator, FVector(1.06f, 1.06f, 0.16f));

	// 5. Twin-Thruster Cyber-Jetpack on Back
	BackpackJetMesh = MakePart(TEXT("BackpackJetMesh"), ClimberMesh, CubeMesh, FVector(-42.0f, 0.0f, 12.0f), FRotator::ZeroRotator, FVector(0.38f, 0.78f, 0.68f));
	LeftThrusterMesh = MakePart(TEXT("LeftThrusterMesh"), BackpackJetMesh, ConeMesh, FVector(-22.0f, -32.0f, -42.0f), FRotator(180.0f, 0.0f, 0.0f), FVector(0.45f, 0.45f, 0.55f));
	RightThrusterMesh = MakePart(TEXT("RightThrusterMesh"), BackpackJetMesh, ConeMesh, FVector(-22.0f, 32.0f, -42.0f), FRotator(180.0f, 0.0f, 0.0f), FVector(0.45f, 0.45f, 0.55f));

	// 6. Armored Shoulder Pauldrons + Articulated Arms + Glowing Cyber-Gauntlets
	LeftShoulderMesh = MakePart(TEXT("LeftShoulderMesh"), ClimberMesh, SphereMesh, FVector(0.0f, -66.0f, 32.0f), FRotator::ZeroRotator, FVector(0.52f, 0.58f, 0.46f));
	RightShoulderMesh = MakePart(TEXT("RightShoulderMesh"), ClimberMesh, SphereMesh, FVector(0.0f, 66.0f, 32.0f), FRotator::ZeroRotator, FVector(0.52f, 0.58f, 0.46f));

	LeftArmMesh = MakePart(TEXT("LeftArmMesh"), LeftShoulderMesh, CylMesh, FVector(6.0f, -18.0f, -54.0f), FRotator(14.0f, 0.0f, 10.0f), FVector(0.46f, 0.46f, 0.95f));
	RightArmMesh = MakePart(TEXT("RightArmMesh"), RightShoulderMesh, CylMesh, FVector(6.0f, 18.0f, -54.0f), FRotator(-14.0f, 0.0f, -10.0f), FVector(0.46f, 0.46f, 0.95f));

	LeftGauntletMesh = MakePart(TEXT("LeftGauntletMesh"), LeftArmMesh, CubeMesh, FVector(8.0f, 0.0f, -52.0f), FRotator::ZeroRotator, FVector(0.95f, 0.95f, 0.52f));
	RightGauntletMesh = MakePart(TEXT("RightGauntletMesh"), RightArmMesh, CubeMesh, FVector(8.0f, 0.0f, -52.0f), FRotator::ZeroRotator, FVector(0.95f, 0.95f, 0.52f));

	// 7. Articulated Cyber-Legs + Oversized High-Top Neon Sneakers / Hover-Boots
	LeftLegMesh = MakePart(TEXT("LeftLegMesh"), ClimberMesh, CylMesh, FVector(0.0f, -28.0f, -78.0f), FRotator::ZeroRotator, FVector(0.36f, 0.44f, 0.52f));
	RightLegMesh = MakePart(TEXT("RightLegMesh"), ClimberMesh, CylMesh, FVector(0.0f, 28.0f, -78.0f), FRotator::ZeroRotator, FVector(0.36f, 0.44f, 0.52f));

	LeftShoeMesh = MakePart(TEXT("LeftShoeMesh"), LeftLegMesh, CubeMesh, FVector(18.0f, 0.0f, -52.0f), FRotator::ZeroRotator, FVector(1.45f, 1.08f, 0.44f));
	RightShoeMesh = MakePart(TEXT("RightShoeMesh"), RightLegMesh, CubeMesh, FVector(18.0f, 0.0f, -52.0f), FRotator::ZeroRotator, FVector(1.45f, 1.08f, 0.44f));

	// 8. 3D Showroom Stage Pedestal & Hologram Ring (visible during Character Select & Main Menu)
	ShowroomPedestalMesh = MakePart(TEXT("ShowroomPedestalMesh"), RootScene, CylMesh, FVector(0.0f, 0.0f, -46.0f), FRotator::ZeroRotator, FVector(1.15f, 1.15f, 0.08f));
	ShowroomHoloRingMesh = MakePart(TEXT("ShowroomHoloRingMesh"), RootScene, CylMesh, FVector(0.0f, 0.0f, -40.0f), FRotator::ZeroRotator, FVector(1.38f, 1.38f, 0.025f));

	ShieldSphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldSphereMesh"));
	ShieldSphereMesh->SetupAttachment(RootScene);
	ShieldSphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldSphereMesh->SetRelativeScale3D(FVector(1.35f));
	ShieldSphereMesh->SetVisibility(false);
	if (SphereMesh) ShieldSphereMesh->SetStaticMesh(SphereMesh);

	JumpGuideRingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("JumpGuideRingMesh"));
	JumpGuideRingMesh->SetupAttachment(RootScene);
	JumpGuideRingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	JumpGuideRingMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -42.0f));
	JumpGuideRingMesh->SetRelativeScale3D(FVector(0.95f, 0.95f, 0.04f));
	if (CylMesh) JumpGuideRingMesh->SetStaticMesh(CylMesh);

	ClimberRimLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ClimberRimLight"));
	ClimberRimLight->SetupAttachment(RootScene);
	ClimberRimLight->SetRelativeLocation(FVector(0.0f, -95.0f, 28.0f));
	ClimberRimLight->SetIntensity(11000.0f);
	ClimberRimLight->SetAttenuationRadius(550.0f);
	ClimberRimLight->SetLightColor(FLinearColor(0.0f, 0.961f, 0.831f));

	ClimberBackLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ClimberBackLight"));
	ClimberBackLight->SetupAttachment(RootScene);
	ClimberBackLight->SetRelativeLocation(FVector(0.0f, 95.0f, 32.0f));
	ClimberBackLight->SetIntensity(14500.0f);
	ClimberBackLight->SetAttenuationRadius(650.0f);
	ClimberBackLight->SetLightColor(FLinearColor(0.0f, 0.961f, 0.831f));

	// 9. Flowing Shinobi Headband with Physics-reactive Ribbons (MetaHuman)
	HeadbandRingMesh = MakePart(TEXT("HeadbandRingMesh"), RootScene, CylMesh, FVector(0.0f, 0.0f, 32.0f), FRotator::ZeroRotator, FVector(0.24f, 0.24f, 0.045f));
	HeadbandKnotMesh = MakePart(TEXT("HeadbandKnotMesh"), HeadbandRingMesh, SphereMesh, FVector(-22.0f, 0.0f, 0.0f), FRotator::ZeroRotator, FVector(0.35f, 0.35f, 0.35f));
	HeadbandRibbonLeftMesh = MakePart(TEXT("HeadbandRibbonLeftMesh"), HeadbandKnotMesh, CubeMesh, FVector(-18.0f, -8.0f, -12.0f), FRotator(-15.0f, -8.0f, 0.0f), FVector(0.38f, 0.06f, 0.02f));
	HeadbandRibbonRightMesh = MakePart(TEXT("HeadbandRibbonRightMesh"), HeadbandKnotMesh, CubeMesh, FVector(-22.0f, 8.0f, -16.0f), FRotator(-22.0f, 10.0f, 0.0f), FVector(0.48f, 0.06f, 0.02f));

	if (HeadbandRingMesh) HeadbandRingMesh->SetVisibility(false);
	if (HeadbandKnotMesh) HeadbandKnotMesh->SetVisibility(false);
	if (HeadbandRibbonLeftMesh) HeadbandRibbonLeftMesh->SetVisibility(false);
	if (HeadbandRibbonRightMesh) HeadbandRibbonRightMesh->SetVisibility(false);

	// Create 7 3D Scarf Segments
	for (int32 i = 0; i < 7; ++i)
	{
		UStaticMeshComponent* Seg = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ScarfSeg_%d"), i));
		Seg->SetupAttachment(RootScene);
		Seg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Seg->SetCastShadow(false);
		if (SphereMesh) Seg->SetStaticMesh(SphereMesh);
		const float ScaleFactor = FMath::Lerp(0.24f, 0.12f, float(i) / 6.0f);
		Seg->SetWorldScale3D(FVector(ScaleFactor));
		ScarfSegmentMeshes.Add(Seg);
		ScarfPoints.Add(FVector::ZeroVector);
	}

	// Create 6 3D Ghost Trail Meshes for Aerial Somersaults & Overdrive
	for (int32 i = 0; i < 6; ++i)
	{
		UStaticMeshComponent* Ghost = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("GhostTrail_%d"), i));
		Ghost->SetupAttachment(RootScene);
		Ghost->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ghost->SetCastShadow(false);
		Ghost->SetVisibility(false);
		if (SphereMesh) Ghost->SetStaticMesh(SphereMesh);
		GhostTrailMeshes.Add(Ghost);
		GhostLocations.Add(FVector::ZeroVector);
		GhostRotations.Add(FRotator::ZeroRotator);
		GhostAlphas.Add(0.0f);
	}
}

void AVTClimberCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AVTClimberCharacter::PlaySkeletalAnim(UAnimSequence* NewAnim, bool bLoop, float PlayRate)
{
	if (!SkeletalClimberMesh || !NewAnim)
	{
		return;
	}

	if (CurrentPlayingAnim != NewAnim)
	{
		CurrentPlayingAnim = NewAnim;
		SkeletalClimberMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		SkeletalClimberMesh->SetAnimation(NewAnim);
		SkeletalClimberMesh->SetPlayRate(PlayRate);
		SkeletalClimberMesh->Play(bLoop);
	}
	else
	{
		SkeletalClimberMesh->SetPlayRate(PlayRate);
		if (!SkeletalClimberMesh->IsPlaying())
		{
			SkeletalClimberMesh->Play(bLoop);
		}
	}
}

void AVTClimberCharacter::ResetClimberForStart(float StartX, float StartZ, int32 InStartFloor, const FVTClimberInfo& InClimberInfo, const FLinearColor& InTrailColor)
{
	PosX = StartX;
	PosZ = StartZ;
	VelX = 0.0f;
	VelZ = 0.0f;
	FacingDir = 1;
	bOnGround = true;
	CurrentFloor = InStartFloor;
	FlipAngleRad = 0.0f;
	bFlipping = false;
	RunPhase = 0.0f;
	SquashScaleX = 1.0f;
	SquashScaleZ = 1.0f;
	CoyoteFrames = 7;
	bWallBounceUsed = false;
	bWasOverdrive = false;

	SetActorLocation(FVector(PosX, 0.0f, PosZ));
	if (ShowroomPedestalMesh) ShowroomPedestalMesh->SetVisibility(false);
	if (ShowroomHoloRingMesh) ShowroomHoloRingMesh->SetVisibility(false);

	for (int32 i = 0; i < ScarfPoints.Num(); ++i)
	{
		ScarfPoints[i] = FVector(PosX, 10.0f, PosZ + 16.0f);
	}
	for (int32 i = 0; i < GhostAlphas.Num(); ++i)
	{
		GhostAlphas[i] = 0.0f;
		if (GhostTrailMeshes[i]) GhostTrailMeshes[i]->SetVisibility(false);
	}

	ApplyClimberVisuals(InClimberInfo, InTrailColor, false);
}

void AVTClimberCharacter::ApplyClimberVisuals(const FVTClimberInfo& InClimberInfo, const FLinearColor& InTrailColor, bool bLockedSilhouette)
{
	const FLinearColor PitchBlack(0.0f, 0.0f, 0.0f, 1.0f);

	// Character #01 Showcase: High-Fidelity Next-Gen Humanoid / MetaHuman Mesh!
	if (SkeletalClimberMesh && !SkeletalClimberMesh->GetSkeletalMeshAsset())
	{
		SkeletalClimberMesh->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Manny/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
	}
	if (!AnimMannyIdle) AnimMannyIdle = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Manny/Animations/Manny/MM_Idle.MM_Idle"));
	if (!AnimMannyRun) AnimMannyRun = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Manny/Animations/Manny/MM_Run_Fwd.MM_Run_Fwd"));
	if (!AnimMannyJump) AnimMannyJump = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Manny/Animations/Manny/MM_Jump.MM_Jump"));
	if (!AnimMannyFall) AnimMannyFall = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Manny/Animations/Manny/MM_Fall_Loop.MM_Fall_Loop"));
	if (!AnimMannyLand) AnimMannyLand = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Manny/Animations/Manny/MM_Land.MM_Land"));
	if (!AnimMannyRoll) AnimMannyRoll = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Manny/Animations/Manny/MM_Land.MM_Land"));

	const bool bIsFirstChar = (InClimberInfo.RosterNumber == 1 || InClimberInfo.Id == EVTClimberId::KidFlex);
	bUsingSkeletalMesh = bIsFirstChar && SkeletalClimberMesh && (SkeletalClimberMesh->GetSkeletalMeshAsset() != nullptr);

	if (bUsingSkeletalMesh)
	{
		SkeletalClimberMesh->SetVisibility(true);
		SkeletalClimberMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		if (AnimMannyIdle)
		{
			PlaySkeletalAnim(AnimMannyIdle, true, 1.0f);
		}
		if (ClimberMesh) ClimberMesh->SetVisibility(false, true);
		if (HeadMesh) HeadMesh->SetVisibility(false, true);
		if (HeadgearMesh) HeadgearMesh->SetVisibility(false);
		if (HeadgearAccentMesh) HeadgearAccentMesh->SetVisibility(false);

		// Hide headband ribbon blocks so Manny's head stays clean and free of strange floating objects
		if (HeadbandRingMesh) HeadbandRingMesh->SetVisibility(false);
		if (HeadbandKnotMesh) HeadbandKnotMesh->SetVisibility(false);
		if (HeadbandRibbonLeftMesh) HeadbandRibbonLeftMesh->SetVisibility(false);
		if (HeadbandRibbonRightMesh) HeadbandRibbonRightMesh->SetVisibility(false);

		for (int32 MatIdx = 0; MatIdx < SkeletalClimberMesh->GetNumMaterials(); ++MatIdx)
		{
			UMaterialInstanceDynamic* DynMat = SkeletalClimberMesh->CreateAndSetMaterialInstanceDynamic(MatIdx);
			if (DynMat)
			{
				if (bLockedSilhouette)
				{
					DynMat->SetVectorParameterValue(TEXT("BaseColor"), PitchBlack);
					DynMat->SetVectorParameterValue(TEXT("Color"), PitchBlack);
					DynMat->SetVectorParameterValue(TEXT("Tint"), PitchBlack);
					DynMat->SetVectorParameterValue(TEXT("EmissiveColor"), PitchBlack);
					DynMat->SetVectorParameterValue(TEXT("Emissive"), PitchBlack);
				}
				else
				{
					// Crisp high-contrast cyberpunk hero material: bright cyber base + vibrant trail emissive
					DynMat->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.92f, 0.96f, 1.0f, 1.0f));
					DynMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.92f, 0.96f, 1.0f, 1.0f));
					DynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.92f, 0.96f, 1.0f, 1.0f));
					DynMat->SetVectorParameterValue(TEXT("AccentColor"), InTrailColor);
					DynMat->SetVectorParameterValue(TEXT("EmissiveColor"), InTrailColor * 3.5f);
					DynMat->SetVectorParameterValue(TEXT("Emissive"), InTrailColor * 3.5f);
				}
			}
		}
	}
	else
	{
		if (HeadbandRingMesh) HeadbandRingMesh->SetVisibility(false);
		if (HeadbandKnotMesh) HeadbandKnotMesh->SetVisibility(false);
		if (HeadbandRibbonLeftMesh) HeadbandRibbonLeftMesh->SetVisibility(false);
		if (HeadbandRibbonRightMesh) HeadbandRibbonRightMesh->SetVisibility(false);

		if (SkeletalClimberMesh) SkeletalClimberMesh->SetVisibility(false);
		if (ClimberMesh) ClimberMesh->SetVisibility(true, true);
	}

	UMaterialInterface* CustomNeonMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VelocityTower/Materials/M_NeonEmissive_Master.M_NeonEmissive_Master"));
	UMaterialInterface* CustomClimberMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VelocityTower/Materials/M_Climber_Master.M_Climber_Master"));
	UMaterialInterface* FallbackMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInterface* BaseMat = CustomClimberMat ? CustomClimberMat : (CustomNeonMat ? CustomNeonMat : FallbackMat);
	UMaterialInterface* NeonOrBaseMat = CustomNeonMat ? CustomNeonMat : BaseMat;

	auto MakeSolidMID = [&](const FLinearColor& Col, float EmissiveBoost = 1.6f) -> UMaterialInstanceDynamic*
	{
		if (!BaseMat) return nullptr;
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
		const FLinearColor FinalCol = bLockedSilhouette ? PitchBlack : Col;
		const FLinearColor FinalEmissive = bLockedSilhouette ? PitchBlack : (Col * EmissiveBoost);
		MID->SetVectorParameterValue(TEXT("BaseColor"), FinalCol);
		MID->SetVectorParameterValue(TEXT("Color"), FinalCol);
		MID->SetVectorParameterValue(TEXT("EmissiveColor"), FinalEmissive);
		return MID;
	};

	// Distinct 3D Headgear & Silhouette geometry per Climber (#01..#20) - ONLY for stylized climber models, NOT MetaHuman!
	if (HeadgearMesh && HeadgearAccentMesh && !bUsingSkeletalMesh)
	{
		const int32 Variant = InClimberInfo.RosterNumber % 6;
		HeadgearMesh->SetVisibility(true);
		HeadgearAccentMesh->SetVisibility(true);

		if (InClimberInfo.RosterNumber == 1)
		{
			// #01 Kid Flex: Street Snapback Cap + Forward Brim + DJ Headphones
			HeadgearMesh->SetRelativeLocation(FVector(-4.0f, 0.0f, 36.0f));
			HeadgearMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
			HeadgearMesh->SetRelativeScale3D(FVector(0.86f, 0.92f, 0.36f));

			HeadgearAccentMesh->SetRelativeLocation(FVector(26.0f, 0.0f, 24.0f));
			HeadgearAccentMesh->SetRelativeRotation(FRotator(-10.0f, 0.0f, 0.0f));
			HeadgearAccentMesh->SetRelativeScale3D(FVector(0.72f, 0.88f, 0.10f));
		}
		else if (Variant == 0)
		{
			// Cyber-Emperor Floating Halo + Crown Spike
			HeadgearMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 52.0f));
			HeadgearMesh->SetRelativeRotation(FRotator::ZeroRotator);
			HeadgearMesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.65f));

			HeadgearAccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 74.0f));
			HeadgearAccentMesh->SetRelativeRotation(FRotator::ZeroRotator);
			HeadgearAccentMesh->SetRelativeScale3D(FVector(1.25f, 1.25f, 0.08f));
		}
		else if (Variant == 1)
		{
			// Samurai Ronin Crest + Wide Battle-Horns
			HeadgearMesh->SetRelativeLocation(FVector(10.0f, 0.0f, 54.0f));
			HeadgearMesh->SetRelativeRotation(FRotator(25.0f, 0.0f, 0.0f));
			HeadgearMesh->SetRelativeScale3D(FVector(0.28f, 1.15f, 0.55f));

			HeadgearAccentMesh->SetRelativeLocation(FVector(-6.0f, 0.0f, 38.0f));
			HeadgearAccentMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
			HeadgearAccentMesh->SetRelativeScale3D(FVector(0.26f, 1.35f, 0.16f));
		}
		else if (Variant == 2)
		{
			// Cyber-Kitsune / Twin Fox Ears + Visor Fin
			HeadgearMesh->SetRelativeLocation(FVector(0.0f, -26.0f, 52.0f));
			HeadgearMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, -18.0f));
			HeadgearMesh->SetRelativeScale3D(FVector(0.38f, 0.38f, 0.68f));

			HeadgearAccentMesh->SetRelativeLocation(FVector(0.0f, 26.0f, 52.0f));
			HeadgearAccentMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 18.0f));
			HeadgearAccentMesh->SetRelativeScale3D(FVector(0.38f, 0.38f, 0.68f));
		}
		else if (Variant == 3)
		{
			// Synthwave DJ Headphones + Tall Cyber-Mohawk Fin
			HeadgearMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 56.0f));
			HeadgearMesh->SetRelativeRotation(FRotator::ZeroRotator);
			HeadgearMesh->SetRelativeScale3D(FVector(0.88f, 0.22f, 0.62f));

			HeadgearAccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
			HeadgearAccentMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
			HeadgearAccentMesh->SetRelativeScale3D(FVector(0.52f, 0.52f, 1.32f));
		}
		else
		{
			// Valkyrie Cyber-Wings + Horned Visor
			HeadgearMesh->SetRelativeLocation(FVector(-8.0f, 0.0f, 44.0f));
			HeadgearMesh->SetRelativeRotation(FRotator(-18.0f, 0.0f, 0.0f));
			HeadgearMesh->SetRelativeScale3D(FVector(0.42f, 1.48f, 0.28f));

			HeadgearAccentMesh->SetRelativeLocation(FVector(18.0f, 0.0f, 48.0f));
			HeadgearAccentMesh->SetRelativeRotation(FRotator(32.0f, 0.0f, 0.0f));
			HeadgearAccentMesh->SetRelativeScale3D(FVector(0.32f, 0.32f, 0.58f));
		}
	}

	UMaterialInstanceDynamic* HoodieMID = MakeSolidMID(InClimberInfo.HoodieColor, 2.2f);
	UMaterialInstanceDynamic* PantsMID = MakeSolidMID(InClimberInfo.PantsColor, 1.6f);
	UMaterialInstanceDynamic* ShoesMID = MakeSolidMID(InClimberInfo.ShoesColor, 3.8f);
	UMaterialInstanceDynamic* VisorMID = MakeSolidMID(InClimberInfo.VisorColor, 12.0f);
	UMaterialInstanceDynamic* EyeMID = MakeSolidMID(FLinearColor::White, 18.0f);
	UMaterialInstanceDynamic* DarkArmorMID = MakeSolidMID(InClimberInfo.PantsColor * 0.45f, 0.8f);

	if (ClimberMesh && HoodieMID) ClimberMesh->SetMaterial(0, HoodieMID);
	if (HeadMesh && DarkArmorMID) HeadMesh->SetMaterial(0, DarkArmorMID);
	if (VisorWrapMesh && VisorMID) VisorWrapMesh->SetMaterial(0, VisorMID);
	if (LeftEyeGlowMesh && EyeMID) LeftEyeGlowMesh->SetMaterial(0, EyeMID);
	if (RightEyeGlowMesh && EyeMID) RightEyeGlowMesh->SetMaterial(0, EyeMID);
	if (HeadgearMesh && HoodieMID) HeadgearMesh->SetMaterial(0, HoodieMID);
	if (HeadgearAccentMesh && VisorMID) HeadgearAccentMesh->SetMaterial(0, VisorMID);
	if (ChestCoreMesh && VisorMID) ChestCoreMesh->SetMaterial(0, VisorMID);
	if (PelvisBeltMesh && DarkArmorMID) PelvisBeltMesh->SetMaterial(0, DarkArmorMID);
	if (BackpackJetMesh && DarkArmorMID) BackpackJetMesh->SetMaterial(0, DarkArmorMID);
	if (LeftThrusterMesh && ShoesMID) LeftThrusterMesh->SetMaterial(0, ShoesMID);
	if (RightThrusterMesh && ShoesMID) RightThrusterMesh->SetMaterial(0, ShoesMID);
	if (LeftShoulderMesh && HoodieMID) LeftShoulderMesh->SetMaterial(0, HoodieMID);
	if (RightShoulderMesh && HoodieMID) RightShoulderMesh->SetMaterial(0, HoodieMID);
	if (LeftArmMesh && HoodieMID) LeftArmMesh->SetMaterial(0, HoodieMID);
	if (RightArmMesh && HoodieMID) RightArmMesh->SetMaterial(0, HoodieMID);
	if (LeftGauntletMesh && VisorMID) LeftGauntletMesh->SetMaterial(0, VisorMID);
	if (RightGauntletMesh && VisorMID) RightGauntletMesh->SetMaterial(0, VisorMID);
	if (LeftLegMesh && PantsMID) LeftLegMesh->SetMaterial(0, PantsMID);
	if (RightLegMesh && PantsMID) RightLegMesh->SetMaterial(0, PantsMID);
	if (LeftShoeMesh && ShoesMID) LeftShoeMesh->SetMaterial(0, ShoesMID);
	if (RightShoeMesh && ShoesMID) RightShoeMesh->SetMaterial(0, ShoesMID);

	// Pedestal is ALWAYS lit so even a pitch-black locked silhouette stands on a glowing hologram stage!
	if (NeonOrBaseMat && ShowroomPedestalMesh && ShowroomHoloRingMesh)
	{
		UMaterialInstanceDynamic* PedMID = UMaterialInstanceDynamic::Create(NeonOrBaseMat, this);
		const FLinearColor PedCol = bLockedSilhouette ? FLinearColor(0.08f, 0.14f, 0.24f, 1.0f) : (InClimberInfo.HoodieColor * 0.35f);
		PedMID->SetVectorParameterValue(TEXT("BaseColor"), PedCol);
		PedMID->SetVectorParameterValue(TEXT("EmissiveColor"), PedCol * 3.0f);
		ShowroomPedestalMesh->SetMaterial(0, PedMID);

		UMaterialInstanceDynamic* RingMID = UMaterialInstanceDynamic::Create(NeonOrBaseMat, this);
		const FLinearColor RingCol = bLockedSilhouette ? FLinearColor(1.0f, 0.78f, 0.10f, 1.0f) : InClimberInfo.VisorColor;
		RingMID->SetVectorParameterValue(TEXT("BaseColor"), RingCol);
		RingMID->SetVectorParameterValue(TEXT("EmissiveColor"), RingCol * 10.0f);
		ShowroomHoloRingMesh->SetMaterial(0, RingMID);
	}

	if (NeonOrBaseMat)
	{
		ScarfMID = UMaterialInstanceDynamic::Create(NeonOrBaseMat, this);
		const FLinearColor ScarfCol = bLockedSilhouette ? PitchBlack : InClimberInfo.ScarfColor;
		ScarfMID->SetVectorParameterValue(TEXT("BaseColor"), ScarfCol);
		ScarfMID->SetVectorParameterValue(TEXT("Color"), ScarfCol);
		ScarfMID->SetVectorParameterValue(TEXT("EmissiveColor"), bLockedSilhouette ? PitchBlack : (InClimberInfo.ScarfColor * 10.0f));
		for (UStaticMeshComponent* Seg : ScarfSegmentMeshes)
		{
			if (Seg) Seg->SetMaterial(0, ScarfMID);
		}

		UMaterialInstanceDynamic* GhostMID = UMaterialInstanceDynamic::Create(NeonOrBaseMat, this);
		GhostMID->SetVectorParameterValue(TEXT("BaseColor"), InTrailColor);
		GhostMID->SetVectorParameterValue(TEXT("Color"), InTrailColor);
		GhostMID->SetVectorParameterValue(TEXT("EmissiveColor"), InTrailColor * 8.0f);
		for (UStaticMeshComponent* Ghost : GhostTrailMeshes)
		{
			if (Ghost) Ghost->SetMaterial(0, GhostMID);
		}

		GuideRingMID = UMaterialInstanceDynamic::Create(NeonOrBaseMat, this);
		GuideRingMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.961f, 0.831f));
		GuideRingMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.961f, 0.831f) * 8.0f);
		JumpGuideRingMesh->SetMaterial(0, GuideRingMID);

		if (bUsingSkeletalMesh)
		{
			UMaterialInstanceDynamic* HeadbandMID = MakeSolidMID(InClimberInfo.VisorColor, 8.0f);
			if (HeadbandRingMesh && HeadbandMID) HeadbandRingMesh->SetMaterial(0, HeadbandMID);
			if (HeadbandKnotMesh && HeadbandMID) HeadbandKnotMesh->SetMaterial(0, HeadbandMID);
			if (HeadbandRibbonLeftMesh && HeadbandMID) HeadbandRibbonLeftMesh->SetMaterial(0, HeadbandMID);
			if (HeadbandRibbonRightMesh && HeadbandMID) HeadbandRibbonRightMesh->SetMaterial(0, HeadbandMID);
		}
	}

	if (ClimberRimLight)
	{
		if (bLockedSilhouette)
		{
			ClimberRimLight->SetIntensity(0.0f);
		}
		else
		{
			ClimberRimLight->SetRelativeLocation(FVector(0.0f, -95.0f, 28.0f));
			ClimberRimLight->SetLightColor(InClimberInfo.HoodieColor);
			ClimberRimLight->SetIntensity(11500.0f);
			ClimberRimLight->SetAttenuationRadius(550.0f);
		}
	}

	if (ClimberBackLight)
	{
		if (bLockedSilhouette)
		{
			ClimberBackLight->SetIntensity(0.0f);
		}
		else
		{
			ClimberBackLight->SetRelativeLocation(FVector(0.0f, 95.0f, 32.0f));
			ClimberBackLight->SetLightColor(InTrailColor);
			ClimberBackLight->SetIntensity(14500.0f);
			ClimberBackLight->SetAttenuationRadius(650.0f);
		}
	}

	if (!bLockedSilhouette)
	{
		ApplyThemeCharacterCostume(ActiveThemeId);
	}
}

void AVTClimberCharacter::ApplyThemeCharacterCostume(EVTGameThemeId InThemeId)
{
	ActiveThemeId = InThemeId;

	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* CylMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* ConeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	UMaterialInterface* FallbackMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInterface* CustomNeonMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VelocityTower/Materials/M_NeonEmissive_Master.M_NeonEmissive_Master"));
	UMaterialInterface* MatToUse = FallbackMat ? FallbackMat : CustomNeonMat;

	auto SetPartMat = [&](UStaticMeshComponent* Comp, const FLinearColor& Col, float EmissBoost = 0.16f)
	{
		if (!Comp || !MatToUse) return;
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(EmissBoost > 1.0f && CustomNeonMat ? CustomNeonMat : MatToUse, this);
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("BaseColor"), Col);
			MID->SetVectorParameterValue(TEXT("Color"), Col);
			MID->SetVectorParameterValue(TEXT("EmissiveColor"), Col * EmissBoost);
			Comp->SetMaterial(0, MID);
		}
	};

	// Reset default meshes before applying theme-specific 1:1 costume
	if (HeadMesh && SphereMesh) HeadMesh->SetStaticMesh(SphereMesh);
	if (ClimberMesh && CylMesh) ClimberMesh->SetStaticMesh(CylMesh);
	if (HeadgearMesh && ConeMesh) HeadgearMesh->SetStaticMesh(ConeMesh);
	if (HeadgearAccentMesh && CylMesh) HeadgearAccentMesh->SetStaticMesh(CylMesh);

	const bool bIsStylizedThemeHero = (
		InThemeId == EVTGameThemeId::PapercraftToybox ||
		InThemeId == EVTGameThemeId::AncientRuins ||
		InThemeId == EVTGameThemeId::DeepSea ||
		InThemeId == EVTGameThemeId::CandyLand ||
		InThemeId == EVTGameThemeId::PixelDungeon ||
		InThemeId == EVTGameThemeId::WhimsicalToybox
	);

	if (bIsStylizedThemeHero)
	{
		bUsingSkeletalMesh = false;
		if (SkeletalClimberMesh) SkeletalClimberMesh->SetVisibility(false);
		if (ClimberMesh) ClimberMesh->SetVisibility(true, true);
		if (HeadMesh) HeadMesh->SetVisibility(true, true);
		if (HeadgearMesh) HeadgearMesh->SetVisibility(true);
		if (HeadgearAccentMesh) HeadgearAccentMesh->SetVisibility(true);
	}

	// 1. PAPERCRAFT TOYBOX 1:1 -> Handmade Knitted Burlap Ragdoll with Orange Yarn Hair, Button Eyes & Blue/Patchwork Dress
	if (InThemeId == EVTGameThemeId::PapercraftToybox)
	{
		const FLinearColor BurlapSkin(0.84f, 0.68f, 0.52f);
		const FLinearColor YarnHair(0.78f, 0.30f, 0.14f);
		const FLinearColor ButtonEye(0.12f, 0.08f, 0.06f);
		const FLinearColor DenimDress(0.20f, 0.44f, 0.72f);
		const FLinearColor PatchworkPink(0.88f, 0.44f, 0.64f);
		const FLinearColor CuffWhite(0.95f, 0.93f, 0.88f);
		const FLinearColor BootBrown(0.32f, 0.22f, 0.16f);

		if (HeadMesh)
		{
			HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 88.0f));
			HeadMesh->SetRelativeScale3D(FVector(1.12f, 1.18f, 1.05f));
			SetPartMat(HeadMesh, BurlapSkin, 0.15f);
		}
		// Parted orange yarn hair cap
		if (HeadgearMesh && SphereMesh)
		{
			HeadgearMesh->SetStaticMesh(SphereMesh);
			HeadgearMesh->SetRelativeLocation(FVector(-4.0f, 0.0f, 18.0f));
			HeadgearMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, -8.0f));
			HeadgearMesh->SetRelativeScale3D(FVector(1.06f, 1.12f, 0.78f));
			SetPartMat(HeadgearMesh, YarnHair, 0.15f);
		}
		if (HeadgearAccentMesh && SphereMesh)
		{
			HeadgearAccentMesh->SetStaticMesh(SphereMesh);
			HeadgearAccentMesh->SetRelativeLocation(FVector(18.0f, -18.0f, 22.0f));
			HeadgearAccentMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 24.0f));
			HeadgearAccentMesh->SetRelativeScale3D(FVector(0.55f, 0.82f, 0.36f));
			SetPartMat(HeadgearAccentMesh, YarnHair, 0.15f);
		}
		// Cute stitched smile & two black button eyes
		if (VisorWrapMesh)
		{
			VisorWrapMesh->SetVisibility(true);
			VisorWrapMesh->SetRelativeLocation(FVector(46.0f, 0.0f, -12.0f));
			VisorWrapMesh->SetRelativeScale3D(FVector(0.08f, 0.28f, 0.04f));
			SetPartMat(VisorWrapMesh, ButtonEye, 0.05f);
		}
		if (LeftEyeGlowMesh && RightEyeGlowMesh)
		{
			LeftEyeGlowMesh->SetVisibility(true);
			RightEyeGlowMesh->SetVisibility(true);
			LeftEyeGlowMesh->SetRelativeLocation(FVector(12.0f, -68.0f, 320.0f));
			RightEyeGlowMesh->SetRelativeLocation(FVector(12.0f, 68.0f, 320.0f));
			LeftEyeGlowMesh->SetRelativeScale3D(FVector(2.2f, 0.65f, 4.2f));
			RightEyeGlowMesh->SetRelativeScale3D(FVector(2.2f, 0.65f, 4.2f));
			SetPartMat(LeftEyeGlowMesh, ButtonEye, 0.05f);
			SetPartMat(RightEyeGlowMesh, ButtonEye, 0.05f);
		}
		// Patchwork dress torso
		SetPartMat(ClimberMesh, DenimDress, 0.15f);
		if (ChestCoreMesh)
		{
			ChestCoreMesh->SetVisibility(true);
			ChestCoreMesh->SetRelativeLocation(FVector(38.0f, 14.0f, 4.0f));
			ChestCoreMesh->SetRelativeScale3D(FVector(0.45f, 0.42f, 0.22f));
			SetPartMat(ChestCoreMesh, PatchworkPink, 0.15f);
		}
		if (PelvisBeltMesh)
		{
			PelvisBeltMesh->SetVisibility(true);
			SetPartMat(PelvisBeltMesh, FLinearColor(0.24f, 0.30f, 0.42f), 0.12f);
		}
		// Outstretched burlap arms & legs with white cuffs and dark stitched tips
		SetPartMat(LeftShoulderMesh, BurlapSkin, 0.15f);
		SetPartMat(RightShoulderMesh, BurlapSkin, 0.15f);
		SetPartMat(LeftArmMesh, BurlapSkin, 0.15f);
		SetPartMat(RightArmMesh, BurlapSkin, 0.15f);
		SetPartMat(LeftGauntletMesh, BootBrown, 0.12f);
		SetPartMat(RightGauntletMesh, BootBrown, 0.12f);
		SetPartMat(LeftLegMesh, BurlapSkin, 0.15f);
		SetPartMat(RightLegMesh, BurlapSkin, 0.15f);
		SetPartMat(LeftShoeMesh, BootBrown, 0.12f);
		SetPartMat(RightShoeMesh, BootBrown, 0.12f);

		if (BackpackJetMesh) BackpackJetMesh->SetVisibility(false, true);
		for (UStaticMeshComponent* Seg : ScarfSegmentMeshes)
		{
			if (Seg) Seg->SetVisibility(false);
		}
		if (ClimberRimLight)
		{
			ClimberRimLight->SetLightColor(FLinearColor(1.0f, 0.94f, 0.84f));
			ClimberRimLight->SetIntensity(4500.0f);
		}
		if (ClimberBackLight) ClimberBackLight->SetIntensity(2000.0f);
	}
	// 2. ANCIENT FANTASY RUINS 1:1 -> Little Purple Wizard with Pointy Hat & Glowing Golden Spell Orb
	else if (InThemeId == EVTGameThemeId::AncientRuins)
	{
		const FLinearColor WizardRobe(0.42f, 0.28f, 0.62f);
		const FLinearColor WizardHatCol(0.36f, 0.22f, 0.54f);
		const FLinearColor GoldSpellOrb(1.0f, 0.82f, 0.22f);

		SetPartMat(ClimberMesh, WizardRobe, 0.35f);
		SetPartMat(HeadMesh, FLinearColor(0.88f, 0.72f, 0.60f), 0.2f);
		if (HeadgearMesh && ConeMesh)
		{
			HeadgearMesh->SetStaticMesh(ConeMesh);
			HeadgearMesh->SetRelativeLocation(FVector(-4.0f, 0.0f, 64.0f));
			HeadgearMesh->SetRelativeRotation(FRotator(-10.0f, 0.0f, 0.0f));
			HeadgearMesh->SetRelativeScale3D(FVector(0.95f, 0.95f, 1.05f));
			SetPartMat(HeadgearMesh, WizardHatCol, 0.35f);
		}
		if (HeadgearAccentMesh && CylMesh)
		{
			// Wide wizard hat brim
			HeadgearAccentMesh->SetStaticMesh(CylMesh);
			HeadgearAccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 24.0f));
			HeadgearAccentMesh->SetRelativeRotation(FRotator(-6.0f, 0.0f, 0.0f));
			HeadgearAccentMesh->SetRelativeScale3D(FVector(1.85f, 1.85f, 0.06f));
			SetPartMat(HeadgearAccentMesh, WizardHatCol, 0.35f);
		}
		SetPartMat(LeftShoulderMesh, WizardRobe, 0.3f);
		SetPartMat(RightShoulderMesh, WizardRobe, 0.3f);
		SetPartMat(LeftArmMesh, WizardRobe, 0.3f);
		SetPartMat(RightArmMesh, WizardRobe, 0.3f);
		// Glowing golden spell light in right hand!
		SetPartMat(RightGauntletMesh, GoldSpellOrb, 18.0f);
		SetPartMat(LeftGauntletMesh, FLinearColor(0.85f, 0.70f, 0.58f), 0.2f);
		SetPartMat(LeftLegMesh, FLinearColor(0.22f, 0.16f, 0.32f), 0.2f);
		SetPartMat(RightLegMesh, FLinearColor(0.22f, 0.16f, 0.32f), 0.2f);
		SetPartMat(LeftShoeMesh, FLinearColor(0.28f, 0.18f, 0.12f), 0.2f);
		SetPartMat(RightShoeMesh, FLinearColor(0.28f, 0.18f, 0.12f), 0.2f);
		if (BackpackJetMesh) BackpackJetMesh->SetVisibility(false, true);
		for (UStaticMeshComponent* Seg : ScarfSegmentMeshes)
		{
			if (Seg)
			{
				Seg->SetVisibility(true);
				SetPartMat(Seg, WizardRobe, 0.6f);
			}
		}
	}
	// 3. BIOLUMINESCENT DEEP SEA 1:1 -> Scuba Diver with Glowing Goggles, Oxygen Tank & Long Flippers
	else if (InThemeId == EVTGameThemeId::DeepSea)
	{
		const FLinearColor WetsuitCol(0.06f, 0.11f, 0.18f);
		const FLinearColor CyanTrim(0.0f, 0.95f, 0.92f);
		const FLinearColor TankYellow(0.95f, 0.82f, 0.15f);

		SetPartMat(ClimberMesh, WetsuitCol, 0.25f);
		SetPartMat(HeadMesh, WetsuitCol, 0.25f);
		if (HeadgearMesh) HeadgearMesh->SetVisibility(false);
		if (HeadgearAccentMesh) HeadgearAccentMesh->SetVisibility(false);
		if (VisorWrapMesh)
		{
			VisorWrapMesh->SetVisibility(true);
			SetPartMat(VisorWrapMesh, CyanTrim, 10.0f);
		}
		if (BackpackJetMesh)
		{
			BackpackJetMesh->SetVisibility(true, true);
			SetPartMat(BackpackJetMesh, TankYellow, 1.5f);
		}
		// Long scuba swimming flippers
		if (LeftShoeMesh && RightShoeMesh)
		{
			LeftShoeMesh->SetRelativeScale3D(FVector(2.35f, 1.18f, 0.22f));
			RightShoeMesh->SetRelativeScale3D(FVector(2.35f, 1.18f, 0.22f));
			SetPartMat(LeftShoeMesh, CyanTrim, 3.0f);
			SetPartMat(RightShoeMesh, CyanTrim, 3.0f);
		}
		for (UStaticMeshComponent* Seg : ScarfSegmentMeshes)
		{
			if (Seg)
			{
				Seg->SetVisibility(true);
				SetPartMat(Seg, FLinearColor(0.75f, 0.98f, 1.0f), 4.0f); // Bubble stream!
			}
		}
	}
	// 4. CANDY LAND DESSERT 1:1 -> Cute Chubby Bubblegum-Pink Plush/Gummy Bear
	else if (InThemeId == EVTGameThemeId::CandyLand)
	{
		const FLinearColor BearPink(1.0f, 0.46f, 0.70f);
		const FLinearColor BellyCream(1.0f, 0.94f, 0.65f);

		if (HeadMesh)
		{
			HeadMesh->SetRelativeScale3D(FVector(1.18f, 1.24f, 1.08f));
			SetPartMat(HeadMesh, BearPink, 0.25f);
		}
		// Two round pink bear ears
		if (HeadgearMesh && SphereMesh && HeadgearAccentMesh)
		{
			HeadgearMesh->SetStaticMesh(SphereMesh);
			HeadgearAccentMesh->SetStaticMesh(SphereMesh);
			HeadgearMesh->SetRelativeLocation(FVector(0.0f, -38.0f, 44.0f));
			HeadgearAccentMesh->SetRelativeLocation(FVector(0.0f, 38.0f, 44.0f));
			HeadgearMesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 0.42f));
			HeadgearAccentMesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 0.42f));
			SetPartMat(HeadgearMesh, BearPink, 0.25f);
			SetPartMat(HeadgearAccentMesh, BearPink, 0.25f);
		}
		SetPartMat(ClimberMesh, BearPink, 0.25f);
		if (ChestCoreMesh)
		{
			ChestCoreMesh->SetVisibility(true);
			SetPartMat(ChestCoreMesh, BellyCream, 0.3f);
		}
		SetPartMat(LeftShoulderMesh, BearPink, 0.25f);
		SetPartMat(RightShoulderMesh, BearPink, 0.25f);
		SetPartMat(LeftArmMesh, BearPink, 0.25f);
		SetPartMat(RightArmMesh, BearPink, 0.25f);
		SetPartMat(LeftGauntletMesh, BearPink, 0.25f);
		SetPartMat(RightGauntletMesh, BearPink, 0.25f);
		SetPartMat(LeftLegMesh, BearPink, 0.25f);
		SetPartMat(RightLegMesh, BearPink, 0.25f);
		SetPartMat(LeftShoeMesh, BellyCream, 0.25f);
		SetPartMat(RightShoeMesh, BellyCream, 0.25f);
		if (BackpackJetMesh) BackpackJetMesh->SetVisibility(false, true);
		for (UStaticMeshComponent* Seg : ScarfSegmentMeshes)
		{
			if (Seg) Seg->SetVisibility(false);
		}
	}
	// 5. 8-BIT PIXEL DUNGEON & WHIMSICAL TOYBOX 1:1 -> Blocky Voxel Hero / Cardboard-Box Robot Kid
	else if (InThemeId == EVTGameThemeId::PixelDungeon || InThemeId == EVTGameThemeId::WhimsicalToybox)
	{
		const bool bToyRobot = (InThemeId == EVTGameThemeId::WhimsicalToybox);
		const FLinearColor HeadCol = bToyRobot ? FLinearColor(0.78f, 0.58f, 0.36f) : FLinearColor(0.95f, 0.74f, 0.58f);
		const FLinearColor HairCol = bToyRobot ? FLinearColor(0.72f, 0.52f, 0.30f) : FLinearColor(0.42f, 0.24f, 0.12f);
		const FLinearColor ShirtCol = bToyRobot ? FLinearColor(0.88f, 0.20f, 0.18f) : FLinearColor(0.32f, 0.26f, 0.78f);
		const FLinearColor PantsCol = bToyRobot ? FLinearColor(0.18f, 0.44f, 0.78f) : FLinearColor(0.22f, 0.18f, 0.48f);

		if (HeadMesh && CubeMesh)
		{
			HeadMesh->SetStaticMesh(CubeMesh);
			HeadMesh->SetRelativeScale3D(FVector(0.86f, 0.92f, 0.82f));
			SetPartMat(HeadMesh, HeadCol, 0.18f);
		}
		if (ClimberMesh && CubeMesh)
		{
			ClimberMesh->SetStaticMesh(CubeMesh);
			SetPartMat(ClimberMesh, ShirtCol, 0.18f);
		}
		if (HeadgearMesh && CubeMesh)
		{
			HeadgearMesh->SetStaticMesh(CubeMesh);
			HeadgearMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 46.0f));
			HeadgearMesh->SetRelativeScale3D(FVector(0.92f, 0.96f, 0.28f));
			SetPartMat(HeadgearMesh, HairCol, 0.18f);
		}
		if (HeadgearAccentMesh) HeadgearAccentMesh->SetVisibility(false);
		SetPartMat(LeftShoulderMesh, ShirtCol, 0.18f);
		SetPartMat(RightShoulderMesh, ShirtCol, 0.18f);
		SetPartMat(LeftArmMesh, ShirtCol, 0.18f);
		SetPartMat(RightArmMesh, ShirtCol, 0.18f);
		SetPartMat(LeftGauntletMesh, HeadCol, 0.18f);
		SetPartMat(RightGauntletMesh, HeadCol, 0.18f);
		SetPartMat(LeftLegMesh, PantsCol, 0.18f);
		SetPartMat(RightLegMesh, PantsCol, 0.18f);
		SetPartMat(LeftShoeMesh, FLinearColor(0.92f, 0.88f, 0.82f), 0.18f);
		SetPartMat(RightShoeMesh, FLinearColor(0.92f, 0.88f, 0.82f), 0.18f);
		if (BackpackJetMesh) BackpackJetMesh->SetVisibility(false, true);
		for (UStaticMeshComponent* Seg : ScarfSegmentMeshes)
		{
			if (Seg) Seg->SetVisibility(false);
		}
	}
	else
	{
		// Sci-Fi / Synthwave / Industrial / Cyberpunk Runner: ensure scarf is visible
		for (UStaticMeshComponent* Seg : ScarfSegmentMeshes)
		{
			if (Seg) Seg->SetVisibility(true);
		}
	}
}

void AVTClimberCharacter::UpdateShowroomStagePose(float TimeSec, float ShowroomYawDeg, bool bInCharSelect)
{
	if (ShowroomPedestalMesh) ShowroomPedestalMesh->SetVisibility(bInCharSelect);
	if (ShowroomHoloRingMesh)
	{
		ShowroomHoloRingMesh->SetVisibility(bInCharSelect);
		const float RingPulse = 1.32f + FMath::Sin(TimeSec * 4.5f) * 0.08f;
		ShowroomHoloRingMesh->SetRelativeScale3D(FVector(RingPulse, RingPulse, 0.025f));
	}
	if (JumpGuideRingMesh) JumpGuideRingMesh->SetVisibility(false);

	// Heroic breathing & articulated limb animation on the 3D Showroom Stage
	const float BreathZ = FMath::Sin(TimeSec * 3.2f) * 2.5f;
	SetActorLocation(FVector(0.0f, 0.0f, (bInCharSelect ? 48.0f : 44.0f) + BreathZ));

	const float StageScale = bInCharSelect ? 1.12f : 1.0f;
	ClimberMesh->SetRelativeRotation(FRotator(0.0f, ShowroomYawDeg, 0.0f));
	ClimberMesh->SetRelativeScale3D(FVector(0.38f * StageScale, 0.28f * StageScale, 0.36f * StageScale));

	if (bUsingSkeletalMesh && SkeletalClimberMesh)
	{
		SkeletalClimberMesh->SetRelativeRotation(FRotator(0.0f, ShowroomYawDeg, 0.0f));
		SkeletalClimberMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -44.0f));
		SkeletalClimberMesh->SetRelativeScale3D(FVector(0.50f * StageScale));
		PlaySkeletalAnim(AnimMannyIdle, true, 1.0f);

		if (HeadbandRingMesh) HeadbandRingMesh->SetVisibility(false);
		if (HeadbandKnotMesh) HeadbandKnotMesh->SetVisibility(false);
		if (HeadbandRibbonLeftMesh) HeadbandRibbonLeftMesh->SetVisibility(false);
		if (HeadbandRibbonRightMesh) HeadbandRibbonRightMesh->SetVisibility(false);
	}

	if (HeadMesh)
	{
		const float HeadNod = FMath::Sin(TimeSec * 2.4f) * 5.0f;
		const float HeadTurn = FMath::Cos(TimeSec * 1.6f) * 8.0f;
		HeadMesh->SetRelativeRotation(FRotator(HeadNod, HeadTurn, 0.0f));
	}
	if (LeftShoulderMesh && RightShoulderMesh)
	{
		const float ArmSwing = FMath::Sin(TimeSec * 3.2f) * 10.0f;
		LeftShoulderMesh->SetRelativeRotation(FRotator(12.0f + ArmSwing, 0.0f, 14.0f));
		RightShoulderMesh->SetRelativeRotation(FRotator(-12.0f - ArmSwing, 0.0f, -14.0f));
	}
	if (LeftLegMesh && RightLegMesh)
	{
		LeftLegMesh->SetRelativeRotation(FRotator(6.0f, -8.0f, 4.0f));
		RightLegMesh->SetRelativeRotation(FRotator(-6.0f, 8.0f, -4.0f));
	}

	// Update flowing 3D Scarf on stage
	for (int32 i = 0; i < ScarfSegmentMeshes.Num(); ++i)
	{
		if (ScarfSegmentMeshes[i])
		{
			const float WaveX = FMath::Sin(TimeSec * 5.0f + float(i) * 0.6f) * (4.0f + float(i) * 2.2f);
			const float BaseZ = (bInCharSelect ? 76.0f : 44.0f) + 18.0f - float(i) * 5.5f;
			ScarfSegmentMeshes[i]->SetWorldLocation(FVector(WaveX - float(i) * 4.0f, 16.0f + float(i) * 2.5f, BaseZ));
		}
	}
}

void AVTClimberCharacter::SetShieldVisualActive(bool bActive)
{
	// NEVER show an opaque sphere that conceals the player character into a metallic ball!
	if (ShieldSphereMesh)
	{
		ShieldSphereMesh->SetVisibility(false);
	}
	if (ClimberRimLight)
	{
		ClimberRimLight->SetLightColor(bActive ? FLinearColor(0.0f, 1.0f, 0.85f) : FLinearColor(0.0f, 0.961f, 0.831f));
		ClimberRimLight->SetIntensity(bActive ? 14000.0f : 6200.0f);
	}
}

void AVTClimberCharacter::PlayAbyssFallPose(float FallProgress, float DeltaTime)
{
	if (ShowroomPedestalMesh) ShowroomPedestalMesh->SetVisibility(false);
	if (ShowroomHoloRingMesh) ShowroomHoloRingMesh->SetVisibility(false);
	if (JumpGuideRingMesh) JumpGuideRingMesh->SetVisibility(false);
	if (ShieldSphereMesh) ShieldSphereMesh->SetVisibility(false);

	SetActorLocation(FVector(PosX, 0.0f, PosZ));

	// Frantic tumbling spin & shrinking depth scaling as character plunges into the dark abyss
	const float TumblePitch = FMath::RadiansToDegrees(FlipAngleRad);
	const float TumbleYaw = FMath::Sin(FallProgress * 28.0f) * 45.0f;
	const float TumbleRoll = FMath::Cos(FallProgress * 32.0f) * 60.0f;
	ClimberMesh->SetRelativeRotation(FRotator(TumblePitch, TumbleYaw, TumbleRoll));

	const float DepthScale = FMath::Max(0.06f, 1.0f - FallProgress * 0.82f);
	ClimberMesh->SetRelativeScale3D(FVector(0.38f * DepthScale, 0.28f * DepthScale, 0.36f * DepthScale));

	if (bUsingSkeletalMesh && SkeletalClimberMesh)
	{
		SkeletalClimberMesh->SetRelativeRotation(FRotator(TumblePitch, TumbleYaw, TumbleRoll));
		SkeletalClimberMesh->SetRelativeScale3D(FVector(0.50f * DepthScale));
		PlaySkeletalAnim(AnimMannyFall, true, 1.0f);
	}

	// Frantic limb flailing in panic!
	if (LeftShoulderMesh && RightShoulderMesh && LeftLegMesh && RightLegMesh)
	{
		const float FlailArm1 = FMath::Sin(FallProgress * 40.0f) * 45.0f;
		const float FlailArm2 = FMath::Cos(FallProgress * 44.0f) * 45.0f;
		LeftShoulderMesh->SetRelativeRotation(FRotator(130.0f + FlailArm1, 0.0f, 35.0f));
		RightShoulderMesh->SetRelativeRotation(FRotator(130.0f + FlailArm2, 0.0f, -35.0f));

		const float KickLeg1 = FMath::Sin(FallProgress * 35.0f) * 40.0f;
		const float KickLeg2 = FMath::Cos(FallProgress * 38.0f) * 40.0f;
		LeftLegMesh->SetRelativeRotation(FRotator(-20.0f + KickLeg1, 0.0f, -15.0f));
		RightLegMesh->SetRelativeRotation(FRotator(-20.0f + KickLeg2, 0.0f, 15.0f));
	}

	if (HeadMesh)
	{
		// Looking down in panic
		HeadMesh->SetRelativeRotation(FRotator(-35.0f, FMath::Sin(FallProgress * 30.0f) * 15.0f, 0.0f));
	}

	// Trailing scarf falling behind
	for (int32 i = 0; i < ScarfSegmentMeshes.Num(); ++i)
	{
		if (ScarfSegmentMeshes[i])
		{
			const float LagZ = PosZ + 20.0f + float(i) * 12.0f;
			const float FlutterX = PosX + FMath::Sin(FallProgress * 30.0f + float(i)) * (8.0f + float(i) * 4.0f);
			ScarfSegmentMeshes[i]->SetWorldLocation(FVector(FlutterX, 0.0f, LagZ));
		}
	}
}

void AVTClimberCharacter::UpdateVisualState(float DeltaTime, bool bShowJumpGuide, float SpeedRatio, bool bNearWallReady)
{
	if (ShowroomPedestalMesh) ShowroomPedestalMesh->SetVisibility(false);
	if (ShowroomHoloRingMesh) ShowroomHoloRingMesh->SetVisibility(false);

	SetActorLocation(FVector(PosX, 0.0f, PosZ));

	// Convert FlipAngleRad & FacingDir into 3D rotation
	const float PitchDeg = FMath::RadiansToDegrees(FlipAngleRad);
	const bool bFrontFacingDoll =
		(ActiveThemeId == EVTGameThemeId::PapercraftToybox ||
		 ActiveThemeId == EVTGameThemeId::CandyLand ||
		 ActiveThemeId == EVTGameThemeId::PixelDungeon ||
		 ActiveThemeId == EVTGameThemeId::WhimsicalToybox);
	const float YawDeg = bFrontFacingDoll
		? (-90.0f + float(FacingDir) * 18.0f)
		: ((FacingDir >= 0) ? 18.0f : 162.0f);
	const float RunBobRoll = bOnGround ? FMath::Sin(RunPhase) * 8.0f : 0.0f;

	if (bUsingSkeletalMesh && SkeletalClimberMesh)
	{
		// SKELETAL METAHUMAN / NEXT-GEN MANNY VISUAL UPDATE
		if (bFlipping)
		{
			// Acrobatic Parkour Somersault Roll Flip with Cowered / Tucked Body (Zusammengekauert)
			// Compress body into a tight aerodynamic tuck ball centered at rotation pivot
			SkeletalClimberMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
			SkeletalClimberMesh->SetRelativeRotation(FRotator(PitchDeg, YawDeg, 0.0f));
			// Tight spherical tuck scale: Body curls tight into a compact ball
			SkeletalClimberMesh->SetRelativeScale3D(FVector(0.40f, 0.38f, 0.30f));
			PlaySkeletalAnim(AnimMannyLand ? AnimMannyLand : (AnimMannyRoll ? AnimMannyRoll : AnimMannyJump), false, 1.4f);

			if (HeadbandRingMesh) HeadbandRingMesh->SetVisibility(false);
			if (HeadbandRibbonLeftMesh) HeadbandRibbonLeftMesh->SetVisibility(false);
			if (HeadbandRibbonRightMesh) HeadbandRibbonRightMesh->SetVisibility(false);
		}
		else
		{
			SkeletalClimberMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -44.0f));
			SkeletalClimberMesh->SetRelativeRotation(FRotator(0.0f, YawDeg, RunBobRoll * 0.35f));
			SkeletalClimberMesh->SetRelativeScale3D(FVector(0.50f));

			if (!bOnGround)
			{
				if (VelZ > 10.0f)
				{
					PlaySkeletalAnim(AnimMannyJump, false, 1.25f);
				}
				else
				{
					PlaySkeletalAnim(AnimMannyFall, true, 1.0f);
				}
			}
			else
			{
				if (FMath::Abs(VelX) > 10.0f)
				{
					PlaySkeletalAnim(AnimMannyRun, true, FMath::Clamp(SpeedRatio * 1.5f, 0.85f, 2.1f));
				}
				else
				{
					PlaySkeletalAnim(AnimMannyIdle, true, 1.0f);
				}
			}
		}

		// Update Shinobi Headband Position & Trailing Inertia Ribbon Physics on MetaHuman
		if (HeadbandRingMesh)
		{
			const FVector HeadPos = SkeletalClimberMesh->DoesSocketExist(TEXT("head"))
				? SkeletalClimberMesh->GetSocketLocation(TEXT("head")) + FVector(float(FacingDir) * 1.5f, 0.0f, 4.0f)
				: FVector(PosX + float(FacingDir) * 1.5f, 0.0f, PosZ + 38.0f);

			HeadbandRingMesh->SetWorldLocation(HeadPos);
			HeadbandRingMesh->SetWorldRotation(FRotator(PitchDeg, YawDeg, 0.0f));

			if (HeadbandRibbonLeftMesh && HeadbandRibbonRightMesh)
			{
				// Realistic spring-damper ribbon flutter reacting to jump/fall inertia
				const float InertiaPitch = FMath::Clamp(-VelZ * 0.09f, -46.0f, 46.0f);
				const float WindFlutter = FMath::Sin(GetWorld()->GetTimeSeconds() * 16.0f) * (5.0f + FMath::Abs(VelX) * 0.12f);
				const float Flutter2 = FMath::Cos(GetWorld()->GetTimeSeconds() * 20.0f + 1.2f) * (6.0f + FMath::Abs(VelX) * 0.15f);

				const float Damping = 0.84f;
				const float SpringStiffness = 16.0f;
				RibbonVelPitch += (InertiaPitch - RibbonPhysicsPitch) * SpringStiffness * DeltaTime;
				RibbonVelPitch *= Damping;
				RibbonPhysicsPitch += RibbonVelPitch * DeltaTime;

				HeadbandRibbonLeftMesh->SetRelativeRotation(FRotator(RibbonPhysicsPitch - 15.0f + WindFlutter, -8.0f, WindFlutter * 0.4f));
				HeadbandRibbonRightMesh->SetRelativeRotation(FRotator(RibbonPhysicsPitch - 24.0f + Flutter2, 10.0f, Flutter2 * 0.4f));
			}
		}
	}
	else if (ClimberMesh)
	{
		// ARTICULATED CYBER-RUNNER DUMMY WITH AUTHENTIC SOMERSAULT TUCK-AND-ROLL (NO BALL!)
		ClimberMesh->SetRelativeRotation(FRotator(PitchDeg, YawDeg, RunBobRoll));

		if (LeftShoulderMesh && RightShoulderMesh && LeftLegMesh && RightLegMesh)
		{
			if (bFlipping)
			{
				// Acrobatic Parkour Somersault Tuck Pose (Knees to chest, arms gripping shins, head tucked)
				ClimberMesh->SetRelativeScale3D(FVector(0.38f, 0.28f, 0.36f)); // Stable athletic proportions

				LeftLegMesh->SetRelativeLocation(FVector(10.0f, -22.0f, -38.0f));
				RightLegMesh->SetRelativeLocation(FVector(10.0f, 22.0f, -38.0f));
				LeftLegMesh->SetRelativeRotation(FRotator(95.0f, 0.0f, 0.0f));
				RightLegMesh->SetRelativeRotation(FRotator(95.0f, 0.0f, 0.0f));

				LeftShoulderMesh->SetRelativeLocation(FVector(12.0f, -52.0f, 18.0f));
				RightShoulderMesh->SetRelativeLocation(FVector(12.0f, 52.0f, 18.0f));
				LeftShoulderMesh->SetRelativeRotation(FRotator(72.0f, 0.0f, 16.0f));
				RightShoulderMesh->SetRelativeRotation(FRotator(72.0f, 0.0f, -16.0f));
				LeftArmMesh->SetRelativeRotation(FRotator(40.0f, 0.0f, 20.0f));
				RightArmMesh->SetRelativeRotation(FRotator(40.0f, 0.0f, -20.0f));

				if (HeadMesh) HeadMesh->SetRelativeRotation(FRotator(25.0f, 0.0f, 0.0f));
			}
			else
			{
				// Reset limbs to natural anatomical origins
				ClimberMesh->SetRelativeScale3D(FVector(0.38f * SquashScaleX, 0.28f * SquashScaleX, 0.36f * SquashScaleZ));

				LeftLegMesh->SetRelativeLocation(FVector(0.0f, -28.0f, -78.0f));
				RightLegMesh->SetRelativeLocation(FVector(0.0f, 28.0f, -78.0f));
				LeftShoulderMesh->SetRelativeLocation(FVector(0.0f, -66.0f, 32.0f));
				RightShoulderMesh->SetRelativeLocation(FVector(0.0f, 66.0f, 32.0f));
				if (HeadMesh) HeadMesh->SetRelativeRotation(FRotator::ZeroRotator);

				if (!bOnGround)
				{
					// Heroic Aerial Jump Pose (cheerful outstretched ragdoll/bear arms when bFrontFacingDoll)
					if (bFrontFacingDoll)
					{
						LeftShoulderMesh->SetRelativeRotation(FRotator(18.0f, 0.0f, 44.0f));
						RightShoulderMesh->SetRelativeRotation(FRotator(18.0f, 0.0f, -44.0f));
						LeftLegMesh->SetRelativeRotation(FRotator(14.0f, 0.0f, 18.0f));
						RightLegMesh->SetRelativeRotation(FRotator(14.0f, 0.0f, -18.0f));
					}
					else
					{
						LeftShoulderMesh->SetRelativeRotation(FRotator(42.0f, 0.0f, 22.0f));
						RightShoulderMesh->SetRelativeRotation(FRotator(-35.0f, 0.0f, -22.0f));
						LeftLegMesh->SetRelativeRotation(FRotator(-24.0f, 0.0f, 0.0f));
						RightLegMesh->SetRelativeRotation(FRotator(28.0f, 0.0f, 0.0f));
					}
				}
				else
				{
					const float Stride = FMath::Sin(RunPhase * 1.4f) * (10.0f + SpeedRatio * 34.0f);
					LeftShoulderMesh->SetRelativeRotation(FRotator(-Stride, 0.0f, 10.0f));
					RightShoulderMesh->SetRelativeRotation(FRotator(Stride, 0.0f, -10.0f));
					LeftLegMesh->SetRelativeRotation(FRotator(Stride, 0.0f, 0.0f));
					RightLegMesh->SetRelativeRotation(FRotator(-Stride, 0.0f, 0.0f));
				}
			}
		}
	}

	// Update 7-segment 3D Scarf positions in world space
	for (int32 i = 0; i < ScarfSegmentMeshes.Num(); ++i)
	{
		if (ScarfSegmentMeshes[i] && ScarfPoints.IsValidIndex(i))
		{
			ScarfSegmentMeshes[i]->SetWorldLocation(ScarfPoints[i]);
		}
	}

	// Note: Ghost spheres removed to prevent dummy ball artifact during rolls!

	// Kinetic Jump Guide Ring under Climber (only in Cyber/Synthwave/Sci-Fi themes)
	if (JumpGuideRingMesh)
	{
		const bool bAllowNeonGuideRing =
			(ActiveThemeId == EVTGameThemeId::CyberpunkDefault ||
			 ActiveThemeId == EVTGameThemeId::RetroSynthwave ||
			 ActiveThemeId == EVTGameThemeId::DeepSpaceOrbit ||
			 ActiveThemeId == EVTGameThemeId::IndustrialForge);
		const bool bCanGroundJump = bOnGround || CoyoteFrames > 0;
		if (bAllowNeonGuideRing && bShowJumpGuide && bCanGroundJump)
		{
			JumpGuideRingMesh->SetVisibility(true);
			const float Pulse = (FMath::Sin(GetWorld()->GetTimeSeconds() * 15.0f) + 1.0f) * 0.5f;
			const float RingRadiusScale = 0.75f + SpeedRatio * 0.55f + Pulse * 0.18f;
			JumpGuideRingMesh->SetRelativeScale3D(FVector(RingRadiusScale, RingRadiusScale, 0.04f));
			if (GuideRingMID)
			{
				const FLinearColor GuideCol = bNearWallReady
					? FLinearColor(1.0f, 0.82f, 0.4f)
					: (SpeedRatio > 0.6f ? FLinearColor(1.0f, 0.745f, 0.043f) : FLinearColor(0.0f, 0.961f, 0.831f));
				GuideRingMID->SetVectorParameterValue(TEXT("BaseColor"), GuideCol);
				GuideRingMID->SetVectorParameterValue(TEXT("EmissiveColor"), GuideCol * (10.0f + Pulse * 8.0f));
			}
		}
		else
		{
			JumpGuideRingMesh->SetVisibility(false);
		}
	}
}

void AVTClimberCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

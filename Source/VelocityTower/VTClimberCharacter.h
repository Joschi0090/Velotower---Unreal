#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VelocityTowerTypes.h"
#include "VTClimberCharacter.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class UAnimSequence;
class UPointLightComponent;
class UMaterialInstanceDynamic;

UCLASS()
class VELOCITYTOWER_API AVTClimberCharacter : public APawn
{
	GENERATED_BODY()

public:
	AVTClimberCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void ResetClimberForStart(float StartX, float StartZ, int32 InStartFloor, const FVTClimberInfo& InClimberInfo, const FLinearColor& InTrailColor);
	void ApplyClimberVisuals(const FVTClimberInfo& InClimberInfo, const FLinearColor& InTrailColor, bool bLockedSilhouette = false);
	void SetShieldVisualActive(bool bActive);
	void UpdateVisualState(float DeltaTime, bool bShowJumpGuide, float SpeedRatio, bool bNearWallReady);
	void UpdateShowroomStagePose(float TimeSec, float ShowroomYawDeg, bool bInCharSelect);
	void PlayAbyssFallPose(float FallProgress, float DeltaTime);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	USceneComponent* RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	USkeletalMeshComponent* SkeletalClimberMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	bool bUsingSkeletalMesh = false;

	UPROPERTY()
	UAnimSequence* AnimMannyIdle = nullptr;

	UPROPERTY()
	UAnimSequence* AnimMannyRun = nullptr;

	UPROPERTY()
	UAnimSequence* AnimMannyJump = nullptr;

	UPROPERTY()
	UAnimSequence* AnimMannyFall = nullptr;

	UPROPERTY()
	UAnimSequence* AnimMannyLand = nullptr;

	UPROPERTY()
	UAnimSequence* AnimMannyRoll = nullptr;

	UPROPERTY(Transient)
	UAnimSequence* CurrentPlayingAnim = nullptr;

	void PlaySkeletalAnim(UAnimSequence* NewAnim, bool bLoop, float PlayRate = 1.0f);

	// Physics-Simulated Flowing Shinobi Headband (wraps MetaHuman forehead, flutters with jump inertia)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* HeadbandRingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* HeadbandKnotMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* HeadbandRibbonLeftMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* HeadbandRibbonRightMesh;

	float RibbonPhysicsPitch = 0.0f;
	float RibbonPhysicsYaw = 0.0f;
	float RibbonVelPitch = 0.0f;
	float RibbonVelYaw = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* ClimberMesh;

	// Detailed Articulated 3D Cyber-Runner Anatomy & Gear Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* VisorWrapMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* LeftEyeGlowMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* RightEyeGlowMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* HeadgearMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* HeadgearAccentMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* ChestCoreMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* BackpackJetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* LeftThrusterMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* RightThrusterMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* LeftShoulderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* RightShoulderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* LeftArmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* RightArmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* LeftGauntletMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* RightGauntletMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* PelvisBeltMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* LeftLegMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* RightLegMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* LeftShoeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* RightShoeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* ShowroomPedestalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* ShowroomHoloRingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* ShieldSphereMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UStaticMeshComponent* JumpGuideRingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	UPointLightComponent* ClimberRimLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	TArray<UStaticMeshComponent*> ScarfSegmentMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Climber")
	TArray<UStaticMeshComponent*> GhostTrailMeshes;

	// Physics & State Variables matching HTML 1:1 (scaled 1px = 2.0cm)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	float PosX = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	float PosZ = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	float VelX = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	float VelZ = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	float RadiusCm = 30.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	float HalfHeightCm = 44.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	int32 FacingDir = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	bool bOnGround = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	int32 CurrentFloor = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	float FlipAngleRad = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	bool bFlipping = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	float RunPhase = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	float SquashScaleX = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	float SquashScaleZ = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	int32 CoyoteFrames = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	bool bWallBounceUsed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Physics")
	bool bWasOverdrive = false;

	TArray<FVector> ScarfPoints;
	TArray<FVector> GhostLocations;
	TArray<FRotator> GhostRotations;
	TArray<float> GhostAlphas;
	int32 NextGhostIndex = 0;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> ClimberMIDs;

	UPROPERTY()
	UMaterialInstanceDynamic* ScarfMID = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* ShieldMID = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* GuideRingMID = nullptr;
};

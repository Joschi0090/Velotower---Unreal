#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VelocityTowerTypes.h"
#include "VTPlatformActor.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;

UCLASS()
class VELOCITYTOWER_API AVTPlatformActor : public AActor
{
	GENERATED_BODY()

public:
	AVTPlatformActor();

	virtual void Tick(float DeltaTime) override;

	void InitializePlatform(
		int32 InFloor,
		float InX,
		float InZ,
		float InWidth,
		float InVx,
		EVTPlatformType InPlatType,
		bool bInCheckpoint,
		EVTItemType InItemType,
		const FVTBiomeInfo& InBiome,
		float InCrackDuration
	);

	void TriggerLandingTouch();
	void ShatterPlatform();
	void CollectItem();
	void ApplyTheme(const FVTGameThemeInfo& Theme);

	bool IsSolidAtTime(float CurrentTimeSeconds) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	UStaticMeshComponent* PlatformMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	UStaticMeshComponent* TopNeonTrimMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	UStaticMeshComponent* CollectibleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	UPointLightComponent* NeonLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	UTextRenderComponent* FloorLabelText;

	UPROPERTY()
	TArray<UStaticMeshComponent*> ThemeDecorations;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> ThemeDecorationMIDs;

	EVTGameThemeId CurrentThemeId = EVTGameThemeId::CyberpunkDefault;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	int32 FloorIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	float PlatformX = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	float PlatformZ = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	float PlatformWidth = 240.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	float VelocityX = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	EVTPlatformType PlatType = EVTPlatformType::Normal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	bool bIsCheckpoint = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	bool bTouched = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	bool bBroken = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	float CrackTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	float CrackDuration = 1.35f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	EVTItemType ItemType = EVTItemType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	bool bItemCollected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	FVector ItemWorldLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Platform")
	FVTBiomeInfo BiomeInfo;

	UPROPERTY()
	UMaterialInstanceDynamic* BodyMID = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* TrimMID = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* ItemMID = nullptr;
};

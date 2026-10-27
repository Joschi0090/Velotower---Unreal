#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "VelocityTowerTypes.h"
#include "VTSaveGame.generated.h"

UCLASS()
class VELOCITYTOWER_API UVTSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UVTSaveGame();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	int32 BestScore = 69079;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	int32 MaxFloorRecord = 125;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	int32 NeonCores = 65;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	int32 TotalCoresEarned = 65;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	int32 PlayerLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	int32 PlayerXP = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	EVTControlMode ControlMode = EVTControlMode::ClassicButtons;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	EVTGameModeType GameMode = EVTGameModeType::Arcade;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	float BaseSpeedRatio = 0.80f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	EVTClimberId SelectedClimber = EVTClimberId::KidFlex;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	FString SelectedTrail = TEXT("cyan");

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	EVTGameThemeId SelectedTheme = EVTGameThemeId::CyberpunkDefault;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	TArray<EVTGameThemeId> UnlockedThemes;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	int32 StartFloor = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	bool bIsVip = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	bool bAudioMuted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	bool bVoiceEnabled = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	FVTHudConfig HudConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	TArray<EVTClimberId> UnlockedClimbers;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	bool bMigratedTo20Roster = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	TArray<FString> UnlockedTrails;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	TArray<FString> UnlockedAchievements;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	TArray<FString> UnlockedVoiceLines;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	TArray<FString> ClaimedPartnerDeals;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	TArray<FVTQuest> DailyQuests;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	int32 StreakDayCount = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	FString LastStreakClaimDate;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	FString LastWheelSpinDate;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	FString PlayerNickname = TEXT("ApexClimber");

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	FString PlayerTag = TEXT("APEX-2026");

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	FString ActiveAuthProvider = TEXT("GAST-MODUS");

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	TArray<FString> LinkedAuthProviders;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	TArray<FVTFriendEntry> FriendsList;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	bool bLinkedGooglePlay = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	bool bLinkedFacebook = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	bool bLinkedInstagram = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	bool bLinkedEmail = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Save")
	bool bRatedApp = false;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VelocityTowerTypes.h"
#include "VTTowerGameMode.generated.h"

class AVTClimberCharacter;
class AVTPlatformActor;
class UVTSaveGame;
class ACameraActor;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UAudioComponent;
class USoundWaveProcedural;

USTRUCT()
struct FVTPcmClip
{
	GENERATED_BODY()

	TArray<int16> StereoSamples; // Interleaved L, R 44.1kHz 16-bit PCM
	int32 NumFrames = 0;
};

USTRUCT()
struct FVTActiveSfxVoice
{
	GENERATED_BODY()

	FString Key;
	double ReadFramePos = 0.0;
	float Pitch = 1.0f;
	float Volume = 1.0f;
	int32 Priority = 0;
};

USTRUCT()
struct FVTIceShardSim
{
	GENERATED_BODY()

	UPROPERTY()
	UStaticMeshComponent* MeshComp = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* MID = nullptr;

	FVector Pos = FVector::ZeroVector;
	FVector Vel = FVector::ZeroVector;
	FRotator Rot = FRotator::ZeroRotator;
	FRotator RotVel = FRotator::ZeroRotator;
	float Life = 0.0f;
};

USTRUCT()
struct FVTShockwaveSim
{
	GENERATED_BODY()

	UPROPERTY()
	UStaticMeshComponent* MeshComp = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* MID = nullptr;

	FVector Pos = FVector::ZeroVector;
	float Radius = 10.0f;
	float MaxRadius = 100.0f;
	float Life = 0.0f;
};

USTRUCT(BlueprintType)
struct FVTFloatingTextEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Text;

	UPROPERTY(BlueprintReadOnly)
	FVector WorldPos = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly)
	int32 FontSize = 14;

	UPROPERTY(BlueprintReadOnly)
	float Life = 1.0f;

	UPROPERTY(BlueprintReadOnly)
	float InitialLife = 1.0f;
};

UCLASS()
class VELOCITYTOWER_API AVTTowerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVTTowerGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// --- Boot Flow, Splash, Teaser Intro & Account Gate ---
	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Boot")
	void SkipToTeaserIntro();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Boot")
	void SkipToAccountGate();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Boot")
	void CompleteAccountLogin(const FString& ProviderKey, const FString& DisplayName, const FString& OptionalUsername = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Boot")
	void ReplayTeaserIntro();

	// --- Core Game Actions ---
	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Game")
	void StartNewRun();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Game")
	void ReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Game")
	void ExecuteSecondChanceRevive();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Game")
	void EndCurrentRun(const FString& InBadgeText, const FString& InReasonSubtext);

	// --- Economy, 20-Character Roster, Shop, Quests, Trophies, Wheel & Streak ---
	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	void AwardNeonCores(int32 Amount, const FString& Reason);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	void RecordQuestProgress(const FString& QuestId, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	bool ClaimQuestReward(int32 QuestIndex);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	void UnlockAchievementById(const FString& AchievementId);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	bool ClaimDailyStreakReward();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	FString SpinFortuneWheel(bool bFreeSpin);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	bool IsClimberUnlocked(EVTClimberId ClimberId) const;

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	int32 GetUnlockedClimberCount() const;

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	bool BuyOrEquipClimber(EVTClimberId ClimberId);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	void PreviewClimberOnStage(EVTClimberId ClimberId);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	bool BuyOrEquipTrail(const FString& TrailId);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	void ToggleVipStatus();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	bool LinkSocialAccount(const FString& ProviderKey, const FString& DisplayName);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	void CycleControlMode();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	void SetControlModeDirect(EVTControlMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	void CycleSpeedPreset();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	void SelectClimberDirection(int32 StepDir);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Themes")
	bool BuyOrEquipTheme(EVTGameThemeId ThemeId);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Themes")
	void PreviewThemeOnStage(EVTGameThemeId ThemeId);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Themes")
	void SelectThemeDirection(int32 StepDir);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Themes")
	bool IsThemeUnlocked(EVTGameThemeId Id) const;

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Themes")
	int32 GetUnlockedThemeCount() const;

	FVTGameThemeInfo GetThemeInfo(EVTGameThemeId Id) const;
	void ApplyThemeVisuals(const FVTGameThemeInfo& ThemeInfo);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Economy")
	void SaveGameToDisk();

	// --- Audio & Dopamine Reward System ---
	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Audio")
	void PlayTowerSFX(const FString& SfxKey, float PitchMult = 1.0f, float VolumeMult = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Audio")
	void TriggerAnnouncerVoice(const FString& VoiceKey, const FString& DisplayCallout);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Audio")
	int32 GetVoicePriority(const FString& VoiceKey) const;

	UPROPERTY(BlueprintReadOnly, Category = "VelocityTower|Audio")
	float LastAnnouncerVoiceTime = -100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VelocityTower|Audio")
	float LastClutchVoiceTime = -100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VelocityTower|Audio")
	float LastWallVoiceTime = -100.0f;

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Audio")
	int32 GetUnlockedVoiceCount() const;

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Audio")
	void ToggleAudioMute();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Audio")
	void ToggleVoiceEnabled();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Progression")
	void AwardPlayerXP(int32 Amount, const FString& SourceLabel);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Progression")
	FString GetPlayerRankTitle() const;

	// --- Developer Admin / Godmode Controls (Hidden from normal users) ---
	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Admin")
	void AdminWarpFloors(int32 DeltaFloors);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Admin")
	void AdminUnlockAllClimbers();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Admin")
	void AdminResetRosterToStarter();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Admin")
	void AdminTriggerMegaCombo(int32 TargetCombo = 25);

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Admin")
	void AdminResetPhysicsDefaults();

	// --- Helpers ---
	FVTBiomeInfo GetBiomeForFloor(int32 Floor) const;
	FVTFloorScaling GetFloorScaling(int32 Floor) const;
	FVTClimberInfo GetClimberInfo(EVTClimberId Id) const;
	FVTTrailInfo GetTrailInfo(const FString& Id) const;

	// --- Boot & Live Telemetry State ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Boot")
	EVTBootFlowState BootFlowState = EVTBootFlowState::Ready;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Boot")
	float BootTimerSec = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	EVTGameState TowerRunState = EVTGameState::Menu;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	bool bIsGamePaused = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	EVTControlMode CurrentControlMode = EVTControlMode::ClassicButtons;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	EVTGameModeType CurrentGameMode = EVTGameModeType::Arcade;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	float BaseSpeedRatio = 0.80f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	EVTClimberId SelectedClimberId = EVTClimberId::KidFlex;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	EVTClimberId PreviewClimberId = EVTClimberId::KidFlex;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	bool bInCharacterSelectStage = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	EVTGameThemeId SelectedThemeId = EVTGameThemeId::PapercraftToybox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	EVTGameThemeId PreviewThemeId = EVTGameThemeId::PapercraftToybox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	bool bInThemeSelectStage = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	FString SelectedTrailId = TEXT("cyan");

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 StartFloor = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 CurrentScore = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 BonusScoreAccumulated = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 LastComboVoiceTier = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	float NearDeathVoiceCooldownSec = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	bool bNewHighScoreAnnouncedThisRun = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 BestScore = 69079;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 MaxFloor = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 CurrentCombo = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 MaxCombo = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 ComboTimerFrames = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 CurrentBpm = 120;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 PeakBpm = 120;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 NeonCores = 65;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 HurryLevel = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 HurryTimerFrames = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	float HurryBannerTimerSec = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	int32 BlitzFramesRemaining = 3600;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	float CameraZ = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	float LeftWallFlash = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	float RightWallFlash = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	float ScreenShake = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	float BeatPulse = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|State")
	float EnergyBarPercent = 0.0f;

	// Powerups
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|PowerUps")
	int32 MagnetFrames = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|PowerUps")
	int32 SpringJumpsLeft = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|PowerUps")
	int32 FreezeFrames = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|PowerUps")
	int32 ShieldCount = 0;

	// Game Over & Revive state
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|GameOver")
	FString GameOverBadge = TEXT("TOWER COLLAPSED");

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|GameOver")
	FString GameOverSubtext = TEXT("Die Kamera hat dich eingeholt!");

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|GameOver")
	bool bHasRevivedThisRun = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|GameOver")
	float ReviveSecondsRemaining = 5.0f;

	// Toast Notification
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|UI")
	FString LatestToastMessage;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|UI")
	float ToastTimerSec = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|UI")
	FString LatestAnnouncerCallout;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|UI")
	float AnnouncerTimerSec = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|UI")
	FString LatestItemBonusText;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|UI")
	float ItemBonusTimerSec = 0.0f;

	// Dopamine Level / XP & Combo Reward State
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Progression")
	int32 PlayerLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Progression")
	int32 CurrentXP = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Progression")
	int32 XPToNextLevel = 500;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Progression")
	float XpGainPulse = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Progression")
	float ComboHitPulse = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Progression")
	float LevelUpBannerTimerSec = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Progression")
	FString LatestLevelUpText = TEXT("LEVEL UP! +25 NEON CORES!");

	// Abyss Fall & Cinematic Death Sequence
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|State")
	bool bIsDeathSequenceActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|State")
	float DeathSequenceTimer = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|State")
	bool bDeadPopupShown = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|State")
	float DeadPopupAlpha = 0.0f;

	// Score Pulse & Highscore Celebration
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|State")
	float ScorePulseAnim = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|State")
	bool bNewHighScoreAchieved = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|State")
	float HighScoreCelebrationTimer = 0.0f;

	// Music Pitch Smoothing (Chill start -> Subtle progressive increase)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Audio")
	float CurrentSmoothedMusicPitch = 0.80f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Audio")
	float TargetMusicPitch = 0.80f;

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Game")
	void TriggerAbyssFallDeath();

	void SpawnFloatingText(const FString& Txt, float X, float Z, const FLinearColor& Color, int32 Size = 14);

	// Hidden Admin / Godmode Testing Variables
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Admin")
	bool bAdminGodMode = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Admin")
	bool bAdminInfiniteJump = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Admin")
	bool bAdminInfinitePowerups = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Admin")
	float AdminGravityScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Admin")
	float AdminJumpForceScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Admin")
	float AdminMoveSpeedScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Admin")
	float AdminCameraSpeedScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Admin")
	float AdminPlatformWidthScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Data")
	TArray<FVTBiomeInfo> Biomes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Data")
	TArray<FVTGameThemeInfo> Themes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Data")
	TArray<FVTClimberInfo> Climbers;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Data")
	TArray<FVTTrailInfo> Trails;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Data")
	TArray<FVTAchievement> Achievements;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Data")
	UVTSaveGame* ActiveSaveGame = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Actors")
	AVTClimberCharacter* ClimberPawn = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Actors")
	TArray<AVTPlatformActor*> Platforms;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|Actors")
	ACameraActor* TowerCamera = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|UI")
	TArray<FVTFloatingTextEntry> FloatingTexts;

	// Constants matching 1:1 Reference Layout (10 visible floors in 9:16 frame)
	static constexpr float PlayfieldHalfWidthCm = 344.0f;
	static constexpr float FloorVerticalSpacingCm = 118.0f;
	static constexpr float FixedStepSeconds = 1.0f / 60.0f;

private:
	void InitializeStaticCatalogs();
	void InitializeAudioEngine();
	bool LoadWavClipFromDisk(const FString& Key, const FString& RelativeWavPath);
	void PumpProceduralAudioStream(float DeltaTime);
	void BuildTowerEnvironment3D();
	void StepFixedPhysics60Hz();
	void Update3DEnvironmentAndCamera(float DeltaTime);
	AVTPlatformActor* SpawnPlatformForFloor(int32 Floor);
	void HandleWallCollision(int32 SideDir, float MaxVx);
	void CashOutCombo();
	void SpawnIceShatter3D(AVTPlatformActor* Plat, const FLinearColor& BiomeColor);
	void SpawnShockwave3D(float X, float Z, float MaxRadiusCm, const FLinearColor& Color);
	void TriggerToast(const FString& Msg);

	float PhysicsTimeAccumulator = 0.0f;
	float SynthBeatTimer = 0.0f;
	float MenuShowroomAngle = 0.0f;

	UPROPERTY()
	AActor* EnvironmentHostActor = nullptr;

	UPROPERTY()
	TArray<UStaticMeshComponent*> LeftWallSegments;

	UPROPERTY()
	TArray<UStaticMeshComponent*> RightWallSegments;

	UPROPERTY()
	TArray<UStaticMeshComponent*> LeftWallPillars;

	UPROPERTY()
	TArray<UStaticMeshComponent*> RightWallPillars;

	UPROPERTY()
	TArray<UStaticMeshComponent*> BackWallPanels;

	UPROPERTY()
	TArray<UStaticMeshComponent*> EqualizerBarMeshes;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> EqualizerBarMIDs;

	UPROPERTY()
	TArray<UStaticMeshComponent*> BgMegastructurePillars;

	UPROPERTY()
	UMaterialInstanceDynamic* LeftWallMID = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* RightWallMID = nullptr;

	UPROPERTY()
	UPointLightComponent* TowerMainLight = nullptr;

	// Theme Specific 3D World Scenery (Outrun Sun, Jellyfish, Hanging Vines, Rotating Gears, Candy Canes, etc.)
	UPROPERTY()
	TArray<UStaticMeshComponent*> ThemeSceneryMeshes;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> ThemeSceneryMIDs;

	UPROPERTY()
	TArray<UPointLightComponent*> ThemeSceneryLights;

	TArray<FVector> ThemeSceneryBasePos;
	TArray<FRotator> ThemeSceneryBaseRot;

	void BuildThemeScenery(const FVTGameThemeInfo& ThemeInfo);
	void UpdateThemeScenery(float DeltaTime, float TimeSec);

	UPROPERTY()
	TArray<FVTIceShardSim> IceShardPool;

	int32 NextIceShardIndex = 0;

	UPROPERTY()
	TArray<FVTShockwaveSim> ShockwavePool;

	int32 NextShockwaveIndex = 0;

	// Real-time Multi-Voice Audio Mixer & Procedural Stream
	UPROPERTY()
	USoundWaveProcedural* ProceduralAudioStream = nullptr;

	UPROPERTY()
	UAudioComponent* ProceduralAudioComp = nullptr;

	TMap<FString, FVTPcmClip> LoadedAudioClips;
	TArray<FVTActiveSfxVoice> ActiveSfxVoices;
	double MusicReadFramePos = 0.0;
};

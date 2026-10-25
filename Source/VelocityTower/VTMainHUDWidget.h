#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VelocityTowerTypes.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "VTMainHUDWidget.generated.h"

class AVTTowerGameMode;
class AVTPlayerController;
class UTexture2D;
class SBorder;
class STextBlock;
class SBox;
class SVerticalBox;

UENUM(BlueprintType)
enum class EVTModalOverlay : uint8
{
	None,
	ModeSelect,
	CharacterSelect,
	SettingsAndControls,
	ControlPreview,
	HudEditor,
	NeonShop,
	RewardsHub,
	FortuneWheel,
	LoginStreak,
	DailyQuests,
	TrophiesShowcase,
	FriendsAndRivals,
	AccountCloudSync,
	RatePrompt,
	AdPlayer,
	AdminGodMode
};

UCLASS()
class VELOCITYTOWER_API UVTMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Admin")
	void ToggleAdminGodModeModal();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|UI")
	void OpenCharacterSelectModal();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|UI")
	void OpenLeaderboardModal(bool bFriendsTab = false);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	AVTTowerGameMode* GetTowerGM() const;
	AVTPlayerController* GetTowerPC() const;

	// Nano Banana 2 UI Texture Loader
	void LoadNanoBananaTextures();
	UTexture2D* LoadTextureFromDiskOrAsset(const FString& AssetPath, const FString& RelativeDiskPath);

	// Dynamic Modal Content Builders
	void OpenModal(EVTModalOverlay ModalType);
	void CloseModal();
	void RebuildModalContent();
	void RebuildGameOverLeaderboard();
	void PopulateLeaderboardRows(TSharedPtr<SVerticalBox> TargetBox, bool bFriendsOnly);
	void StartSimulatedAd(TFunction<void()> OnCompleteCallback);

	EVTModalOverlay ActiveModal = EVTModalOverlay::None;
	EVTControlMode PreviewedControlMode = EVTControlMode::VirtualJoystick;
	int32 ActiveShopTab = 2; // Default to 20-Character Roster Tab! (0=Gratis Cores, 1=Deals, 2=20 Climbers, 3=Trails, 4=VIP)
	bool bLeaderboardFriendsOnly = false;
	int32 LastBuiltLeaderboardScore = -1;
	bool bLastBuiltFriendsMode = false;
	FString LastWheelResultText = TEXT("BEREIT FUER MEGA-LOOT!");
	float AdCountdownRemaining = 0.0f;
	TFunction<void()> PendingAdRewardCallback;
	int32 SecretAdminTapCount = 0;

	// Nano Banana 2 Textures & Slate Brushes
	UPROPERTY()
	UTexture2D* TexMainBanner = nullptr;

	UPROPERTY()
	UTexture2D* TexModeCard = nullptr;

	UPROPERTY()
	UTexture2D* TexCharSpotlight = nullptr;

	UPROPERTY()
	UTexture2D* TexFeverFrame = nullptr;

	UPROPERTY()
	UTexture2D* TexDeadBanner = nullptr;

	TSharedPtr<FSlateDynamicImageBrush> BrushMainBanner;
	TSharedPtr<FSlateDynamicImageBrush> BrushModeCard;
	TSharedPtr<FSlateDynamicImageBrush> BrushCharSpotlight;
	TSharedPtr<FSlateDynamicImageBrush> BrushFeverFrame;
	TSharedPtr<FSlateDynamicImageBrush> BrushDeadBanner;

	TSharedPtr<SBorder> DeadBannerOverlayBorder;
	TSharedPtr<SBorder> HighScoreBannerOverlayBorder;
	TSharedPtr<SBorder> TopBestScoreBorder;
	TSharedPtr<STextBlock> TxtTopBestScoreVal;

	// Interactive HUD Overlay Drag-and-Drop State
	bool bHudDragEditMode = false;
	int32 DraggingClusterIndex = 0; // 0=None, 1=Left Joystick/D-Pad, 2=Right Jump Button
	FVector2D DragLastMousePos = FVector2D::ZeroVector;
	bool bVirtualJoystickMouseDrag = false;
	FVector2D VirtualJoystickMouseOrigin = FVector2D::ZeroVector;

	// Boot Sequence Overlay Pointers (Splash -> Teaser Intro -> Account Login Gate)
	TSharedPtr<SBorder> EngineSplashOverlayBorder;
	TSharedPtr<SBox> SplashProgressFillBox;
	TSharedPtr<STextBlock> TxtSplashStatus;
	TSharedPtr<STextBlock> TxtSplashPercent;

	TSharedPtr<SBorder> TeaserIntroOverlayBorder;
	TSharedPtr<STextBlock> TxtTeaserHeadline;
	TSharedPtr<STextBlock> TxtTeaserSubline;
	TSharedPtr<STextBlock> TxtTeaserPhaseBadge;

	TSharedPtr<SBorder> AccountGateOverlayBorder;

	// Top In-Game Telemetry Bar Container
	TSharedPtr<SBorder> InGameTopHudBorder;

	// Live Slate Pointers for 60fps zero-allocation telemetry updates
	TSharedPtr<STextBlock> TxtBiomeLabel;
	TSharedPtr<STextBlock> TxtScoreVal;
	TSharedPtr<STextBlock> TxtFloorVal;
	TSharedPtr<STextBlock> TxtBpmVal;
	TSharedPtr<STextBlock> TxtCoresVal;
	TSharedPtr<SBorder> AdminBadgeBorder;

	// Left Top Bar: Dual-Telemetry Combo-Stick & Multiplier Escalation Bar
	TSharedPtr<SBorder> TopComboBarBorder;
	TSharedPtr<SBox> TopComboFillBox;
	TSharedPtr<SBorder> TopComboFillBorder;
	TSharedPtr<STextBlock> TxtTopComboBadge;
	TSharedPtr<STextBlock> TxtTopComboSub;

	// Right Top Bar: Dopamine Level & XP Progression Bar
	TSharedPtr<SBorder> TopXpBarBorder;
	TSharedPtr<SBox> TopXpFillBox;
	TSharedPtr<SBorder> TopXpFillBorder;
	TSharedPtr<STextBlock> TxtTopLevelBadge;
	TSharedPtr<STextBlock> TxtTopXpNumbers;
	TSharedPtr<SBorder> LevelUpBannerBorder;
	TSharedPtr<STextBlock> TxtLevelUpBanner;

	TSharedPtr<SBorder> ToastBannerBorder;
	TSharedPtr<STextBlock> TxtToastMessage;
	TSharedPtr<SBorder> HurryBannerBorder;
	TSharedPtr<SBorder> PowerUpBannerBorder;
	TSharedPtr<STextBlock> TxtPowerUpBanner;
	TSharedPtr<SBorder> AnnouncerBannerBorder;
	TSharedPtr<STextBlock> TxtAnnouncerCallout;

	// Dopamine Combo & Fever Meter Pointers
	TSharedPtr<SBorder> ComboHudBorder;
	TSharedPtr<STextBlock> TxtComboMult;
	TSharedPtr<STextBlock> TxtComboTierLabel;
	TSharedPtr<SBox> ComboFuseFillBox;
	TSharedPtr<SBorder> ComboFuseFillBorder;

	TSharedPtr<SBorder> BlitzTimerBorder;
	TSharedPtr<STextBlock> TxtBlitzTimer;

	// Subtle Draggable Touch HUD Container, Virtual Joystick & Classic D-Pad
	TSharedPtr<SBorder> TouchHudContainer;
	TSharedPtr<SBorder> HudDragBannerBorder;
	TSharedPtr<SBorder> LeftTouchClusterBorder;
	TSharedPtr<SBorder> RightTouchClusterBorder;
	TSharedPtr<SBox> LeftClusterSizeBox;
	TSharedPtr<SBox> RightClusterSizeBox;
	TSharedPtr<SBorder> VirtualJoystickModeBox;
	TSharedPtr<SBorder> ClassicDpadModeBox;
	TSharedPtr<SBorder> JoystickKnobBorder;

	// Clean Guided Main Menu Overlay Pointers
	TSharedPtr<SBorder> MainMenuOverlayBorder;
	TSharedPtr<STextBlock> TxtHomeNickname;
	TSharedPtr<STextBlock> TxtHomeAccountBadge;
	TSharedPtr<STextBlock> TxtHomeCores;
	TSharedPtr<STextBlock> TxtHomeRosterProgress;
	TSharedPtr<STextBlock> TxtHomeLevelTitle;
	TSharedPtr<STextBlock> TxtHomeXpProgress;
	TSharedPtr<SBox> HomeXpFillBox;
	TSharedPtr<STextBlock> TxtHomeBestRecord;
	TSharedPtr<STextBlock> TxtHomeEquippedSummary;
	TSharedPtr<STextBlock> TxtHomePlayBtnSub;

	// Game Over Overlay Pointers + Post-Game Leaderboard & Friends Comparison
	TSharedPtr<SBorder> GameOverOverlayBorder;
	TSharedPtr<STextBlock> TxtOverBadge;
	TSharedPtr<STextBlock> TxtOverSubtext;
	TSharedPtr<STextBlock> TxtOverScore;
	TSharedPtr<STextBlock> TxtOverFloor;
	TSharedPtr<STextBlock> TxtOverCombo;
	TSharedPtr<STextBlock> TxtOverBpm;
	TSharedPtr<STextBlock> TxtOverPlacementSummary;
	TSharedPtr<SBorder> TabOverGlobalBorder;
	TSharedPtr<SBorder> TabOverFriendsBorder;
	TSharedPtr<SVerticalBox> GameOverLeaderboardListBox;
	TSharedPtr<SBorder> ReviveButtonContainer;
	TSharedPtr<STextBlock> TxtReviveCountdown;

	// Universal Modal Backdrop & Dynamic Host Box
	TSharedPtr<SBorder> ModalBackdropBorder;
	TSharedPtr<SBorder> ModalOuterFrameBorder;
	TSharedPtr<SBorder> ModalInnerCardBorder;
	TSharedPtr<SBox> ModalMaxHeightBox;
	TSharedPtr<SVerticalBox> ModalBodyBox;
};

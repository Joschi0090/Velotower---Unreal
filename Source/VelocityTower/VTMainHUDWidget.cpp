#include "VTMainHUDWidget.h"
#include "VTTowerGameMode.h"
#include "VTPlayerController.h"
#include "VTClimberCharacter.h"
#include "VTSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"
#include "UnrealClient.h"
#include "Kismet/KismetSystemLibrary.h"

namespace VTTheme
{
	static const FLinearColor Cyan(0.0f, 0.961f, 0.831f, 1.0f);
	static const FLinearColor Pink(0.969f, 0.145f, 0.522f, 1.0f);
	static const FLinearColor Gold(1.0f, 0.785f, 0.080f, 1.0f);
	static const FLinearColor Purple(0.680f, 0.320f, 1.0f, 1.0f);
	static const FLinearColor Emerald(0.0f, 0.920f, 0.520f, 1.0f);
	static const FLinearColor SkyBlue(0.220f, 0.741f, 0.973f, 1.0f);
	static const FLinearColor DarkBg(0.012f, 0.022f, 0.055f, 0.95f);
	static const FLinearColor CardBg(0.035f, 0.062f, 0.135f, 0.94f);
	static const FLinearColor PillBg(0.024f, 0.045f, 0.098f, 0.94f);
	static const FLinearColor MutedText(0.620f, 0.690f, 0.790f, 1.0f);

	static FSlateFontInfo Font(int32 Size, bool bBold = true)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	static TSharedRef<SWidget> MakeButton(
		const FString& Label,
		const FLinearColor& BgColor,
		const FLinearColor& TextColor,
		int32 FontSize,
		FMargin Padding,
		FOnClicked OnClicked)
	{
		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
		const bool bBrightFill = (BgColor.GetLuminance() > 0.32f);
		const FLinearColor RimColor = bBrightFill ? BgColor : FLinearColor(TextColor.R, TextColor.G, TextColor.B, 0.88f);
		const FLinearColor InnerFill = bBrightFill
			? FLinearColor(BgColor.R * 0.24f, BgColor.G * 0.28f, BgColor.B * 0.32f, 0.96f)
			: FLinearColor(BgColor.R, BgColor.G, BgColor.B, 0.95f);
		const FLinearColor FinalTextCol = bBrightFill ? FLinearColor::White : ((TextColor.GetLuminance() < 0.25f) ? FLinearColor::White : TextColor);

		return SNew(SButton)
			.IsFocusable(false)
			.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
			.ContentPadding(FMargin(0.0f))
			.OnClicked(OnClicked)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(RimColor)
				.Padding(FMargin(bBrightFill ? 2.0f : 1.5f))
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(InnerFill)
					.Padding(Padding)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Label))
						.Font(Font(FontSize, true))
						.ColorAndOpacity(FinalTextCol)
						.Justification(ETextJustify::Center)
					]
				]
			];
	}

	static TSharedRef<SWidget> MakeDockTabButton(
		const FString& TagText,
		const FString& MainTitle,
		const FString& SubText,
		const FLinearColor& AccentColor,
		FOnClicked OnClicked)
	{
		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
		return SNew(SButton)
			.IsFocusable(false)
			.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
			.ContentPadding(FMargin(0.0f))
			.OnClicked(OnClicked)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(AccentColor.R, AccentColor.G, AccentColor.B, 0.85f))
				.Padding(FMargin(1.5f))
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(FLinearColor(0.025f, 0.048f, 0.110f, 0.96f))
					.Padding(FMargin(8.0f, 6.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TagText))
							.Font(Font(7, true))
							.ColorAndOpacity(AccentColor)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(MainTitle))
							.Font(Font(10, true))
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(SubText))
							.Font(Font(7, false))
							.ColorAndOpacity(MutedText)
						]
					]
				]
			];
	}

	static FString GetControlModeShortName(EVTControlMode Mode)
	{
		switch (Mode)
		{
		case EVTControlMode::VirtualJoystick: return TEXT("1. VIRTUELLER JOYSTICK");
		case EVTControlMode::ClassicButtons: return TEXT("2. KLASSISCHES D-PAD");
		case EVTControlMode::OneThumbSlide: return TEXT("3. 1-DAUMEN SLIDE");
		case EVTControlMode::FullscreenSplit: return TEXT("4. FULLSCREEN SPLIT");
		case EVTControlMode::SwipeGestures: return TEXT("5. SWIPE & FLICK GESTEN");
		case EVTControlMode::AutoBounce: return TEXT("6. AUTO-BOUNCE ZEN");
		default: return TEXT("VIRTUELLER JOYSTICK");
		}
	}

	static FString GetGameModeShortName(EVTGameModeType Mode)
	{
		switch (Mode)
		{
		case EVTGameModeType::Arcade: return TEXT("ARCADE CLASSIC");
		case EVTGameModeType::Hyper: return TEXT("HYPER 1.25x TURBO");
		case EVTGameModeType::Blitz60s: return TEXT("BLITZ 60s ATTACK");
		case EVTGameModeType::Zen: return TEXT("ZEN ENDLESS");
		case EVTGameModeType::Overdrive2x: return TEXT("OVERDRIVE 2x FEVER");
		case EVTGameModeType::Rival1v1: return TEXT("1v1 RIVAL DUELL");
		default: return TEXT("ARCADE CLASSIC");
		}
	}
}

AVTTowerGameMode* UVTMainHUDWidget::GetTowerGM() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<AVTTowerGameMode>() : nullptr;
}

AVTPlayerController* UVTMainHUDWidget::GetTowerPC() const
{
	return Cast<AVTPlayerController>(GetOwningPlayer());
}

UTexture2D* UVTMainHUDWidget::LoadTextureFromDiskOrAsset(const FString& AssetPath, const FString& RelativeDiskPath)
{
	if (UTexture2D* ImportedTex = LoadObject<UTexture2D>(nullptr, *AssetPath))
	{
		return ImportedTex;
	}

	TArray<FString> Candidates;
	Candidates.Add(FPaths::Combine(FPaths::ProjectDir(), RelativeDiskPath));
	Candidates.Add(FPaths::Combine(FPaths::LaunchDir(), RelativeDiskPath));
	Candidates.Add(FPaths::Combine(FPaths::LaunchDir(), TEXT("VelocityTower"), RelativeDiskPath));
	Candidates.Add(FPaths::Combine(FPaths::ProjectDir(), TEXT("VelocityTower"), RelativeDiskPath));
	Candidates.Add(FPaths::Combine(FPaths::ProjectDir(), TEXT("RawAssets"), RelativeDiskPath));
	Candidates.Add(FPaths::Combine(FPaths::ProjectContentDir(), TEXT(".."), RelativeDiskPath));
	Candidates.Add(FPaths::Combine(FPaths::RootDir(), RelativeDiskPath));
	Candidates.Add(FPaths::Combine(TEXT("E:/Projekte/Velocitytower"), RelativeDiskPath));

	TArray<uint8> FileData;
	for (const FString& Candidate : Candidates)
	{
		if (FFileHelper::LoadFileToArray(FileData, *Candidate) && FileData.Num() > 0)
		{
			break;
		}
	}

	if (FileData.Num() == 0)
	{
		return nullptr;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	EImageFormat DetectedFormat = ImageWrapperModule.DetectImageFormat(FileData.GetData(), FileData.Num());
	if (DetectedFormat == EImageFormat::Invalid)
	{
		DetectedFormat = EImageFormat::JPEG;
	}

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(DetectedFormat);
	if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num()))
	{
		return nullptr;
	}

	TArray<uint8> UncompressedBGRA;
	if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
	{
		return nullptr;
	}

	const int32 Width = ImageWrapper->GetWidth();
	const int32 Height = ImageWrapper->GetHeight();
	if (Width <= 0 || Height <= 0) return nullptr;

	UTexture2D* NewTex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!NewTex || !NewTex->GetPlatformData() || NewTex->GetPlatformData()->Mips.Num() == 0)
	{
		return nullptr;
	}

	void* TextureData = NewTex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
	NewTex->GetPlatformData()->Mips[0].BulkData.Unlock();
	NewTex->UpdateResource();
	return NewTex;
}

void UVTMainHUDWidget::LoadNanoBananaTextures()
{
	if (!TexMainBanner)
	{
		TexMainBanner = LoadTextureFromDiskOrAsset(
			TEXT("/Game/VelocityTower/UI/UI_MainBanner_Apex.UI_MainBanner_Apex"),
			TEXT("RawAssets/UI/UI_MainBanner_Apex.jpg"));
		if (TexMainBanner)
		{
			BrushMainBanner = MakeShareable(new FSlateDynamicImageBrush(TexMainBanner, FVector2D(640.0f, 180.0f), FName("VTMainBannerBrush")));
		}
	}
	if (!TexModeCard)
	{
		TexModeCard = LoadTextureFromDiskOrAsset(
			TEXT("/Game/VelocityTower/UI/UI_ModeCards_Atlas.UI_ModeCards_Atlas"),
			TEXT("RawAssets/UI/UI_ModeCards_Atlas.jpg"));
		if (TexModeCard)
		{
			BrushModeCard = MakeShareable(new FSlateDynamicImageBrush(TexModeCard, FVector2D(640.0f, 140.0f), FName("VTModeCardBrush")));
		}
	}
	if (!TexCharSpotlight)
	{
		TexCharSpotlight = LoadTextureFromDiskOrAsset(
			TEXT("/Game/VelocityTower/UI/UI_CharSelect_Spotlight.UI_CharSelect_Spotlight"),
			TEXT("RawAssets/UI/UI_CharSelect_Spotlight.jpg"));
		if (TexCharSpotlight)
		{
			BrushCharSpotlight = MakeShareable(new FSlateDynamicImageBrush(TexCharSpotlight, FVector2D(640.0f, 220.0f), FName("VTCharSpotlightBrush")));
		}
	}
	if (!TexFeverFrame)
	{
		TexFeverFrame = LoadTextureFromDiskOrAsset(
			TEXT("/Game/VelocityTower/UI/UI_Dopamine_FeverFrame.UI_Dopamine_FeverFrame"),
			TEXT("RawAssets/UI/UI_Dopamine_FeverFrame.jpg"));
		if (TexFeverFrame)
		{
			BrushFeverFrame = MakeShareable(new FSlateDynamicImageBrush(TexFeverFrame, FVector2D(180.0f, 28.0f), FName("VTFeverFrameBrush")));
		}
	}
	if (!TexDeadBanner)
	{
		TexDeadBanner = LoadTextureFromDiskOrAsset(
			TEXT("/Game/VelocityTower/UI/UI_Dead_Banner.UI_Dead_Banner"),
			TEXT("RawAssets/UI/UI_Dead_Banner.jpg"));
		if (TexDeadBanner)
		{
			BrushDeadBanner = MakeShareable(new FSlateDynamicImageBrush(TexDeadBanner, FVector2D(580.0f, 320.0f), FName("VTDeadBannerBrush")));
		}
	}

	// Load all 10 1:1 Theme Frame Textures from RawAssets/Themes/
	auto LoadThemeTex = [this](EVTGameThemeId ThemeId, const FString& FileName, const FName& BrushName)
	{
		if (ThemeFrameBrushes.Contains(ThemeId)) return;
		UTexture2D* Tex = LoadTextureFromDiskOrAsset(
			FString::Printf(TEXT("/Game/VelocityTower/Themes/%s.%s"), *FPaths::GetBaseFilename(FileName), *FPaths::GetBaseFilename(FileName)),
			FString::Printf(TEXT("RawAssets/Themes/%s"), *FileName));
		if (Tex)
		{
			ThemeFrameTextures.Add(ThemeId, Tex);
			ThemeFrameBrushes.Add(ThemeId, MakeShareable(new FSlateDynamicImageBrush(Tex, FVector2D(float(Tex->GetSizeX()), float(Tex->GetSizeY())), BrushName)));
		}
	};

	LoadThemeTex(EVTGameThemeId::PapercraftToybox, TEXT("Theme_Papercraft.jpg"), FName("VTTheme_Papercraft"));
	LoadThemeTex(EVTGameThemeId::AncientRuins, TEXT("Theme_AncientRuins.jpg"), FName("VTTheme_AncientRuins"));
	LoadThemeTex(EVTGameThemeId::DeepSea, TEXT("Theme_DeepSea.jpg"), FName("VTTheme_DeepSea"));
	LoadThemeTex(EVTGameThemeId::RetroSynthwave, TEXT("Theme_RetroSynthwave_Alt.jpg"), FName("VTTheme_RetroSynthwave"));
	LoadThemeTex(EVTGameThemeId::CandyLand, TEXT("Theme_CandyLand.jpg"), FName("VTTheme_CandyLand"));
	LoadThemeTex(EVTGameThemeId::IndustrialForge, TEXT("Theme_IndustrialForge.jpg"), FName("VTTheme_IndustrialForge"));
	LoadThemeTex(EVTGameThemeId::DeepSpaceOrbit, TEXT("Theme_DeepSpaceOrbit.jpg"), FName("VTTheme_DeepSpaceOrbit"));
	LoadThemeTex(EVTGameThemeId::PixelDungeon, TEXT("Theme_PixelDungeon.jpg"), FName("VTTheme_PixelDungeon"));
	LoadThemeTex(EVTGameThemeId::WhimsicalToybox, TEXT("Theme_WhimsicalToybox.jpg"), FName("VTTheme_WhimsicalToybox"));
	LoadThemeTex(EVTGameThemeId::CyberpunkDefault, TEXT("Theme_CyberpunkMetropolis.jpg"), FName("VTTheme_CyberpunkMetropolis"));
	LoadThemeTex(EVTGameThemeId::BioLumenArk, TEXT("Theme_BioLumenArk.jpg"), FName("VTTheme_BioLumenArk"));
}

void UVTMainHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	LoadNanoBananaTextures();
}

void UVTMainHUDWidget::ToggleAdminGodModeModal()
{
	if (AVTTowerGameMode* GM = GetTowerGM())
	{
		GM->PlayTowerSFX(TEXT("ComboUp"), 1.35f, 1.0f);
	}
	if (ActiveModal == EVTModalOverlay::AdminGodMode)
	{
		CloseModal();
	}
	else
	{
		OpenModal(EVTModalOverlay::AdminGodMode);
	}
}

void UVTMainHUDWidget::OpenCharacterSelectModal()
{
	OpenModal(EVTModalOverlay::CharacterSelect);
}

void UVTMainHUDWidget::OpenThemeSelectModal()
{
	OpenModal(EVTModalOverlay::ThemeSelect);
}

void UVTMainHUDWidget::OpenLeaderboardModal(bool bFriendsTab)
{
	bLeaderboardFriendsOnly = bFriendsTab;
	OpenModal(EVTModalOverlay::FriendsAndRivals);
}

#ifdef _MSC_VER
#pragma warning(disable: 4883)
#endif
TSharedRef<SWidget> UVTMainHUDWidget::RebuildWidget()
{
	LoadNanoBananaTextures();
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
	const FSlateBrush* MainBannerImg = BrushMainBanner.IsValid() ? BrushMainBanner.Get() : WhiteBrush;
	const FSlateBrush* FeverFrameImg = BrushFeverFrame.IsValid() ? BrushFeverFrame.Get() : WhiteBrush;

	return SNew(SOverlay)

	// =========================================================================
	// LAYER 1: DECLUTTERED TOP IN-GAME HUD + DOPAMINE XP/LEVEL BAR + COMBO FEVER
	// =========================================================================
	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SNew(SVerticalBox)

		// 1. TOP IN-GAME HUD TELEMETRY CONTAINER (1A + 1B)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(InGameTopHudBorder, SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.Padding(0.0f)
			[
				SNew(SVerticalBox)

				// 1A. Modern Top Telemetry Bar with Mobile Notch Safe-Zone
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(FLinearColor(0.0f, 0.961f, 0.831f, 0.65f))
					.Padding(FMargin(0.0f, 0.0f, 0.0f, 1.5f))
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(0.012f, 0.024f, 0.058f, 0.96f))
						.Padding(FMargin(10.0f, 32.0f, 10.0f, 6.0f))
						[
							SNew(SHorizontalBox)

							// Left: Brand Icon + Live Score & Biome Label
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(0.0f, 0.0f, 6.0f, 0.0f)
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(VTTheme::Cyan)
									.Padding(FMargin(6.0f, 4.0f))
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("VT")))
										.Font(VTTheme::Font(10, true))
										.ColorAndOpacity(FLinearColor(0.01f, 0.03f, 0.08f, 1.0f))
									]
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight()
									[
										SAssignNew(TxtBiomeLabel, STextBlock)
										.Text(FText::FromString(TEXT("SCORE")))
										.Font(VTTheme::Font(7, true))
										.ColorAndOpacity(VTTheme::Cyan)
									]
									+ SVerticalBox::Slot().AutoHeight()
									[
										SAssignNew(TxtScoreVal, STextBlock)
										.Text(FText::FromString(TEXT("0")))
										.Font(VTTheme::Font(14, true))
										.ColorAndOpacity(FLinearColor::White)
									]
								]
							]

							+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]

							// Center: Highscore & Current Floor
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(4.0f, 0.0f)
							[
								SNew(SHorizontalBox)
								// BEST / REKORD PILL
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(0.0f, 0.0f, 8.0f, 0.0f)
								[
									SAssignNew(TopBestScoreBorder, SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(FLinearColor(1.0f, 0.745f, 0.043f, 0.85f))
									.Padding(FMargin(1.0f))
									[
										SNew(SBorder)
										.BorderImage(WhiteBrush)
										.BorderBackgroundColor(FLinearColor(0.04f, 0.02f, 0.01f, 0.96f))
										.Padding(FMargin(6.0f, 2.5f))
										[
											SNew(SVerticalBox)
											+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
											[
												SNew(STextBlock).Text(FText::FromString(TEXT("BEST"))).Font(VTTheme::Font(6, true)).ColorAndOpacity(VTTheme::Gold)
											]
											+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
											[
												SAssignNew(TxtTopBestScoreVal, STextBlock).Text(FText::FromString(TEXT("0"))).Font(VTTheme::Font(10, true)).ColorAndOpacity(VTTheme::Gold)
											]
										]
									]
								]
								// FLOOR PILL
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(FLinearColor(0.0f, 0.961f, 0.831f, 0.65f))
									.Padding(FMargin(1.5f))
									[
										SNew(SBorder)
										.BorderImage(WhiteBrush)
										.BorderBackgroundColor(VTTheme::PillBg)
										.Padding(FMargin(9.0f, 3.0f))
										[
											SNew(SVerticalBox)
											+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
											[
												SNew(STextBlock).Text(FText::FromString(TEXT("TURM-ETAGE"))).Font(VTTheme::Font(7, true)).ColorAndOpacity(VTTheme::MutedText)
											]
											+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
											[
												SAssignNew(TxtFloorVal, STextBlock).Text(FText::FromString(TEXT("0"))).Font(VTTheme::Font(14, true)).ColorAndOpacity(VTTheme::Cyan)
											]
										]
									]
								]
							]

							+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]

							// Retain TxtBpmVal & AdminBadgeBorder so pointers remain valid
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SAssignNew(AdminBadgeBorder, SBorder)
								.Visibility(EVisibility::Collapsed)
								[
									SAssignNew(TxtBpmVal, STextBlock).Text(FText::FromString(TEXT("120")))
								]
							]

							// Right: Neon Cores + Settings Gear + Pause/Menu
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SHorizontalBox)
								// CORES
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(0.0f, 0.0f, 6.0f, 0.0f)
								[
									SNew(SButton)
									.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
									.ContentPadding(FMargin(0.0f))
									.OnClicked_Lambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click")); ActiveShopTab = 2; OpenModal(EVTModalOverlay::NeonShop); return FReply::Handled(); })
									[
										SNew(SBorder)
										.BorderImage(WhiteBrush)
										.BorderBackgroundColor(VTTheme::Gold)
										.Padding(FMargin(1.0f))
										[
											SNew(SBorder)
											.BorderImage(WhiteBrush)
											.BorderBackgroundColor(VTTheme::PillBg)
											.Padding(FMargin(6.0f, 2.5f))
											[
												SNew(SHorizontalBox)
												+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 2.0f, 0.0f)
												[
													SNew(STextBlock).Text(FText::FromString(TEXT("◆"))).Font(VTTheme::Font(7, true)).ColorAndOpacity(VTTheme::Gold)
												]
												+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
												[
													SAssignNew(TxtCoresVal, STextBlock).Text(FText::FromString(TEXT("0"))).Font(VTTheme::Font(10, true)).ColorAndOpacity(VTTheme::Gold)
												]
											]
										]
									]
								]
								// SETTINGS GEAR BUTTON
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(0.0f, 0.0f, 4.0f, 0.0f)
								[
									VTTheme::MakeButton(TEXT("⚙"), VTTheme::CardBg, VTTheme::Cyan, 12, FMargin(7.0f, 4.0f),
										FOnClicked::CreateLambda([this]()
										{
											if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click"));
											OpenModal(EVTModalOverlay::SettingsAndControls);
											return FReply::Handled();
										}))
								]
								// QUICK MENU / PAUSE BUTTON
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									VTTheme::MakeButton(TEXT("⏸"), VTTheme::CardBg, VTTheme::Pink, 11, FMargin(7.0f, 4.0f),
										FOnClicked::CreateLambda([this]()
										{
											if (AVTTowerGameMode* GM = GetTowerGM())
											{
												GM->PlayTowerSFX(TEXT("Click"));
												CloseModal();
												GM->ReturnToMainMenu();
											}
											return FReply::Handled();
										}))
								]
							]
						]
					]
				]

		// 1B. DUAL TELEMETRY BARS: COMBO-STICK (LEFT) & LEVEL PROGRESSION (RIGHT)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(TopComboBarBorder, SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(FLinearColor(0.010f, 0.020f, 0.048f, 0.94f))
			.Padding(FMargin(12.0f, 5.0f))
			[
				SNew(SHorizontalBox)

				// LEFT BAR: COMBO-STICK // CONSECUTIVE COMBO STREAK & MULTIPLIER
				+ SHorizontalBox::Slot()
				.FillWidth(0.50f)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 2.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SAssignNew(TxtTopComboBadge, STextBlock)
							.Text(FText::FromString(TEXT("COMBO STICK // 0x BEREIT")))
							.Font(VTTheme::Font(7, true))
							.ColorAndOpacity(VTTheme::Cyan)
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SAssignNew(TxtTopComboSub, STextBlock)
							.Text(FText::FromString(TEXT("SPRINGE WEITER!")))
							.Font(VTTheme::Font(6, true))
							.ColorAndOpacity(VTTheme::MutedText)
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(0.0f, 0.961f, 0.831f, 0.45f))
						.Padding(FMargin(1.0f))
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.02f, 0.04f, 0.09f, 0.95f))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SAssignNew(TopComboFillBox, SBox)
									.WidthOverride(12.0f)
									.HeightOverride(7.0f)
									[
										SAssignNew(TopComboFillBorder, SBorder)
										.BorderImage(WhiteBrush)
										.BorderBackgroundColor(VTTheme::Cyan)
									]
								]
							]
						]
					]
				]

				// RIGHT BAR: LEVEL // XP PROGRESSION TO NEXT LEVEL
				+ SHorizontalBox::Slot()
				.FillWidth(0.50f)
				.VAlign(VAlign_Center)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 2.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SAssignNew(TxtTopLevelBadge, STextBlock)
							.Text(FText::FromString(TEXT("LVL 1 // ROOKIE CLIMBER")))
							.Font(VTTheme::Font(7, true))
							.ColorAndOpacity(VTTheme::Gold)
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SAssignNew(TxtTopXpNumbers, STextBlock)
							.Text(FText::FromString(TEXT("0 / 400 XP")))
							.Font(VTTheme::Font(6, true))
							.ColorAndOpacity(VTTheme::Cyan)
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(1.0f, 0.785f, 0.08f, 0.45f))
						.Padding(FMargin(1.0f))
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.02f, 0.04f, 0.09f, 0.95f))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SAssignNew(TopXpFillBox, SBox)
									.WidthOverride(24.0f)
									.HeightOverride(7.0f)
									[
										SAssignNew(TopXpFillBorder, SBorder)
										.BorderImage(WhiteBrush)
										.BorderBackgroundColor(VTTheme::Gold)
									]
								]
							]
						]
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SAssignNew(BlitzTimerBorder, SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(VTTheme::Pink)
					.Padding(FMargin(8.0f, 4.0f))
					.Visibility(EVisibility::Collapsed)
					[
						SAssignNew(TxtBlitzTimer, STextBlock)
						.Text(FText::FromString(TEXT("60.0s")))
						.Font(VTTheme::Font(10, true))
						.ColorAndOpacity(FLinearColor::White)
					]
				]
			]
		]
			]
		]

		// 1C-ALTITUDE: DEDICATED PROMINENT LIVE TOWER HEIGHT OVERLAY (Always prominent during climb)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(0.0f, 6.0f, 0.0f, 2.0f)
		[
			SAssignNew(LiveAltitudeHudBorder, SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(FLinearColor(0.0f, 0.961f, 0.831f, 0.85f))
			.Padding(FMargin(1.5f))
			.Visibility(EVisibility::Collapsed)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.012f, 0.024f, 0.055f, 0.94f))
				.Padding(FMargin(18.0f, 5.0f))
				[
					SNew(SHorizontalBox)
					// Neon Elevation Arrow Icon
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("▲")))
						.Font(VTTheme::Font(15, true))
						.ColorAndOpacity(VTTheme::Cyan)
					]
					// Large Bold Floor Indicator
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SAssignNew(TxtLiveFloorNumber, STextBlock)
						.Text(FText::FromString(TEXT("ETAGE 0")))
						.Font(VTTheme::Font(18, true))
						.ColorAndOpacity(FLinearColor::White)
					]
					// Stylish Cyber Divider
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(10.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("//")))
						.Font(VTTheme::Font(13, true))
						.ColorAndOpacity(FLinearColor(0.25f, 0.45f, 0.70f))
					]
					// Large Bold Altitude In Meters
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SAssignNew(TxtLiveAltitudeMeters, STextBlock)
						.Text(FText::FromString(TEXT("0 METER")))
						.Font(VTTheme::Font(14, true))
						.ColorAndOpacity(VTTheme::Cyan)
					]
				]
			]
		]

		// 1C-Drag: Interactive HUD Drag-Move Mode Toolbar Banner (Only shown when editing HUD overlay!)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.0f, 4.0f)
		[
			SAssignNew(HudDragBannerBorder, SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(VTTheme::Gold)
			.Padding(FMargin(2.0f))
			.Visibility(EVisibility::Collapsed)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.012f, 0.03f, 0.08f, 0.97f))
				.Padding(FMargin(12.0f, 10.0f))
				[
					SNew(SVerticalBox)

					// Title
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 3.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("TOUCH-STEUERUNG KONFIGURIEREN")))
						.Font(VTTheme::Font(12, true))
						.ColorAndOpacity(VTTheme::Gold)
					]

					// Prominent Active Button Indicator
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SAssignNew(TxtActiveButtonTitle, STextBlock)
						.Text(FText::FromString(TEXT("AKTIV: 1. [ ◀ LINKS ] // X=24 px, Y=24 px")))
						.Font(VTTheme::Font(11, true))
						.ColorAndOpacity(FLinearColor(1.0f, 0.88f, 0.2f, 1.0f))
					]

					// Row 1: Target Button Selection Tabs with visual highlighting
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 3.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
						[
							SAssignNew(TabBorderBtnLinks, SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(VTTheme::Gold)
							.Padding(FMargin(2.0f))
							[
								VTTheme::MakeButton(TEXT("◀ LINKS"), VTTheme::CardBg, VTTheme::Cyan, 12, FMargin(18.0f, 10.0f),
									FOnClicked::CreateLambda([this]()
									{
										SelectedEditButtonIndex = 1;
										return FReply::Handled();
									}))
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
						[
							SAssignNew(TabBorderBtnRechts, SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.0f, 0.961f, 0.831f, 0.35f))
							.Padding(FMargin(2.0f))
							[
								VTTheme::MakeButton(TEXT("RECHTS ▶"), VTTheme::CardBg, VTTheme::Cyan, 12, FMargin(18.0f, 10.0f),
									FOnClicked::CreateLambda([this]()
									{
										SelectedEditButtonIndex = 2;
										return FReply::Handled();
									}))
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
						[
							SAssignNew(TabBorderBtnJump, SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.98f, 0.12f, 0.55f, 0.35f))
							.Padding(FMargin(2.0f))
							[
								VTTheme::MakeButton(TEXT("▲▲ JUMP"), VTTheme::CardBg, VTTheme::Pink, 12, FMargin(18.0f, 10.0f),
									FOnClicked::CreateLambda([this]()
									{
										SelectedEditButtonIndex = 3;
										return FReply::Handled();
									}))
							]
						]
					]

					// Row 2: Large 4-Way Directional Placement Controls (96px x 56px per button!)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 6.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f, 0.0f)
						[
							SNew(SBox)
							.WidthOverride(96.0f)
							.HeightOverride(56.0f)
							[
								VTTheme::MakeButton(TEXT("◀ -15px"), VTTheme::CardBg, VTTheme::Cyan, 13, FMargin(8.0f, 6.0f),
									FOnClicked::CreateLambda([this]()
									{
										if (AVTTowerGameMode* GM = GetTowerGM())
										{
											if (GM->ActiveSaveGame)
											{
												FVTHudConfig& Cfg = GM->ActiveSaveGame->HudConfig;
												if (SelectedEditButtonIndex == 1) Cfg.LeftClusterOffsetX = FMath::Clamp(Cfg.LeftClusterOffsetX - 15.0f, 4.0f, 650.0f);
												else if (SelectedEditButtonIndex == 2) Cfg.RightSteerOffsetX = FMath::Clamp(Cfg.RightSteerOffsetX - 15.0f, 4.0f, 650.0f);
												else if (SelectedEditButtonIndex == 3) Cfg.RightClusterOffsetX = FMath::Clamp(Cfg.RightClusterOffsetX + 15.0f, 4.0f, 650.0f);
												GM->SaveGameToDisk();
											}
										}
										return FReply::Handled();
									}))
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f, 0.0f)
						[
							SNew(SBox)
							.WidthOverride(96.0f)
							.HeightOverride(56.0f)
							[
								VTTheme::MakeButton(TEXT("+15px ▶"), VTTheme::CardBg, VTTheme::Cyan, 13, FMargin(8.0f, 6.0f),
									FOnClicked::CreateLambda([this]()
									{
										if (AVTTowerGameMode* GM = GetTowerGM())
										{
											if (GM->ActiveSaveGame)
											{
												FVTHudConfig& Cfg = GM->ActiveSaveGame->HudConfig;
												if (SelectedEditButtonIndex == 1) Cfg.LeftClusterOffsetX = FMath::Clamp(Cfg.LeftClusterOffsetX + 15.0f, 4.0f, 650.0f);
												else if (SelectedEditButtonIndex == 2) Cfg.RightSteerOffsetX = FMath::Clamp(Cfg.RightSteerOffsetX + 15.0f, 4.0f, 650.0f);
												else if (SelectedEditButtonIndex == 3) Cfg.RightClusterOffsetX = FMath::Clamp(Cfg.RightClusterOffsetX - 15.0f, 4.0f, 650.0f);
												GM->SaveGameToDisk();
											}
										}
										return FReply::Handled();
									}))
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f, 3.0f, 0.0f)
						[
							SNew(SBox)
							.WidthOverride(96.0f)
							.HeightOverride(56.0f)
							[
								VTTheme::MakeButton(TEXT("▲ +15px"), VTTheme::CardBg, VTTheme::Gold, 13, FMargin(8.0f, 6.0f),
									FOnClicked::CreateLambda([this]()
									{
										if (AVTTowerGameMode* GM = GetTowerGM())
										{
											if (GM->ActiveSaveGame)
											{
												FVTHudConfig& Cfg = GM->ActiveSaveGame->HudConfig;
												if (SelectedEditButtonIndex == 1) Cfg.LeftClusterOffsetY = FMath::Clamp(Cfg.LeftClusterOffsetY + 15.0f, 4.0f, 850.0f);
												else if (SelectedEditButtonIndex == 2) Cfg.RightSteerOffsetY = FMath::Clamp(Cfg.RightSteerOffsetY + 15.0f, 4.0f, 850.0f);
												else if (SelectedEditButtonIndex == 3) Cfg.RightClusterOffsetY = FMath::Clamp(Cfg.RightClusterOffsetY + 15.0f, 4.0f, 850.0f);
												GM->SaveGameToDisk();
											}
										}
										return FReply::Handled();
									}))
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f, 0.0f)
						[
							SNew(SBox)
							.WidthOverride(96.0f)
							.HeightOverride(56.0f)
							[
								VTTheme::MakeButton(TEXT("-15px ▼"), VTTheme::CardBg, VTTheme::Gold, 13, FMargin(8.0f, 6.0f),
									FOnClicked::CreateLambda([this]()
									{
										if (AVTTowerGameMode* GM = GetTowerGM())
										{
											if (GM->ActiveSaveGame)
											{
												FVTHudConfig& Cfg = GM->ActiveSaveGame->HudConfig;
												if (SelectedEditButtonIndex == 1) Cfg.LeftClusterOffsetY = FMath::Clamp(Cfg.LeftClusterOffsetY - 15.0f, 4.0f, 850.0f);
												else if (SelectedEditButtonIndex == 2) Cfg.RightSteerOffsetY = FMath::Clamp(Cfg.RightSteerOffsetY - 15.0f, 4.0f, 850.0f);
												else if (SelectedEditButtonIndex == 3) Cfg.RightClusterOffsetY = FMath::Clamp(Cfg.RightClusterOffsetY - 15.0f, 4.0f, 850.0f);
												GM->SaveGameToDisk();
											}
										}
										return FReply::Handled();
									}))
							]
						]
					]

					// Row 3: Live Transparency & Size Steppers
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 5.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(3.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("TRANSP:"))).Font(VTTheme::Font(11, true)).ColorAndOpacity(VTTheme::Cyan)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f, 0.0f)
						[
							VTTheme::MakeButton(TEXT("-10%"), VTTheme::CardBg, VTTheme::Cyan, 11, FMargin(14.0f, 8.0f),
								FOnClicked::CreateLambda([this]()
								{
									if (AVTTowerGameMode* GM = GetTowerGM())
									{
										if (GM->ActiveSaveGame)
										{
											GM->ActiveSaveGame->HudConfig.OpacityPct = FMath::Clamp(GM->ActiveSaveGame->HudConfig.OpacityPct - 10, 15, 95);
											GM->SaveGameToDisk();
										}
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f, 0.0f)
						[
							SAssignNew(TxtEditorOpacity, STextBlock).Text(FText::FromString(TEXT("50%"))).Font(VTTheme::Font(12, true)).ColorAndOpacity(VTTheme::Gold)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f, 0.0f)
						[
							VTTheme::MakeButton(TEXT("+10%"), VTTheme::CardBg, VTTheme::Cyan, 11, FMargin(14.0f, 8.0f),
								FOnClicked::CreateLambda([this]()
								{
									if (AVTTowerGameMode* GM = GetTowerGM())
									{
										if (GM->ActiveSaveGame)
										{
											GM->ActiveSaveGame->HudConfig.OpacityPct = FMath::Clamp(GM->ActiveSaveGame->HudConfig.OpacityPct + 10, 15, 95);
											GM->SaveGameToDisk();
										}
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f, 3.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("GRÖSSE:"))).Font(VTTheme::Font(11, true)).ColorAndOpacity(VTTheme::Gold)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f, 0.0f)
						[
							VTTheme::MakeButton(TEXT("-10%"), VTTheme::CardBg, VTTheme::Gold, 11, FMargin(14.0f, 8.0f),
								FOnClicked::CreateLambda([this]()
								{
									if (AVTTowerGameMode* GM = GetTowerGM())
									{
										if (GM->ActiveSaveGame)
										{
											GM->ActiveSaveGame->HudConfig.SizePct = FMath::Clamp(GM->ActiveSaveGame->HudConfig.SizePct - 10, 65, 150);
											GM->SaveGameToDisk();
										}
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f, 0.0f)
						[
							SAssignNew(TxtEditorSize, STextBlock).Text(FText::FromString(TEXT("100%"))).Font(VTTheme::Font(12, true)).ColorAndOpacity(VTTheme::Gold)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f, 0.0f)
						[
							VTTheme::MakeButton(TEXT("+10%"), VTTheme::CardBg, VTTheme::Gold, 11, FMargin(14.0f, 8.0f),
								FOnClicked::CreateLambda([this]()
								{
									if (AVTTowerGameMode* GM = GetTowerGM())
									{
										if (GM->ActiveSaveGame)
										{
											GM->ActiveSaveGame->HudConfig.SizePct = FMath::Clamp(GM->ActiveSaveGame->HudConfig.SizePct + 10, 65, 150);
											GM->SaveGameToDisk();
										}
									}
									return FReply::Handled();
								}))
						]
					]

					// Row 4: Presets, Reset & Save
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 7.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
						[
							VTTheme::MakeButton(TEXT("RESET"), VTTheme::CardBg, VTTheme::Pink, 11, FMargin(16.0f, 10.0f),
								FOnClicked::CreateLambda([this]()
								{
									if (AVTTowerGameMode* GM = GetTowerGM())
									{
										if (GM->ActiveSaveGame)
										{
											GM->ActiveSaveGame->HudConfig.LeftClusterOffsetX = 24.0f;
											GM->ActiveSaveGame->HudConfig.LeftClusterOffsetY = 24.0f;
											GM->ActiveSaveGame->HudConfig.RightSteerOffsetX = 114.0f;
											GM->ActiveSaveGame->HudConfig.RightSteerOffsetY = 24.0f;
											GM->ActiveSaveGame->HudConfig.RightClusterOffsetX = 24.0f;
											GM->ActiveSaveGame->HudConfig.RightClusterOffsetY = 24.0f;
											GM->ActiveSaveGame->HudConfig.OpacityPct = 50;
											GM->ActiveSaveGame->HudConfig.SizePct = 100;
											GM->SaveGameToDisk();
										}
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
						[
							VTTheme::MakeButton(TEXT("STANDARD-SPLIT"), VTTheme::CardBg, VTTheme::Cyan, 11, FMargin(16.0f, 10.0f),
								FOnClicked::CreateLambda([this]()
								{
									if (AVTTowerGameMode* GM = GetTowerGM())
									{
										if (GM->ActiveSaveGame)
										{
											GM->ActiveSaveGame->HudConfig.LeftClusterOffsetX = 24.0f;
											GM->ActiveSaveGame->HudConfig.LeftClusterOffsetY = 24.0f;
											GM->ActiveSaveGame->HudConfig.RightSteerOffsetX = 114.0f;
											GM->ActiveSaveGame->HudConfig.RightSteerOffsetY = 24.0f;
											GM->ActiveSaveGame->HudConfig.RightClusterOffsetX = 24.0f;
											GM->ActiveSaveGame->HudConfig.RightClusterOffsetY = 24.0f;
											GM->SaveGameToDisk();
										}
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
						[
							VTTheme::MakeButton(TEXT("✔ FERTIG & SPEICHERN"), VTTheme::Emerald, FLinearColor::White, 12, FMargin(20.0f, 10.0f),
								FOnClicked::CreateLambda([this]()
								{
									bHudDragEditMode = false;
									DraggingClusterIndex = 0;
									if (AVTTowerGameMode* GM = GetTowerGM()) GM->SaveGameToDisk();
									OpenModal(EVTModalOverlay::SettingsAndControls);
									return FReply::Handled();
								}))
						]
					]
				]
			]
		]

		// 1D. Compact, Mobile-Safe In-Game Popups & Combo Pill (Strictly inside the 9:16 Phone Screen!)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(0.0f, 3.0f)
		[
			SNew(SBox)
			.WidthOverride(330.0f)
			[
				SNew(SVerticalBox)

				// Top Row inside 9:16 Tower Screen: Compact Combo Pill (Left inside tower) + Compact Toast/Level-Up Pill (Right/Center inside tower)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 1.0f)
				[
					SNew(SHorizontalBox)

					// Prominent Mobile Combo & Item-Multiplier Badge (Inside top-left of 9:16 phone screen!)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Top)
					.Padding(0.0f, 0.0f, 6.0f, 0.0f)
					[
						SAssignNew(ComboHudBorder, SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(VTTheme::Gold)
						.Padding(FMargin(2.0f))
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.015f, 0.030f, 0.075f, 0.95f))
							.Padding(FMargin(9.0f, 4.0f))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
									[
										SAssignNew(TxtComboMult, STextBlock)
										.Text(FText::FromString(TEXT("2x")))
										.Font(VTTheme::Font(14, true))
										.ColorAndOpacity(VTTheme::Gold)
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[
										SAssignNew(TxtComboTierLabel, STextBlock)
										.Text(FText::FromString(TEXT("COMBO // ITEM-PTS x3")))
										.Font(VTTheme::Font(7, true))
										.ColorAndOpacity(VTTheme::Cyan)
									]
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.5f, 0.0f, 0.0f)
								[
									SNew(SBox)
									.WidthOverride(110.0f)
									.HeightOverride(4.5f)
									[
										SNew(SBorder)
										.BorderImage(WhiteBrush)
										.BorderBackgroundColor(FLinearColor(0.08f, 0.14f, 0.25f, 0.95f))
										[
											SNew(SHorizontalBox)
											+ SHorizontalBox::Slot().AutoWidth()
											[
												SAssignNew(ComboFuseFillBox, SBox)
												.WidthOverride(0.0f)
												[
													SAssignNew(ComboFuseFillBorder, SBorder)
													.BorderImage(WhiteBrush)
													.BorderBackgroundColor(VTTheme::Gold)
												]
											]
										]
									]
								]
							]
						]
					]

					+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]

					// Prominent Quake Announcer Callout Pill (Inside top-right of 9:16 phone screen!)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Top)
					[
						SAssignNew(AnnouncerBannerBorder, SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(VTTheme::Gold)
						.Padding(FMargin(2.0f))
						.Visibility(EVisibility::Collapsed)
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.02f, 0.04f, 0.11f, 0.95f))
							.Padding(FMargin(9.0f, 4.0f))
							[
								SAssignNew(TxtAnnouncerCallout, STextBlock)
								.Text(FText::FromString(TEXT("EXCELLENT!")))
								.Font(VTTheme::Font(9, true))
								.ColorAndOpacity(VTTheme::Gold)
							]
						]
					]
				]

				// Compact Level-Up Pill (Sleek, small, centered inside 9:16 tower screen)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 1.5f)
				[
					SAssignNew(LevelUpBannerBorder, SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(VTTheme::Gold)
					.Padding(FMargin(1.5f))
					.Visibility(EVisibility::Collapsed)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(0.02f, 0.05f, 0.14f, 0.92f))
						.Padding(FMargin(10.0f, 3.0f))
						[
							SAssignNew(TxtLevelUpBanner, STextBlock)
							.Text(FText::FromString(TEXT("LEVEL UP! LVL 2 (+25 CORES)")))
							.Font(VTTheme::Font(8, true))
							.ColorAndOpacity(VTTheme::Gold)
						]
					]
				]

				// Compact Toast Pill (Sleek, small, centered inside 9:16 tower screen)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 1.0f)
				[
					SAssignNew(ToastBannerBorder, SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(VTTheme::Cyan)
					.Padding(FMargin(1.0f))
					.Visibility(EVisibility::Collapsed)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(0.02f, 0.04f, 0.10f, 0.90f))
						.Padding(FMargin(8.0f, 2.5f))
						[
							SAssignNew(TxtToastMessage, STextBlock)
							.Text(FText::FromString(TEXT("TOAST")))
							.Font(VTTheme::Font(7, true))
							.ColorAndOpacity(VTTheme::Cyan)
						]
					]
				]

				// Compact Hurry-Up & Power-Up Mini Pills
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 1.0f)
				[
					SAssignNew(HurryBannerBorder, SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(VTTheme::Pink)
					.Padding(FMargin(8.0f, 2.0f))
					.Visibility(EVisibility::Collapsed)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("TEMPO & BPM UP!")))
						.Font(VTTheme::Font(7, true))
						.ColorAndOpacity(FLinearColor::White)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 1.0f)
				[
					SAssignNew(PowerUpBannerBorder, SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(FLinearColor(0.03f, 0.06f, 0.12f, 0.90f))
					.Padding(FMargin(8.0f, 2.0f))
					.Visibility(EVisibility::Collapsed)
					[
						SAssignNew(TxtPowerUpBanner, STextBlock)
						.Text(FText::FromString(TEXT("POWER-UP AKTIV")))
						.Font(VTTheme::Font(7, true))
						.ColorAndOpacity(VTTheme::Gold)
					]
				]
			]
		]

		// 1E. Open Middle Playfield Stage Spacer
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSpacer)
		]

		// 1F. Bottom Safe Spacer inside Layer 1
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Fill)
		.Padding(0.0f)
		[
			SNew(SSpacer).Size(FVector2D(0.0f, 24.0f))
		]
	]

	// =========================================================================
	// LAYER 1-TOUCH: FULL-SCREEN FLOATING CYBERPUNK ARCADE TOUCH CONTROLS
	// =========================================================================
	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SAssignNew(TouchHudContainer, SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(FLinearColor::Transparent)
		.Padding(0.0f)
		[
			SNew(SOverlay)

			// 1. BUTTON LINKS: Independently Movable Sleek Cyber-Paddle (Anchored Bottom-Left)
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Bottom)
			[
				SAssignNew(LeftTouchClusterBorder, SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				.BorderBackgroundColor(FLinearColor::Transparent)
				.Padding(FMargin(24.0f, 0.0f, 0.0f, 24.0f))
				[
					SAssignNew(LeftClusterSizeBox, SBox)
					.WidthOverride(80.0f)
					.HeightOverride(70.0f)
					[
						SNew(SButton)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
						.ContentPadding(FMargin(0.0f))
						.OnPressed_Lambda([this]()
						{
							if (bHudDragEditMode)
							{
								SelectedEditButtonIndex = 1;
								DraggingClusterIndex = 1;
								bDragJustStarted = true;
								if (AVTPlayerController* PC = GetTowerPC())
								{
									float CurX = 0.0f, CurY = 0.0f;
									bool bTouch = false;
									PC->GetInputTouchState(ETouchIndex::Touch1, CurX, CurY, bTouch);
									if (bTouch) DragLastMousePos = FVector2D(CurX, CurY);
									else if (PC->GetMousePosition(CurX, CurY)) DragLastMousePos = FVector2D(CurX, CurY);
								}
							}
							else if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->bKeyLeft = true;
								PC->bKeyRight = false;
								PC->AnalogSteer = -1.0f;
								PC->TriggerHapticFeedback(14);
							}
						})
						.OnReleased_Lambda([this]()
						{
							if (bHudDragEditMode)
							{
								DraggingClusterIndex = 0;
							}
							else if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->bKeyLeft = false;
								PC->AnalogSteer = 0.0f;
							}
						})
						[
							SAssignNew(LeftButtonHighlightBorder, SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.0f, 0.961f, 0.831f, 0.90f))
							.Padding(FMargin(2.0f))
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(FLinearColor(0.012f, 0.035f, 0.082f, 0.78f))
								.HAlign(HAlign_Center).VAlign(VAlign_Center)
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 3.0f, 0.0f, 0.0f)
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("◀"))).Font(VTTheme::Font(20, true)).ColorAndOpacity(VTTheme::Cyan)
									]
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 1.0f, 0.0f, 3.0f)
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("LINKS"))).Font(VTTheme::Font(8, true)).ColorAndOpacity(FLinearColor(0.85f, 0.95f, 1.0f, 0.95f))
									]
								]
							]
						]
					]
				]
			]

			// 2. BUTTON RECHTS: Independently Movable Sleek Cyber-Paddle (Anchored Bottom-Left, with distinct X offset)
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Bottom)
			[
				SAssignNew(RightSteerClusterBorder, SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				.BorderBackgroundColor(FLinearColor::Transparent)
				.Padding(FMargin(108.0f, 0.0f, 0.0f, 24.0f))
				[
					SAssignNew(RightSteerSizeBox, SBox)
					.WidthOverride(80.0f)
					.HeightOverride(70.0f)
					[
						SNew(SButton)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
						.ContentPadding(FMargin(0.0f))
						.OnPressed_Lambda([this]()
						{
							if (bHudDragEditMode)
							{
								SelectedEditButtonIndex = 2;
								DraggingClusterIndex = 2;
								bDragJustStarted = true;
								if (AVTPlayerController* PC = GetTowerPC())
								{
									float CurX = 0.0f, CurY = 0.0f;
									bool bTouch = false;
									PC->GetInputTouchState(ETouchIndex::Touch1, CurX, CurY, bTouch);
									if (bTouch) DragLastMousePos = FVector2D(CurX, CurY);
									else if (PC->GetMousePosition(CurX, CurY)) DragLastMousePos = FVector2D(CurX, CurY);
								}
							}
							else if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->bKeyRight = true;
								PC->bKeyLeft = false;
								PC->AnalogSteer = 1.0f;
								PC->TriggerHapticFeedback(14);
							}
						})
						.OnReleased_Lambda([this]()
						{
							if (bHudDragEditMode)
							{
								DraggingClusterIndex = 0;
							}
							else if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->bKeyRight = false;
								PC->AnalogSteer = 0.0f;
							}
						})
						[
							SAssignNew(RightButtonHighlightBorder, SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.0f, 0.961f, 0.831f, 0.90f))
							.Padding(FMargin(2.0f))
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(FLinearColor(0.012f, 0.035f, 0.082f, 0.78f))
								.HAlign(HAlign_Center).VAlign(VAlign_Center)
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 3.0f, 0.0f, 0.0f)
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("▶"))).Font(VTTheme::Font(20, true)).ColorAndOpacity(VTTheme::Cyan)
									]
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 1.0f, 0.0f, 3.0f)
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("RECHTS"))).Font(VTTheme::Font(8, true)).ColorAndOpacity(FLinearColor(0.85f, 0.95f, 1.0f, 0.95f))
									]
								]
							]
						]
					]
				]
			]

			// 3. BUTTON JUMP: Independently Movable Glowing Cyber-Disc (Anchored Bottom-Right)
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			[
				SAssignNew(RightTouchClusterBorder, SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				.BorderBackgroundColor(FLinearColor::Transparent)
				.Padding(FMargin(0.0f, 0.0f, 24.0f, 24.0f))
				[
					SAssignNew(RightClusterSizeBox, SBox)
					.WidthOverride(92.0f)
					.HeightOverride(92.0f)
					[
						SNew(SButton)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
						.ContentPadding(FMargin(0.0f))
						.OnPressed_Lambda([this]()
						{
							if (bHudDragEditMode)
							{
								SelectedEditButtonIndex = 3;
								DraggingClusterIndex = 3;
								bDragJustStarted = true;
								if (AVTPlayerController* PC = GetTowerPC())
								{
									float CurX = 0.0f, CurY = 0.0f;
									bool bTouch = false;
									PC->GetInputTouchState(ETouchIndex::Touch1, CurX, CurY, bTouch);
									if (bTouch) DragLastMousePos = FVector2D(CurX, CurY);
									else if (PC->GetMousePosition(CurX, CurY)) DragLastMousePos = FVector2D(CurX, CurY);
								}
							}
							else if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->TriggerJumpAction();
								PC->TriggerHapticFeedback(22);
							}
						})
						.OnReleased_Lambda([this]()
						{
							if (bHudDragEditMode)
							{
								DraggingClusterIndex = 0;
							}
							else if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->ReleaseJumpAction();
							}
						})
						[
							SAssignNew(JumpButtonHighlightBorder, SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.98f, 0.12f, 0.55f, 0.92f))
							.Padding(FMargin(2.0f))
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(FLinearColor(0.08f, 0.015f, 0.045f, 0.80f))
								.HAlign(HAlign_Center).VAlign(VAlign_Center)
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 4.0f, 0.0f, 0.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("▲▲")))
										.Font(VTTheme::Font(20, true))
										.ColorAndOpacity(FLinearColor(1.0f, 0.92f, 0.35f, 0.98f))
									]
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 1.0f, 0.0f, 4.0f)
									[
										SAssignNew(TxtJumpButtonLabel, STextBlock)
										.Text(FText::FromString(TEXT("JUMP")))
										.Font(VTTheme::Font(10, true))
										.ColorAndOpacity(VTTheme::Pink)
									]
								]
							]
						]
					]
				]
			]

			// 4. 1:1 INTERACTIVE BOTTOM CONTROL BAR HITBOXES (CRAZY CLIMBER < / > | GLUE | JUMP | SHOP | START!)
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Bottom)
			[
				SNew(SBox)
				.HeightOverride(96.0f)
				[
					SNew(SHorizontalBox)

					// 4A. Bottom-Left "<" (CRAZY CLIMBER Left)
					+ SHorizontalBox::Slot().FillWidth(0.16f)
					[
						SNew(SButton)
						.IsFocusable(false)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
						.OnPressed_Lambda([this]()
						{
							if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->bKeyLeft = true;
								PC->bKeyRight = false;
								PC->AnalogSteer = -1.0f;
								PC->TriggerHapticFeedback(14);
							}
						})
						.OnReleased_Lambda([this]()
						{
							if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->bKeyLeft = false;
								PC->AnalogSteer = 0.0f;
							}
						})
						[ SNew(SSpacer) ]
					]

					// 4B. Bottom-Left ">" (CRAZY CLIMBER Right)
					+ SHorizontalBox::Slot().FillWidth(0.16f)
					[
						SNew(SButton)
						.IsFocusable(false)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
						.OnPressed_Lambda([this]()
						{
							if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->bKeyRight = true;
								PC->bKeyLeft = false;
								PC->AnalogSteer = 1.0f;
								PC->TriggerHapticFeedback(14);
							}
						})
						.OnReleased_Lambda([this]()
						{
							if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->bKeyRight = false;
								PC->AnalogSteer = 0.0f;
							}
						})
						[ SNew(SSpacer) ]
					]

					// 4C. "GLUE" Button (Activates Time-Freeze / Glue Slowdown!)
					+ SHorizontalBox::Slot().FillWidth(0.12f)
					[
						SNew(SButton)
						.IsFocusable(false)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
						.OnClicked_Lambda([this]()
						{
							if (AVTTowerGameMode* GM = GetTowerGM())
							{
								GM->FreezeFrames = 360;
								GM->PlayTowerSFX(TEXT("PowerUp_Freeze"), 1.0f, 1.0f);
								GM->SpawnFloatingText(TEXT("GLUE AKTIV! (KAMERA GESTOPPT)"), 0.0f, GM->CameraZ, FLinearColor(0.98f, 0.85f, 0.20f), 16);
							}
							return FReply::Handled();
						})
						[ SNew(SSpacer) ]
					]

					// 4D. Center Hanger-Tab "JUMP" Button
					+ SHorizontalBox::Slot().FillWidth(0.24f)
					[
						SNew(SButton)
						.IsFocusable(false)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
						.OnPressed_Lambda([this]()
						{
							if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->TriggerJumpAction();
								PC->TriggerHapticFeedback(22);
							}
						})
						.OnReleased_Lambda([this]()
						{
							if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->ReleaseJumpAction();
							}
						})
						[ SNew(SSpacer) ]
					]

					// 4E. "SHOP" Button (Opens 10-World Theme Selector / Shop)
					+ SHorizontalBox::Slot().FillWidth(0.14f)
					[
						SNew(SButton)
						.IsFocusable(false)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
						.OnClicked_Lambda([this]()
						{
							OpenThemeSelectModal();
							return FReply::Handled();
						})
						[ SNew(SSpacer) ]
					]

					// 4F. "START!" Button (Starts fresh run / Launches climber!)
					+ SHorizontalBox::Slot().FillWidth(0.18f)
					[
						SNew(SButton)
						.IsFocusable(false)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
						.OnClicked_Lambda([this]()
						{
							if (AVTTowerGameMode* GM = GetTowerGM())
							{
								GM->StartNewRun();
							}
							if (AVTPlayerController* PC = GetTowerPC())
							{
								PC->TriggerJumpAction();
							}
							return FReply::Handled();
						})
						[ SNew(SSpacer) ]
					]
				]
			]

			// 5. COMPACT 1-CLICK LIVE WORLD SWITCHER BAR (Top-Center below Header so all 10 1:1 Worlds can be tested anytime!)
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top)
			.Padding(FMargin(0.0f, 88.0f, 0.0f, 0.0f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.12f, 0.08f, 0.05f, 0.82f))
				.Padding(FMargin(4.0f, 2.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
					[
						VTTheme::MakeButton(TEXT("◀ WELT"), FLinearColor(0.20f, 0.14f, 0.08f, 0.95f), VTTheme::Gold, 8, FMargin(7.0f, 3.0f),
							FOnClicked::CreateLambda([this]()
							{
								if (AVTTowerGameMode* GM = GetTowerGM())
								{
									GM->SelectThemeDirection(-1);
								}
								return FReply::Handled();
							}))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f)
					[
						SNew(SButton)
						.IsFocusable(false)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
						.OnClicked_Lambda([this]()
						{
							OpenThemeSelectModal();
							return FReply::Handled();
						})
						[
							SAssignNew(TxtLiveThemeSwitcherLabel, STextBlock)
							.Text(FText::FromString(TEXT("WELT 1/10: PAPERCRAFT")))
							.Font(VTTheme::Font(8, true))
							.ColorAndOpacity(FLinearColor(1.0f, 0.95f, 0.80f, 1.0f))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
					[
						VTTheme::MakeButton(TEXT("WELT ▶"), FLinearColor(0.20f, 0.14f, 0.08f, 0.95f), VTTheme::Gold, 8, FMargin(7.0f, 3.0f),
							FOnClicked::CreateLambda([this]()
							{
								if (AVTTowerGameMode* GM = GetTowerGM())
								{
									GM->SelectThemeDirection(1);
								}
								return FReply::Handled();
							}))
					]
				]
			]
		]
	]

	// =========================================================================
	// LAYER 2: GUIDED, UNCLUTTERED AAA MAIN MENU (NANO BANANA 2 ARTWORK + 3D STAGE)
	// =========================================================================
	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SAssignNew(MainMenuOverlayBorder, SBorder)
		.BorderImage(WhiteBrush)
		.BorderBackgroundColor(FLinearColor(0.006f, 0.012f, 0.032f, 0.16f))
		.Padding(FMargin(12.0f, 36.0f, 12.0f, 12.0f))
		[
			SNew(SVerticalBox)

			// 2A. Top Lobby Bar: Player Profile + Live Level/XP Progress + Roster Count + Cores
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				// Left: Player Profile & Level/XP Card
				+ SHorizontalBox::Slot()
				.FillWidth(0.55f)
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
					.ContentPadding(FMargin(0.0f))
					.OnClicked_Lambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click")); OpenModal(EVTModalOverlay::AccountCloudSync); return FReply::Handled(); })
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(VTTheme::Cyan)
						.Padding(FMargin(1.5f))
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(VTTheme::DarkBg)
							.Padding(FMargin(10.0f, 6.0f))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
									[
										SNew(SBorder)
										.BorderImage(WhiteBrush)
										.BorderBackgroundColor(VTTheme::Gold)
										.Padding(FMargin(6.0f, 2.0f))
										[
											SAssignNew(TxtHomeLevelTitle, STextBlock)
											.Text(FText::FromString(TEXT("LVL 1")))
											.Font(VTTheme::Font(8, true))
											.ColorAndOpacity(FLinearColor(0.01f, 0.02f, 0.05f, 1.0f))
										]
									]
									+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
									[
										SAssignNew(TxtHomeNickname, STextBlock)
										.Text(FText::FromString(TEXT("ApexClimber")))
										.Font(VTTheme::Font(10, true))
										.ColorAndOpacity(FLinearColor::White)
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[
										SAssignNew(TxtHomeXpProgress, STextBlock)
										.Text(FText::FromString(TEXT("0/400 XP")))
										.Font(VTTheme::Font(7, true))
										.ColorAndOpacity(VTTheme::Gold)
									]
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 2.0f)
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(FLinearColor(0.10f, 0.18f, 0.32f, 1.0f))
									.Padding(FMargin(1.0f))
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().AutoWidth()
										[
											SAssignNew(HomeXpFillBox, SBox)
											.WidthOverride(36.0f)
											.HeightOverride(5.0f)
											[
												SNew(SBorder)
												.BorderImage(WhiteBrush)
												.BorderBackgroundColor(VTTheme::Gold)
											]
										]
									]
								]
								+ SVerticalBox::Slot().AutoHeight()
								[
									SAssignNew(TxtHomeAccountBadge, STextBlock)
									.Text(FText::FromString(TEXT("KONTO: GAST-MODUS [WECHSELN]")))
									.Font(VTTheme::Font(6, true))
									.ColorAndOpacity(VTTheme::Cyan)
								]
							]
						]
					]
				]

				// Right: Roster Unlocked + Neon Cores Treasury
				+ SHorizontalBox::Slot()
				.FillWidth(0.45f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 2.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
							.ContentPadding(FMargin(0.0f))
							.OnClicked_Lambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click")); OpenModal(EVTModalOverlay::CharacterSelect); return FReply::Handled(); })
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(VTTheme::Purple)
								.Padding(FMargin(1.5f))
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(VTTheme::DarkBg)
									.Padding(FMargin(6.0f, 5.0f))
									.HAlign(HAlign_Center)
									[
										SAssignNew(TxtHomeRosterProgress, STextBlock)
										.Text(FText::FromString(TEXT("HELDEN: 1 / 20")))
										.Font(VTTheme::Font(7, true))
										.ColorAndOpacity(VTTheme::Purple)
									]
								]
							]
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
							.ContentPadding(FMargin(0.0f))
							.OnClicked_Lambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click")); OpenModal(EVTModalOverlay::ThemeSelect); return FReply::Handled(); })
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(VTTheme::Gold)
								.Padding(FMargin(1.5f))
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(VTTheme::DarkBg)
									.Padding(FMargin(6.0f, 5.0f))
									.HAlign(HAlign_Center)
									[
										SAssignNew(TxtHomeThemeProgress, STextBlock)
										.Text(FText::FromString(TEXT("THEMES: 1 / 8")))
										.Font(VTTheme::Font(7, true))
										.ColorAndOpacity(VTTheme::Gold)
									]
								]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SButton)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
						.ContentPadding(FMargin(0.0f))
						.OnClicked_Lambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click")); ActiveShopTab = 0; OpenModal(EVTModalOverlay::NeonShop); return FReply::Handled(); })
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(VTTheme::Gold)
							.Padding(FMargin(1.5f))
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(VTTheme::DarkBg)
								.Padding(FMargin(10.0f, 5.0f))
								.HAlign(HAlign_Center)
								[
									SAssignNew(TxtHomeCores, STextBlock)
									.Text(FText::FromString(TEXT("NEON CORES: 65 [+]")))
									.Font(VTTheme::Font(9, true))
									.ColorAndOpacity(VTTheme::Gold)
								]
							]
						]
					]
				]
			]

			// 2B. NANO BANANA 2 HERO KEY-ART BANNER + OPEN 3D SHOWROOM STAGE
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(VTTheme::Cyan)
				.Padding(FMargin(2.0f))
				[
					SNew(SBox)
					.HeightOverride(132.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SImage)
							.Image(MainBannerImg)
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SOverlay::Slot()
						.VAlign(VAlign_Bottom)
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.008f, 0.016f, 0.045f, 0.86f))
							.Padding(FMargin(12.0f, 5.0f))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
								[
									SAssignNew(TxtHomeBestRecord, STextBlock)
									.Text(FText::FromString(TEXT("REKORD: ETAGE 125 // 69.079 PTS")))
									.Font(VTTheme::Font(8, true))
									.ColorAndOpacity(VTTheme::Gold)
								]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									SAssignNew(TxtHomeEquippedSummary, STextBlock)
									.Text(FText::FromString(TEXT("AKTIV: #01 KID FLEX // ARCADE")))
									.Font(VTTheme::Font(8, true))
									.ColorAndOpacity(VTTheme::Cyan)
								]
							]
						]
					]
				]
			]

			// Open Middle 3D Showroom Window so the 3D Climber & Lumen Tower shine!
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SSpacer)
			]

			// 2C. GUIDED, UNCLUTTERED BOTTOM ACTION DECK (Step 1: Choose Game Mode -> Step 2: Classic Character Select -> Climb!)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(VTTheme::Cyan)
				.Padding(FMargin(2.0f))
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(VTTheme::DarkBg)
					.Padding(FMargin(14.0f, 12.0f))
					[
						SNew(SVerticalBox)

						// GUIDED PRIMARY PLAY CTA BUTTON -> Opens Step 1: Game Mode Selection (which then leads to Step 2: Classic Character Selection!)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 10.0f)
						[
							SNew(SButton)
							.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
							.ContentPadding(FMargin(0.0f))
							.OnClicked_Lambda([this]()
							{
								if (AVTTowerGameMode* GM = GetTowerGM())
								{
									GM->PlayTowerSFX(TEXT("ComboUp"), 1.15f, 1.0f);
								}
								OpenModal(EVTModalOverlay::ModeSelect);
								return FReply::Handled();
							})
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(VTTheme::Emerald)
								.Padding(FMargin(2.5f))
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(FLinearColor(0.015f, 0.24f, 0.16f, 0.98f))
									.Padding(FMargin(16.0f, 12.0f))
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("SPIELEN // SPIELMODUS & CHARAKTER WAEHLEN >>")))
											.Font(VTTheme::Font(14, true))
											.ColorAndOpacity(FLinearColor::White)
										]
										+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 0.0f)
										[
											SAssignNew(TxtHomePlayBtnSub, STextBlock)
											.Text(FText::FromString(TEXT("Schritt 1: Modus waehlen  ->  Schritt 2: Klassische Charakter-Auswahl  ->  Start!")))
											.Font(VTTheme::Font(8, true))
											.ColorAndOpacity(VTTheme::Gold)
										]
									]
								]
							]
						]

						// 6 Clean, Guided Navigation Dock Cards (Including 3D Themes & Leaderboards!)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeDockTabButton(
									TEXT("3D VORSCHAU"),
									TEXT("CHARAKTERE"),
									TEXT("20 Helden"),
									VTTheme::Cyan,
									FOnClicked::CreateLambda([this]()
									{
										if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click"));
										OpenModal(EVTModalOverlay::CharacterSelect);
										return FReply::Handled();
									}))
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeDockTabButton(
									TEXT("TOWER DESIGN"),
									TEXT("THEMES"),
									TEXT("8 Welten"),
									VTTheme::Gold,
									FOnClicked::CreateLambda([this]()
									{
										if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click"));
										OpenModal(EVTModalOverlay::ThemeSelect);
										return FReply::Handled();
									}))
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeDockTabButton(
									TEXT("GLOBAL & FREUNDE"),
									TEXT("BESTENLISTE"),
									TEXT("Platzierung"),
									VTTheme::Emerald,
									FOnClicked::CreateLambda([this]()
									{
										if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click"));
										OpenModal(EVTModalOverlay::FriendsAndRivals);
										return FReply::Handled();
									}))
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeDockTabButton(
									TEXT("HELDEN & SKINS"),
									TEXT("NEON SHOP"),
									TEXT("Freischalten"),
									VTTheme::Pink,
									FOnClicked::CreateLambda([this]()
									{
										if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click"));
										ActiveShopTab = 2;
										OpenModal(EVTModalOverlay::NeonShop);
										return FReply::Handled();
									}))
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeDockTabButton(
									TEXT("LOOT & QUESTS"),
									TEXT("BELOHNUNG"),
									TEXT("Gluecksrad"),
									VTTheme::Gold,
									FOnClicked::CreateLambda([this]()
									{
										if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click"));
										OpenModal(EVTModalOverlay::RewardsHub);
										return FReply::Handled();
									}))
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeDockTabButton(
									TEXT("TECHNIK & AUDIO"),
									TEXT("EINSTELLUNGEN"),
									TEXT("Steuerung"),
									VTTheme::Purple,
									FOnClicked::CreateLambda([this]()
									{
										if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click"));
										OpenModal(EVTModalOverlay::SettingsAndControls);
										return FReply::Handled();
									}))
							]
						]
					]
				]
			]
		]
	]

	// =========================================================================
	// LAYER 3: GAME OVER, LIVE LEADERBOARD PLACEMENT (GLOBAL & FRIENDS) & REVIVE
	// =========================================================================
	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SAssignNew(GameOverOverlayBorder, SBorder)
		.BorderImage(WhiteBrush)
		.BorderBackgroundColor(FLinearColor(0.005f, 0.012f, 0.032f, 0.94f))
		.Padding(FMargin(10.0f, 32.0f, 10.0f, 12.0f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(EVisibility::Collapsed)
		[
			SNew(SBox)
			.WidthOverride(420.0f)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(VTTheme::Pink)
				.Padding(FMargin(2.0f))
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(VTTheme::DarkBg)
					.Padding(FMargin(14.0f, 10.0f))
					[
						SNew(SVerticalBox)

						// Header: Run Terminated + Reason
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
							[
								SAssignNew(TxtOverBadge, STextBlock)
								.Text(FText::FromString(TEXT("TOWER COLLAPSED")))
								.Font(VTTheme::Font(8, true))
								.ColorAndOpacity(VTTheme::Pink)
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("// RUN BEENDET")))
								.Font(VTTheme::Font(12, true))
								.ColorAndOpacity(FLinearColor::White)
							]
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 1.0f, 0.0f, 6.0f)
						[
							SAssignNew(TxtOverSubtext, STextBlock)
							.Text(FText::FromString(TEXT("Die Kamera hat dich eingeholt!")))
							.Font(VTTheme::Font(7, false))
							.ColorAndOpacity(VTTheme::MutedText)
						]

						// Compact 4-Stat Row (SCORE, FLOOR, COMBO, BPM)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
							[
								SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(VTTheme::CardBg).Padding(FMargin(6.0f, 5.0f))
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("SCORE"))).Font(VTTheme::Font(6, true)).ColorAndOpacity(VTTheme::MutedText)
									]
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
									[
										SAssignNew(TxtOverScore, STextBlock).Text(FText::FromString(TEXT("0"))).Font(VTTheme::Font(11, true)).ColorAndOpacity(FLinearColor::White)
									]
								]
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
							[
								SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(VTTheme::CardBg).Padding(FMargin(6.0f, 5.0f))
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("ETAGE"))).Font(VTTheme::Font(6, true)).ColorAndOpacity(VTTheme::MutedText)
									]
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
									[
										SAssignNew(TxtOverFloor, STextBlock).Text(FText::FromString(TEXT("0"))).Font(VTTheme::Font(11, true)).ColorAndOpacity(VTTheme::Cyan)
									]
								]
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
							[
								SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(VTTheme::CardBg).Padding(FMargin(6.0f, 5.0f))
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("COMBO"))).Font(VTTheme::Font(6, true)).ColorAndOpacity(VTTheme::MutedText)
									]
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
									[
										SAssignNew(TxtOverCombo, STextBlock).Text(FText::FromString(TEXT("0x"))).Font(VTTheme::Font(11, true)).ColorAndOpacity(VTTheme::Gold)
									]
								]
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
							[
								SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(VTTheme::CardBg).Padding(FMargin(6.0f, 5.0f))
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("PEAK BPM"))).Font(VTTheme::Font(6, true)).ColorAndOpacity(VTTheme::MutedText)
									]
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
									[
										SAssignNew(TxtOverBpm, STextBlock).Text(FText::FromString(TEXT("120"))).Font(VTTheme::Font(10, true)).ColorAndOpacity(VTTheme::Pink)
									]
								]
							]
						]

						// =========================================================
						// POST-GAME LEADERBOARD PLACEMENT & FRIENDS COMPARISON
						// =========================================================
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 4.0f)
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(VTTheme::Cyan)
							.Padding(FMargin(1.5f))
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(FLinearColor(0.015f, 0.030f, 0.072f, 0.98f))
								.Padding(FMargin(8.0f, 6.0f))
								[
									SNew(SVerticalBox)

									// Placement Summary Headline
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 5.0f)
									[
										SAssignNew(TxtOverPlacementSummary, STextBlock)
										.Text(FText::FromString(TEXT("DEINE PLATZIERUNG: WELTWEIT #8  //  UNTER FREUNDEN #5 VON 9")))
										.Font(VTTheme::Font(8, true))
										.ColorAndOpacity(VTTheme::Gold)
									]

									// 2 Tabs: [WELTWEIT / BESTENLISTE] vs. [NUR MIT FREUNDEN VERGLEICHEN]
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 3.0f, 0.0f)
										[
											SNew(SButton)
											.IsFocusable(false)
											.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
											.ContentPadding(FMargin(0.0f))
											.OnClicked_Lambda([this]()
											{
												if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click"));
												bLeaderboardFriendsOnly = false;
												LastBuiltLeaderboardScore = -1;
												RebuildGameOverLeaderboard();
												return FReply::Handled();
											})
											[
												SAssignNew(TabOverGlobalBorder, SBorder)
												.BorderImage(WhiteBrush)
												.BorderBackgroundColor(VTTheme::Cyan)
												.Padding(FMargin(6.0f, 5.0f))
												.HAlign(HAlign_Center)
												[
													SNew(STextBlock)
													.Text(FText::FromString(TEXT("WELTWEIT (BESTENLISTE)")))
													.Font(VTTheme::Font(7, true))
													.ColorAndOpacity(FLinearColor::White)
												]
											]
										]
										+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f, 0.0f, 0.0f)
										[
											SNew(SButton)
											.IsFocusable(false)
											.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
											.ContentPadding(FMargin(0.0f))
											.OnClicked_Lambda([this]()
											{
												if (AVTTowerGameMode* GM = GetTowerGM()) GM->PlayTowerSFX(TEXT("Click"));
												bLeaderboardFriendsOnly = true;
												LastBuiltLeaderboardScore = -1;
												RebuildGameOverLeaderboard();
												return FReply::Handled();
											})
											[
												SAssignNew(TabOverFriendsBorder, SBorder)
												.BorderImage(WhiteBrush)
												.BorderBackgroundColor(VTTheme::CardBg)
												.Padding(FMargin(6.0f, 5.0f))
												.HAlign(HAlign_Center)
												[
													SNew(STextBlock)
													.Text(FText::FromString(TEXT("NUR FREUNDE VERGLEICHEN (9)")))
													.Font(VTTheme::Font(7, true))
													.ColorAndOpacity(FLinearColor::White)
												]
											]
										]
									]

									// Scrollable Leaderboard Rows right inside Game Over!
									+ SVerticalBox::Slot().AutoHeight()
									[
										SNew(SBox)
										.HeightOverride(148.0f)
										[
											SNew(SScrollBox)
											+ SScrollBox::Slot()
											[
												SAssignNew(GameOverLeaderboardListBox, SVerticalBox)
											]
										]
									]
								]
							]
						]

						// 2nd Chance Revive Button
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
						[
							SAssignNew(ReviveButtonContainer, SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor::Transparent)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									VTTheme::MakeButton(TEXT("2. CHANCE: WEITERSPIELEN (25 CORES / VIP GRATIS)"), VTTheme::Gold, FLinearColor::White, 8, FMargin(10.0f, 6.0f),
										FOnClicked::CreateLambda([this]()
										{
											if (AVTTowerGameMode* GM = GetTowerGM())
											{
												const bool bVip = GM->ActiveSaveGame && GM->ActiveSaveGame->bIsVip;
												if (bVip || GM->NeonCores >= 25)
												{
													if (!bVip) GM->AwardNeonCores(-25, TEXT("2nd Chance Revive"));
													GM->ExecuteSecondChanceRevive();
												}
												else
												{
													StartSimulatedAd([this]()
													{
														if (AVTTowerGameMode* InnerGM = GetTowerGM()) InnerGM->ExecuteSecondChanceRevive();
													});
												}
											}
											return FReply::Handled();
										}))
								]
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 1.0f, 0.0f, 0.0f)
								[
									SAssignNew(TxtReviveCountdown, STextBlock)
									.Text(FText::FromString(TEXT("Noch 5s fuer Rettung!")))
									.Font(VTTheme::Font(6, true))
									.ColorAndOpacity(VTTheme::Gold)
								]
							]
						]

						// Play Again, Change Character/Mode & Return to Lobby
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
						[
							VTTheme::MakeButton(TEXT("SOFORT NOCHMAL SPIELEN >>"), VTTheme::Emerald, FLinearColor::White, 10, FMargin(12.0f, 8.0f),
								FOnClicked::CreateLambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) { GM->PlayTowerSFX(TEXT("ComboUp")); GM->StartNewRun(); } return FReply::Handled(); }))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 3.0f, 0.0f)
							[
								VTTheme::MakeButton(TEXT("CHARAKTER / MODUS"), VTTheme::CardBg, VTTheme::Gold, 8, FMargin(8.0f, 6.0f),
									FOnClicked::CreateLambda([this]()
									{
										if (AVTTowerGameMode* GM = GetTowerGM())
										{
											GM->PlayTowerSFX(TEXT("Click"));
											GM->ReturnToMainMenu();
										}
										OpenModal(EVTModalOverlay::CharacterSelect);
										return FReply::Handled();
									}))
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f, 0.0f, 0.0f)
							[
								VTTheme::MakeButton(TEXT("HAUPTMENUE"), VTTheme::CardBg, VTTheme::Cyan, 8, FMargin(8.0f, 6.0f),
									FOnClicked::CreateLambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) { GM->PlayTowerSFX(TEXT("Click")); GM->ReturnToMainMenu(); } return FReply::Handled(); }))
							]
						]
					]
				]
			]
		]
	]

	// =========================================================================
	// LAYER 4: UNIVERSAL MODAL BACKDROP & DYNAMIC CONTENT HOST
	// =========================================================================
	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SAssignNew(ModalBackdropBorder, SBorder)
		.BorderImage(WhiteBrush)
		.BorderBackgroundColor(FLinearColor(0.005f, 0.010f, 0.030f, 0.90f))
		.Padding(FMargin(8.0f, 32.0f, 8.0f, 12.0f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(EVisibility::Collapsed)
		[
			SNew(SBox)
			.WidthOverride(410.0f)
			[
				SAssignNew(ModalOuterFrameBorder, SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(VTTheme::Cyan)
				.Padding(FMargin(2.0f))
				[
					SAssignNew(ModalInnerCardBorder, SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(VTTheme::DarkBg)
					.Padding(FMargin(12.0f, 10.0f))
					[
						SAssignNew(ModalMaxHeightBox, SBox)
						.MaxDesiredHeight(670.0f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SAssignNew(ModalBodyBox, SVerticalBox)
							]
						]
					]
				]
			]
		]
	]

	// =========================================================================
	// LAYER 5A: BOOT PHASE 1 — UNREAL ENGINE 5.8 SPLASH & LOADING SCREEN
	// =========================================================================
	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SAssignNew(EngineSplashOverlayBorder, SBorder)
		.BorderImage(WhiteBrush)
		.BorderBackgroundColor(FLinearColor(0.005f, 0.008f, 0.022f, 0.98f))
		.Padding(FMargin(24.0f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(EVisibility::Visible)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(VTTheme::Cyan)
			.Padding(FMargin(2.5f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.012f, 0.024f, 0.058f, 0.99f))
				.Padding(FMargin(36.0f, 28.0f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 14.0f)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(VTTheme::Cyan)
						.Padding(FMargin(2.5f))
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.01f, 0.03f, 0.08f, 1.0f))
							.Padding(FMargin(22.0f, 12.0f))
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("U")))
								.Font(VTTheme::Font(34, true))
								.ColorAndOpacity(FLinearColor::White)
							]
						]
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("POWERED BY")))
						.Font(VTTheme::Font(8, true))
						.ColorAndOpacity(VTTheme::Cyan)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("UNREAL ENGINE 5.8")))
						.Font(VTTheme::Font(24, true))
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 18.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("LUMEN GLOBAL ILLUMINATION // NANITE 3D GEOMETRY // SYNTH AUDIO ENGINE")))
						.Font(VTTheme::Font(8, true))
						.ColorAndOpacity(VTTheme::Gold)
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 4.0f)
					[
						SNew(SBox)
						.WidthOverride(420.0f)
						.HeightOverride(12.0f)
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.08f, 0.14f, 0.28f, 1.0f))
							.Padding(FMargin(1.5f))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SAssignNew(SplashProgressFillBox, SBox)
									.WidthOverride(40.0f)
									[
										SNew(SBorder)
										.BorderImage(WhiteBrush)
										.BorderBackgroundColor(VTTheme::Cyan)
									]
								]
							]
						]
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f)
					[
						SAssignNew(TxtSplashStatus, STextBlock)
						.Text(FText::FromString(TEXT("INITIALISIERE LUMEN RAY-LIGHTING & SYNTH-AUDIO ENGINE...")))
						.Font(VTTheme::Font(8, true))
						.ColorAndOpacity(VTTheme::MutedText)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 14.0f)
					[
						SAssignNew(TxtSplashPercent, STextBlock)
						.Text(FText::FromString(TEXT("0%")))
						.Font(VTTheme::Font(11, true))
						.ColorAndOpacity(VTTheme::Cyan)
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						VTTheme::MakeButton(TEXT("WEITER ZUM TEASER INTRO >>"), VTTheme::CardBg, VTTheme::Cyan, 8, FMargin(14.0f, 6.0f),
							FOnClicked::CreateLambda([this]()
							{
								if (AVTTowerGameMode* GM = GetTowerGM()) GM->SkipToTeaserIntro();
								return FReply::Handled();
							}))
					]
				]
			]
		]
	]

	// =========================================================================
	// LAYER 5B: BOOT PHASE 2 — 3D CINEMATIC TEASER INTRO OVERLAY
	// =========================================================================
	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SAssignNew(TeaserIntroOverlayBorder, SBorder)
		.BorderImage(WhiteBrush)
		.BorderBackgroundColor(FLinearColor::Transparent)
		.Padding(FMargin(0.0f))
		.Visibility(EVisibility::Collapsed)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.005f, 0.008f, 0.022f, 0.94f))
				.Padding(FMargin(20.0f, 14.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(VTTheme::Pink)
						.Padding(FMargin(8.0f, 3.0f))
						[
							SAssignNew(TxtTeaserPhaseBadge, STextBlock)
							.Text(FText::FromString(TEXT("CINEMATIC TEASER // IN-ENGINE 3D LUMEN")))
							.Font(VTTheme::Font(8, true))
							.ColorAndOpacity(FLinearColor::White)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						VTTheme::MakeButton(TEXT("INTRO UEBERSPRINGEN >>"), VTTheme::CardBg, VTTheme::Gold, 8, FMargin(12.0f, 5.0f),
							FOnClicked::CreateLambda([this]()
							{
								if (AVTTowerGameMode* GM = GetTowerGM()) GM->SkipToAccountGate();
								return FReply::Handled();
							}))
					]
				]
			]

			+ SVerticalBox::Slot().FillHeight(1.0f) [ SNew(SSpacer) ]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(VTTheme::Cyan)
				.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(FLinearColor(0.005f, 0.008f, 0.022f, 0.95f))
					.Padding(FMargin(24.0f, 18.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SAssignNew(TxtTeaserHeadline, STextBlock)
							.Text(FText::FromString(TEXT("NEO-VICE // JAHR 2099")))
							.Font(VTTheme::Font(18, true))
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 4.0f)
						[
							SAssignNew(TxtTeaserSubline, STextBlock)
							.Text(FText::FromString(TEXT("Ein endloser Kryo-Monolith erhebt sich ueber den Wolken. 20 Legenden wagen den Aufstieg.")))
							.Font(VTTheme::Font(10, true))
							.ColorAndOpacity(VTTheme::Cyan)
						]
					]
				]
			]
		]
	]

	// =========================================================================
	// LAYER 5C: BOOT PHASE 3 — AAA ACCOUNT LOGIN, GUEST & 12x 3RD-PARTY SSO GATE
	// =========================================================================
	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SAssignNew(AccountGateOverlayBorder, SBorder)
		.BorderImage(WhiteBrush)
		.BorderBackgroundColor(FLinearColor(0.006f, 0.012f, 0.032f, 0.88f))
		.Padding(FMargin(20.0f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(EVisibility::Collapsed)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(VTTheme::Cyan)
			.Padding(FMargin(2.5f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(VTTheme::DarkBg)
				.Padding(FMargin(22.0f, 16.0f))
				[
					SNew(SBox)
					.WidthOverride(660.0f)
					[
						SNew(SVerticalBox)

						// Nano Banana 2 Key Art Header in Account Gate
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(VTTheme::Cyan)
							.Padding(FMargin(1.5f))
							[
								SNew(SBox)
								.HeightOverride(110.0f)
								[
									SNew(SImage)
									.Image(MainBannerImg)
								]
							]
						]

						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("ANMELDUNG & KONTO-WAHL")))
							.Font(VTTheme::Font(16, true))
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 10.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Spiele sofort als Gast, erstelle ein neues Apex-Konto oder verknuepfe deine Lieblings-App (+500 Neon Cores Bonus)!")))
							.Font(VTTheme::Font(8, false))
							.ColorAndOpacity(VTTheme::MutedText)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
						[
							VTTheme::MakeButton(
								TEXT("ALS GAST FORTFAHREN (SOFORT INS HAUPTMENUE)"),
								VTTheme::Emerald, FLinearColor::White, 12, FMargin(16.0f, 10.0f),
								FOnClicked::CreateLambda([this]()
								{
									if (AVTTowerGameMode* GM = GetTowerGM())
									{
										GM->PlayTowerSFX(TEXT("ComboUp"));
										GM->CompleteAccountLogin(TEXT("guest"), TEXT("GAST-MODUS"), TEXT(""));
									}
									return FReply::Handled();
								}))
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 8.0f)
						[
							VTTheme::MakeButton(
								TEXT("+ NEUES APEX-KONTO ERSTELLEN (INKL. +500 NEON CORES STARTBONUS)"),
								VTTheme::Gold, FLinearColor::White, 10, FMargin(14.0f, 8.0f),
								FOnClicked::CreateLambda([this]()
								{
									if (AVTTowerGameMode* GM = GetTowerGM())
									{
										GM->PlayTowerSFX(TEXT("LevelUp"));
										const FString NewName = FString::Printf(TEXT("ApexLegend_%d"), FMath::RandRange(100, 999));
										GM->CompleteAccountLogin(TEXT("apex_new_account"), TEXT("APEX ID CLOUD"), NewName);
									}
									return FReply::Handled();
								}))
						]

						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 6.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("--- ODER MIT 3RD-PARTY GAMING- & SOCIAL-APPS ANMELDEN (+500 CORES) ---")))
							.Font(VTTheme::Font(8, true))
							.ColorAndOpacity(VTTheme::Cyan)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeButton(TEXT("GOOGLE PLAY"), VTTheme::CardBg, VTTheme::Emerald, 8, FMargin(6.0f, 7.0f),
									FOnClicked::CreateLambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->CompleteAccountLogin(TEXT("googlePlay"), TEXT("GOOGLE PLAY"), TEXT("")); return FReply::Handled(); }))
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeButton(TEXT("APPLE ID"), VTTheme::CardBg, FLinearColor::White, 8, FMargin(6.0f, 7.0f),
									FOnClicked::CreateLambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->CompleteAccountLogin(TEXT("apple"), TEXT("APPLE GAME CENTER"), TEXT("")); return FReply::Handled(); }))
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeButton(TEXT("EPIC GAMES"), VTTheme::CardBg, VTTheme::Gold, 8, FMargin(6.0f, 7.0f),
									FOnClicked::CreateLambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->CompleteAccountLogin(TEXT("epic"), TEXT("EPIC GAMES"), TEXT("")); return FReply::Handled(); }))
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeButton(TEXT("STEAM"), VTTheme::CardBg, VTTheme::SkyBlue, 8, FMargin(6.0f, 7.0f),
									FOnClicked::CreateLambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->CompleteAccountLogin(TEXT("steam"), TEXT("STEAM"), TEXT("")); return FReply::Handled(); }))
							]
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeButton(TEXT("PLAYSTATION"), VTTheme::CardBg, VTTheme::SkyBlue, 8, FMargin(6.0f, 7.0f),
									FOnClicked::CreateLambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->CompleteAccountLogin(TEXT("psn"), TEXT("PLAYSTATION PSN"), TEXT("")); return FReply::Handled(); }))
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeButton(TEXT("XBOX LIVE"), VTTheme::CardBg, VTTheme::Emerald, 8, FMargin(6.0f, 7.0f),
									FOnClicked::CreateLambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->CompleteAccountLogin(TEXT("xbox"), TEXT("XBOX LIVE"), TEXT("")); return FReply::Handled(); }))
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeButton(TEXT("NINTENDO"), VTTheme::CardBg, VTTheme::Pink, 8, FMargin(6.0f, 7.0f),
									FOnClicked::CreateLambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->CompleteAccountLogin(TEXT("nintendo"), TEXT("NINTENDO"), TEXT("")); return FReply::Handled(); }))
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								VTTheme::MakeButton(TEXT("DISCORD"), VTTheme::CardBg, VTTheme::Purple, 8, FMargin(6.0f, 7.0f),
									FOnClicked::CreateLambda([this]() { if (AVTTowerGameMode* GM = GetTowerGM()) GM->CompleteAccountLogin(TEXT("discord"), TEXT("DISCORD"), TEXT("")); return FReply::Handled(); }))
							]
						]
					]
				]
			]
		]
	]

		// =========================================================================
		// LAYER: NANO BANANA 2 "DEAD" CINEMATIC ABYSS POPUP
		// =========================================================================
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(16.0f))
		[
			SAssignNew(DeadBannerOverlayBorder, SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(FLinearColor(1.0f, 0.08f, 0.32f, 0.95f))
			.Padding(FMargin(2.5f))
			.Visibility(EVisibility::Collapsed)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.05f, 0.96f))
				.Padding(FMargin(24.0f, 20.0f))
				[
					SNew(SBox)
					.WidthOverride(380.0f)
					[
						SNew(SVerticalBox)

						// Hazard Header Bar
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.12f, 0.02f, 0.06f, 0.90f))
							.Padding(FMargin(12.0f, 4.0f))
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("⚠ // CRITICAL DESCENT // ⚠")))
								.Font(VTTheme::Font(9, true))
								.ColorAndOpacity(FLinearColor(1.0f, 0.30f, 0.45f, 1.0f))
							]
						]

						// Giant Glitch "WIPEOUT" Headline
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 4.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("⚡ WIPEOUT ⚡")))
							.Font(VTTheme::Font(28, true))
							.ColorAndOpacity(FLinearColor::White)
						]

						// Subtitle: SIGNAL TERMINATED
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 10.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("SYSTEM SIGNAL TERMINATED")))
							.Font(VTTheme::Font(10, true))
							.ColorAndOpacity(VTTheme::Pink)
						]

						// Glowing Neon Divider Line
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
						[
							SNew(SBox).HeightOverride(2.0f)
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(VTTheme::Cyan)
							]
						]

						// Telemetry Info
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 2.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("IN DEN TURM-ABGRUND GESTÜRZT")))
							.Font(VTTheme::Font(9, false))
							.ColorAndOpacity(VTTheme::MutedText)
						]
					]
				]
			]
		]

		// =========================================================================
		// LAYER: NEW HIGH SCORE CELEBRATION BANNER
		// =========================================================================
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(0.0f, 95.0f, 0.0f, 0.0f)
		[
			SAssignNew(HighScoreBannerOverlayBorder, SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(FLinearColor(1.0f, 0.82f, 0.10f, 0.98f))
			.Padding(FMargin(2.0f))
			.Visibility(EVisibility::Collapsed)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.06f, 0.96f))
				.Padding(FMargin(28.0f, 8.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("⚡ NEW HIGH SCORE! ⚡")))
						.Font(VTTheme::Font(18, true))
						.ColorAndOpacity(VTTheme::Gold)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("NEUER REKORD GEKNACKT // WEITER KLETTERN!")))
						.Font(VTTheme::Font(9, true))
						.ColorAndOpacity(VTTheme::Cyan)
					]
				]
			]
		];
}

void UVTMainHUDWidget::OpenModal(EVTModalOverlay ModalType)
{
	ActiveModal = ModalType;
	AVTTowerGameMode* GM = GetTowerGM();
	const bool bIs3DStageModal = (ModalType == EVTModalOverlay::CharacterSelect || ModalType == EVTModalOverlay::ThemeSelect);

	if (GM)
	{
		GM->bInCharacterSelectStage = (ModalType == EVTModalOverlay::CharacterSelect);
		GM->bInThemeSelectStage = (ModalType == EVTModalOverlay::ThemeSelect);
		if (ModalType == EVTModalOverlay::CharacterSelect)
		{
			GM->PreviewClimberOnStage(GM->PreviewClimberId);
		}
		else if (ModalType == EVTModalOverlay::ThemeSelect)
		{
			GM->PreviewThemeOnStage(GM->PreviewThemeId);
		}
		else if (GM->TowerRunState == EVTGameState::Menu)
		{
			GM->PreviewClimberOnStage(GM->SelectedClimberId);
			GM->ApplyThemeVisuals(GM->GetThemeInfo(GM->SelectedThemeId));
		}
	}

	if (ModalBackdropBorder.IsValid())
	{
		ModalBackdropBorder->SetVisibility(ModalType == EVTModalOverlay::None ? EVisibility::Collapsed : EVisibility::Visible);
		ModalBackdropBorder->SetBorderBackgroundColor(bIs3DStageModal
			? FLinearColor(0.002f, 0.005f, 0.015f, 0.08f)
			: FLinearColor(0.005f, 0.010f, 0.030f, 0.90f));
		ModalBackdropBorder->SetPadding(bIs3DStageModal ? FMargin(8.0f, 4.0f) : FMargin(12.0f, 16.0f));
	}
	if (ModalOuterFrameBorder.IsValid())
	{
		ModalOuterFrameBorder->SetBorderBackgroundColor(bIs3DStageModal ? FLinearColor::Transparent : VTTheme::Cyan);
	}
	if (ModalInnerCardBorder.IsValid())
	{
		ModalInnerCardBorder->SetBorderBackgroundColor(bIs3DStageModal ? FLinearColor::Transparent : VTTheme::DarkBg);
		ModalInnerCardBorder->SetPadding(bIs3DStageModal ? FMargin(0.0f) : FMargin(12.0f, 10.0f));
	}
	if (ModalMaxHeightBox.IsValid())
	{
		ModalMaxHeightBox->SetMaxDesiredHeight(bIs3DStageModal ? 1150.0f : 670.0f);
	}
	RebuildModalContent();
}

void UVTMainHUDWidget::CloseModal()
{
	ActiveModal = EVTModalOverlay::None;
	if (AVTTowerGameMode* GM = GetTowerGM())
	{
		GM->bInCharacterSelectStage = false;
		GM->bInThemeSelectStage = false;
		GM->ApplyThemeVisuals(GM->GetThemeInfo(GM->SelectedThemeId));
		if (GM->TowerRunState == EVTGameState::Menu)
		{
			GM->PreviewClimberOnStage(GM->SelectedClimberId);
		}
	}
	if (ModalBackdropBorder.IsValid())
	{
		ModalBackdropBorder->SetVisibility(EVisibility::Collapsed);
	}
}

void UVTMainHUDWidget::PopulateLeaderboardRows(TSharedPtr<SVerticalBox> TargetBox, bool bFriendsOnly)
{
	if (!TargetBox.IsValid()) return;
	TargetBox->ClearChildren();

	AVTTowerGameMode* GM = GetTowerGM();
	if (!GM) return;

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
	const int32 RunScore = GM->CurrentScore;
	const int32 BestScore = FMath::Max(RunScore, GM->BestScore);
	const int32 ScoreForRank = (GM->TowerRunState == EVTGameState::GameOver) ? RunScore : BestScore;
	const int32 FloorForRank = (GM->TowerRunState == EVTGameState::GameOver)
		? GM->MaxFloor
		: (GM->ActiveSaveGame ? FMath::Max(GM->MaxFloor, GM->ActiveSaveGame->MaxFloorRecord) : GM->MaxFloor);
	const FString PlayerName = (GM->ActiveSaveGame && !GM->ActiveSaveGame->PlayerNickname.IsEmpty())
		? GM->ActiveSaveGame->PlayerNickname
		: TEXT("Du (Gast)");

	struct FVTLeaderboardEntry
	{
		FString Name;
		FString Tag;
		int32 Floor;
		int32 Score;
		bool bIsPlayer;
		bool bOnline;
	};

	TArray<FVTLeaderboardEntry> Entries;
	if (bFriendsOnly)
	{
		Entries = {
			{ TEXT("Laura_Apex"),      TEXT("FREUND // ONLINE"),  148, 84200, false, true  },
			{ TEXT("xX_NeonFelix_Xx"), TEXT("FREUND // ONLINE"),  125, 68500, false, true  },
			{ TEXT("Maxi_Climber"),    TEXT("FREUND // VOR 12m"),  92, 42800, false, false },
			{ TEXT("Sarah_Vibe"),      TEXT("FREUND // ONLINE"),   68, 28400, false, true  },
			{ TEXT("Kev_99"),          TEXT("FREUND // VOR 1h"),   45, 16900, false, false },
			{ TEXT("Tim_Turbo"),       TEXT("FREUND // ONLINE"),   28,  8200, false, true  },
			{ TEXT("Nina_Ice"),        TEXT("FREUND // VOR 3h"),   14,  3400, false, false },
			{ TEXT("Luk_Sky"),         TEXT("FREUND // GESTERN"),   6,   950, false, false }
		};
	}
	else
	{
		Entries = {
			{ TEXT("ApexGod_KR"),      TEXT("WELT-CHAMPION #1"),  260, 148900, false, true  },
			{ TEXT("TokyoDrift_JP"),   TEXT("TOP GLOBAL"),        215, 122400, false, true  },
			{ TEXT("CyberQueen_US"),   TEXT("TOP GLOBAL"),        178,  98500, false, true  },
			{ TEXT("Laura_Apex"),      TEXT("FREUND // GLOBAL"),  148,  84200, false, true  },
			{ TEXT("Vortex_DE"),       TEXT("TOP EUROPA"),        134,  76200, false, false },
			{ TEXT("xX_NeonFelix_Xx"), TEXT("FREUND // GLOBAL"),  125,  68500, false, true  },
			{ TEXT("Maxi_Climber"),    TEXT("FREUND // GLOBAL"),   92,  42800, false, false },
			{ TEXT("ZeroGravity_UK"),  TEXT("GLOBAL CLIMBER"),     54,  21500, false, true  },
			{ TEXT("PulseRunner_FR"),  TEXT("GLOBAL CLIMBER"),     24,   6800, false, false },
			{ TEXT("IceRookie_ES"),    TEXT("GLOBAL CLIMBER"),      8,   1200, false, true  }
		};
	}

	const FString PlayerTag = (GM->TowerRunState == EVTGameState::GameOver)
		? FString::Printf(TEXT("DEIN RUN (REKORD: %s)"), *FText::AsNumber(BestScore).ToString())
		: TEXT("DEIN AKTUELLER REKORD");

	Entries.Add({ FString::Printf(TEXT("DU (%s)"), *PlayerName), PlayerTag, FloorForRank, ScoreForRank, true, true });

	Entries.Sort([](const FVTLeaderboardEntry& A, const FVTLeaderboardEntry& B)
	{
		if (A.Score != B.Score) return A.Score > B.Score;
		return A.bIsPlayer;
	});

	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FVTLeaderboardEntry& E = Entries[i];
		const int32 RankNum = i + 1;
		const FLinearColor RowBorderCol = E.bIsPlayer
			? VTTheme::Emerald
			: (RankNum == 1 ? VTTheme::Gold : (RankNum <= 3 ? VTTheme::Cyan : FLinearColor(1.0f, 1.0f, 1.0f, 0.14f)));
		const FLinearColor RowBgCol = E.bIsPlayer
			? FLinearColor(0.015f, 0.16f, 0.11f, 0.98f)
			: FLinearColor(0.022f, 0.042f, 0.095f, 0.94f);

		TargetBox->AddSlot().AutoHeight().Padding(0.0f, 1.5f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(RowBorderCol)
			.Padding(FMargin(E.bIsPlayer ? 2.0f : 1.0f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(RowBgCol)
				.Padding(FMargin(8.0f, 4.0f))
				[
					SNew(SHorizontalBox)

					// Rank Badge (#1, #2, #3...)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(E.bIsPlayer ? VTTheme::Emerald : (RankNum <= 3 ? VTTheme::Gold : VTTheme::PillBg))
						.Padding(FMargin(6.0f, 2.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("#%d"), RankNum)))
							.Font(VTTheme::Font(8, true))
							.ColorAndOpacity((E.bIsPlayer || RankNum <= 3) ? FLinearColor(0.01f, 0.02f, 0.05f, 1.0f) : VTTheme::Cyan)
						]
					]

					// Player Name + Status Tag
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(E.Name))
							.Font(VTTheme::Font(8, true))
							.ColorAndOpacity(E.bIsPlayer ? VTTheme::Gold : FLinearColor::White)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%s // ETAGE %d"), *E.Tag, E.Floor)))
							.Font(VTTheme::Font(6, true))
							.ColorAndOpacity(E.bIsPlayer ? VTTheme::Cyan : (E.bOnline ? VTTheme::Emerald : VTTheme::MutedText))
						]
					]

					// Score
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%s PTS"), *FText::AsNumber(E.Score).ToString())))
						.Font(VTTheme::Font(9, true))
						.ColorAndOpacity(E.bIsPlayer ? VTTheme::Emerald : VTTheme::Gold)
					]
				]
			]
		];
	}
}

void UVTMainHUDWidget::RebuildGameOverLeaderboard()
{
	AVTTowerGameMode* GM = GetTowerGM();
	if (!GM) return;

	const int32 ScoreForRank = GM->CurrentScore;
	LastBuiltLeaderboardScore = ScoreForRank;
	bLastBuiltFriendsMode = bLeaderboardFriendsOnly;

	// Compute exact rank in Global & Friends lists for the summary banner
	const TArray<int32> FriendScores = { 84200, 68500, 42800, 28400, 16900, 8200, 3400, 950 };
	int32 FriendsRank = 1;
	for (int32 FS : FriendScores)
	{
		if (FS > ScoreForRank) FriendsRank++;
	}

	const TArray<int32> GlobalTopScores = { 148900, 122400, 98500, 84200, 76200, 68500, 42800, 21500, 6800, 1200 };
	int32 GlobalRank = 1;
	for (int32 GS : GlobalTopScores)
	{
		if (GS > ScoreForRank) GlobalRank++;
	}

	if (TxtOverPlacementSummary.IsValid())
	{
		TxtOverPlacementSummary->SetText(FText::FromString(FString::Printf(
			TEXT("PLATZIERUNG DIESER RUN: WELTWEIT #%d  //  FREUNDE #%d VON 9"),
			GlobalRank, FriendsRank)));
	}
	if (TabOverGlobalBorder.IsValid())
	{
		TabOverGlobalBorder->SetBorderBackgroundColor(!bLeaderboardFriendsOnly ? VTTheme::Cyan : VTTheme::CardBg);
	}
	if (TabOverFriendsBorder.IsValid())
	{
		TabOverFriendsBorder->SetBorderBackgroundColor(bLeaderboardFriendsOnly ? VTTheme::Emerald : VTTheme::CardBg);
	}

	PopulateLeaderboardRows(GameOverLeaderboardListBox, bLeaderboardFriendsOnly);
}

void UVTMainHUDWidget::StartSimulatedAd(TFunction<void()> OnCompleteCallback)
{
	PendingAdRewardCallback = OnCompleteCallback;
	AdCountdownRemaining = 3.0f;
	OpenModal(EVTModalOverlay::AdPlayer);
}

void UVTMainHUDWidget::RebuildModalContent()
{
	if (!ModalBodyBox.IsValid()) return;
	ModalBodyBox->ClearChildren();

	AVTTowerGameMode* GM = GetTowerGM();
	if (!GM || ActiveModal == EVTModalOverlay::None) return;

	LoadNanoBananaTextures();
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
	const FSlateBrush* ModeCardImg = BrushModeCard.IsValid() ? BrushModeCard.Get() : WhiteBrush;
	const FSlateBrush* CharSpotImg = BrushCharSpotlight.IsValid() ? BrushCharSpotlight.Get() : WhiteBrush;

	auto AddModalHeader = [this, GM, WhiteBrush](const FString& Tag, const FString& Title, const FLinearColor& TagCol)
	{
		ModalBodyBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(TagCol)
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 1.5f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.015f, 0.030f, 0.075f, 0.98f))
				.Padding(FMargin(10.0f, 8.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(FText::FromString(Tag)).Font(VTTheme::Font(7, true)).ColorAndOpacity(TagCol)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(FText::FromString(Title)).Font(VTTheme::Font(14, true)).ColorAndOpacity(FLinearColor::White)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						VTTheme::MakeButton(TEXT("X SCHLIESSEN"), VTTheme::CardBg, VTTheme::Pink, 8, FMargin(10.0f, 5.0f),
							FOnClicked::CreateLambda([this, GM]() { GM->PlayTowerSFX(TEXT("Click")); CloseModal(); return FReply::Handled(); }))
					]
				]
			]
		];
	};

	// =========================================================================
	// MODAL A: GUIDED STEP 1 OF 2 — GAME MODE SELECTION (NANO BANANA 2 CARDS)
	// =========================================================================
	if (ActiveModal == EVTModalOverlay::ModeSelect)
	{
		AddModalHeader(
			TEXT("SCHRITT 1 VON 2 // SPIELMODUS WAEHLEN"),
			TEXT("Waehle deine Herausforderung fuer den naechsten Turm-Aufstieg"),
			VTTheme::Cyan);

		auto AddGameModeCard = [this, GM, WhiteBrush, ModeCardImg](
			EVTGameModeType Mode,
			const FString& Title,
			const FString& BadgeText,
			const FString& GoalText,
			const FString& MechanicText,
			const FString& RewardText,
			const FLinearColor& AccentCol)
		{
			const bool bIsSelected = (GM->CurrentGameMode == Mode);
			const FLinearColor BorderCol = bIsSelected ? VTTheme::Emerald : AccentCol;
			const FLinearColor CardBgCol = bIsSelected ? FLinearColor(0.015f, 0.07f, 0.05f, 0.94f) : FLinearColor(0.012f, 0.024f, 0.055f, 0.90f);

			ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
			[
				SNew(SButton)
				.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
				.ContentPadding(FMargin(0.0f))
				.OnClicked_Lambda([this, GM, Mode]()
				{
					GM->CurrentGameMode = Mode;
					GM->SaveGameToDisk();
					GM->PlayTowerSFX(TEXT("ComboUp"), 1.15f, 1.0f);
					OpenModal(EVTModalOverlay::CharacterSelect);
					return FReply::Handled();
				})
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(BorderCol)
					.Padding(FMargin(bIsSelected ? 2.5f : 1.5f))
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SImage)
							.Image(ModeCardImg)
							.ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, bIsSelected ? 0.35f : 0.18f))
						]
						+ SOverlay::Slot()
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(CardBgCol)
							.Padding(FMargin(14.0f, 10.0f))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
								[
									SNew(SVerticalBox)
									// Header Row: Title + Mode Badge
									+ SVerticalBox::Slot().AutoHeight()
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
										[
											SNew(STextBlock)
											.Text(FText::FromString(Title))
											.Font(VTTheme::Font(12, true))
											.ColorAndOpacity(bIsSelected ? VTTheme::Emerald : FLinearColor::White)
										]
										+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
										[
											SNew(SBorder)
											.BorderImage(WhiteBrush)
											.BorderBackgroundColor(AccentCol)
											.Padding(FMargin(6.0f, 2.0f))
											[
												SNew(STextBlock)
												.Text(FText::FromString(BadgeText))
												.Font(VTTheme::Font(7, true))
												.ColorAndOpacity(FLinearColor(0.01f, 0.02f, 0.05f, 1.0f))
											]
										]
										+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
										[
											bIsSelected
												? SNew(SBorder)
													.BorderImage(WhiteBrush)
													.BorderBackgroundColor(VTTheme::Emerald)
													.Padding(FMargin(6.0f, 2.0f))
													[
														SNew(STextBlock)
														.Text(FText::FromString(TEXT("AKTIV GEWAEHLT")))
														.Font(VTTheme::Font(7, true))
														.ColorAndOpacity(FLinearColor(0.01f, 0.02f, 0.05f, 1.0f))
													]
												: SNullWidget::NullWidget
										]
									]
									// Section 1: ZIEL
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 1.0f)
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("ZIEL:")))
											.Font(VTTheme::Font(7, true))
											.ColorAndOpacity(VTTheme::Cyan)
										]
										+ SHorizontalBox::Slot().FillWidth(1.0f)
										[
											SNew(STextBlock)
											.Text(FText::FromString(GoalText))
											.Font(VTTheme::Font(7, false))
											.ColorAndOpacity(FLinearColor(0.92f, 0.94f, 0.98f, 1.0f))
										]
									]
									// Section 2: MECHANIK / REGELN
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 1.0f)
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("REGELN:")))
											.Font(VTTheme::Font(7, true))
											.ColorAndOpacity(VTTheme::Gold)
										]
										+ SHorizontalBox::Slot().FillWidth(1.0f)
										[
											SNew(STextBlock)
											.Text(FText::FromString(MechanicText))
											.Font(VTTheme::Font(7, false))
											.ColorAndOpacity(VTTheme::MutedText)
										]
									]
									// Section 3: BELOHNUNG / MULTIPLIKATOR
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("BONUS:")))
											.Font(VTTheme::Font(7, true))
											.ColorAndOpacity(AccentCol)
										]
										+ SHorizontalBox::Slot().FillWidth(1.0f)
										[
											SNew(STextBlock)
											.Text(FText::FromString(RewardText))
											.Font(VTTheme::Font(7, true))
											.ColorAndOpacity(AccentCol)
										]
									]
								]
								// Right Action Button
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(bIsSelected ? VTTheme::Emerald : AccentCol)
									.Padding(FMargin(12.0f, 10.0f))
									[
										SNew(STextBlock)
										.Text(FText::FromString(bIsSelected ? TEXT("STARTEN >>") : TEXT("WAEHLEN >>")))
										.Font(VTTheme::Font(9, true))
										.ColorAndOpacity(FLinearColor(0.01f, 0.02f, 0.05f, 1.0f))
									]
								]
							]
						]
					]
				]
			];
		};

		AddGameModeCard(
			EVTGameModeType::Arcade,
			TEXT("1. ARCADE CLASSIC (STANDARD)"),
			TEXT("EMPFOHLEN // 6 BIOME"),
			TEXT("Erklimme alle 6 Cyber-Eis-Biome bis zur Turmspitze."),
			TEXT("Dynamisch ansteigende Gefahrenzone, Hurry-Up-Wellen, alle Power-Ups & Quake-Combos bis 200x Godlike!"),
			TEXT("1.0x Punkte // 1.0x XP // Regulaere Highscore-Bestenliste"),
			VTTheme::Cyan);

		AddGameModeCard(
			EVTGameModeType::Overdrive2x,
			TEXT("2. OVERDRIVE 2x FEVER"),
			TEXT("2x PUNKTE // 2x XP // HARDCORE SPEED"),
			TEXT("Maximaler Highscore & ultraschnelles Account-Leveling."),
			TEXT("Turm bewegt sich +25% schneller! Permanente Fever-Aura mit doppelter Kletter-Dynamik & schmalerem Reaktionsfenster."),
			TEXT("Permanenter 2.0x Score- und 2.0x XP-Multiplikator fuer Rekorde!"),
			VTTheme::Pink);

		AddGameModeCard(
			EVTGameModeType::Blitz60s,
			TEXT("3. 60 SEKUNDEN BLITZ-ATTACK"),
			TEXT("60s TIME ATTACK // EXTREMER NERVENKITZEL"),
			TEXT("Erklimme so viele Etagen wie moeglich vor Ablauf der 60 Sekunden."),
			TEXT("Kein Kamera-Tod! Der HUD-Timer zaehlt unerbittlich von 60 auf 0. Jeder 10x-Combo-Streak schenkt +3 Sekunden Zeitbonus!"),
			TEXT("Exklusive Blitz-Bestenliste // Sofortige Core-Ausschuettung pro Etage"),
			VTTheme::Gold);

		AddGameModeCard(
			EVTGameModeType::Rival1v1,
			TEXT("4. 1v1 RIVALEN-CHALLENGE"),
			TEXT("VS GHOST-CHAMPION // ETAGE 125"),
			TEXT("Schlage den Rekord des legendaeren Turm-Rivalen APEX_NEXUS (Etage 125 // 68.500 Pts)."),
			TEXT("Ein holografischer Rivalen-Ghost klettert zeitgleich mit dir im Turm! Halte die Fuehrung bis zum Checkpoint."),
			TEXT("+150 Sofort-Cores und prestigetraechtiger 'Rival Hunter' Badge!"),
			VTTheme::Purple);

		AddGameModeCard(
			EVTGameModeType::Zen,
			TEXT("5. ZEN ENDLESS TRAINING"),
			TEXT("WARM-UP // KEIN ZEITDRUCK"),
			TEXT("Perfektioniere Wall-Bounces, Super-Ricochets und 200x Combo-Ketten ohne jeden Absturzdruck."),
			TEXT("Die Kamera bleibt entspannt stehen und wartet auf deinen naechsten Sprung. Keine Hurry-Up-Sirenen."),
			TEXT("0.5x XP // Perfekt zum Trainieren und Ausprobieren neuer Charaktere"),
			VTTheme::Emerald);
	}

	// =========================================================================
	// MODAL B: FAMILIAR 3D CHARACTER SELECTION (3D MODEL VIEWPORT + BLACK SILHOUETTE FOR LOCKED)
	// =========================================================================
	else if (ActiveModal == EVTModalOverlay::CharacterSelect)
	{
		const FVTClimberInfo Spot = GM->GetClimberInfo(GM->PreviewClimberId);
		const bool bSpotUnlocked = GM->IsClimberUnlocked(Spot.Id);
		const FLinearColor PitchBlack(0.0f, 0.0f, 0.0f, 1.0f);

		// Calculate visual stat bar percentages for the Spotlight Hero Card
		const float SpeedPct = bSpotUnlocked ? FMath::Clamp(0.55f + float(Spot.RosterNumber) * 0.022f, 0.55f, 1.0f) : 0.25f;
		const float JumpPct  = bSpotUnlocked ? FMath::Clamp(0.60f + float((Spot.RosterNumber * 7) % 10) * 0.04f, 0.60f, 1.0f) : 0.25f;
		const float BpmPct   = bSpotUnlocked ? FMath::Clamp(float(Spot.BaseBpm - 115) / 28.0f, 0.35f, 1.0f) : 0.25f;

		auto MakeStatBarRow = [WhiteBrush](const FString& StatLabel, float Pct, const FLinearColor& BarCol) -> TSharedRef<SWidget>
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(64.0f)
					[
						SNew(STextBlock).Text(FText::FromString(StatLabel)).Font(VTTheme::Font(6, true)).ColorAndOpacity(VTTheme::MutedText)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(FLinearColor(0.08f, 0.14f, 0.25f, 0.95f))
					.Padding(FMargin(1.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(FMath::RoundToFloat(Pct * 140.0f))
							.HeightOverride(4.5f)
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(BarCol)
							]
						]
					]
				];
		};

		// 1. TOP CARD: Header + Spotlight Info (Unlocked = Full Details, Locked = Black Silhouette Mystery)
		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(bSpotUnlocked ? VTTheme::Emerald : VTTheme::Gold)
			.Padding(FMargin(1.5f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.010f, 0.020f, 0.052f, 0.94f))
				.Padding(FMargin(10.0f, 7.0f))
				[
					SNew(SVerticalBox)

					// Compact Top Header Row
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("CHARAKTER-AUSWAHL (%d/20 FREI) // MODUS: %s"),
								GM->GetUnlockedClimberCount(), *VTTheme::GetGameModeShortName(GM->CurrentGameMode))))
							.Font(VTTheme::Font(8, true))
							.ColorAndOpacity(VTTheme::Cyan)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							VTTheme::MakeButton(TEXT("X SCHLIESSEN"), VTTheme::CardBg, VTTheme::Pink, 7, FMargin(8.0f, 3.0f),
								FOnClicked::CreateLambda([this, GM]() { GM->PlayTowerSFX(TEXT("Click")); CloseModal(); return FReply::Handled(); }))
						]
					]

					// Character Info Row (Colored Portrait vs. Black Silhouette)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)

						// Left Avatar Box: Full Color Figure if Unlocked, Pure Black Silhouette if Locked!
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(bSpotUnlocked ? Spot.RarityColor : FLinearColor(0.25f, 0.35f, 0.50f, 0.85f))
							.Padding(FMargin(1.5f))
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(bSpotUnlocked ? FLinearColor(0.05f, 0.12f, 0.24f, 1.0f) : FLinearColor(0.12f, 0.18f, 0.28f, 1.0f))
								.Padding(FMargin(10.0f, 6.0f))
								[
									SNew(SVerticalBox)
									// Head of character / silhouette
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
									[
										SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
										[
											SNew(SBorder)
											.BorderImage(WhiteBrush)
											.BorderBackgroundColor(bSpotUnlocked ? Spot.VisorColor : PitchBlack)
										]
									]
									// Body/Shoulders of character / silhouette
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 0.0f)
									[
										SNew(SBox).WidthOverride(28.0f).HeightOverride(18.0f)
										[
											SNew(SBorder)
											.BorderImage(WhiteBrush)
											.BorderBackgroundColor(bSpotUnlocked ? Spot.HoodieColor : PitchBlack)
											.HAlign(HAlign_Center).VAlign(VAlign_Center)
											[
												SNew(STextBlock)
												.Text(FText::FromString(bSpotUnlocked ? FString::Printf(TEXT("#%02d"), Spot.RosterNumber) : TEXT("?")))
												.Font(VTTheme::Font(7, true))
												.ColorAndOpacity(bSpotUnlocked ? FLinearColor(0.01f, 0.02f, 0.05f, 1.0f) : FLinearColor(0.35f, 0.45f, 0.60f, 0.9f))
											]
										]
									]
								]
							]
						]

						// Center Info & Stats
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(bSpotUnlocked
										? FString::Printf(TEXT("#%02d %s"), Spot.RosterNumber, *Spot.Name)
										: FString::Printf(TEXT("#%02d ??? [GESPERRTE SILHOUETTE]"), Spot.RosterNumber)))
									.Font(VTTheme::Font(11, true))
									.ColorAndOpacity(bSpotUnlocked ? FLinearColor::White : VTTheme::Gold)
								]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f)
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(bSpotUnlocked ? VTTheme::Emerald : PitchBlack)
									.Padding(FMargin(5.0f, 1.5f))
									[
										SNew(STextBlock)
										.Text(FText::FromString(bSpotUnlocked ? TEXT("3D-MODELL FREI") : FString::Printf(TEXT("LOCKED (%d CORES)"), Spot.CostCores)))
										.Font(VTTheme::Font(6, true))
										.ColorAndOpacity(bSpotUnlocked ? FLinearColor(0.01f, 0.02f, 0.05f, 1.0f) : VTTheme::Gold)
									]
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.5f, 0.0f, 3.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(bSpotUnlocked
									? Spot.Description
									: TEXT("Gesperrter Charakter: Du siehst nur seine schwarze 3D-Silhouette! Schalte ihn frei, um das 3D-Modell zu enthüllen.")))
								.Font(VTTheme::Font(6, false))
								.ColorAndOpacity(VTTheme::MutedText)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
							[
								MakeStatBarRow(TEXT("SPEED"), SpeedPct, bSpotUnlocked ? VTTheme::Cyan : FLinearColor(0.2f, 0.25f, 0.35f, 1.0f))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
							[
								MakeStatBarRow(TEXT("JUMP"), JumpPct, bSpotUnlocked ? VTTheme::Gold : FLinearColor(0.2f, 0.25f, 0.35f, 1.0f))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
							[
								MakeStatBarRow(TEXT("SYNTH BPM"), BpmPct, bSpotUnlocked ? VTTheme::Pink : FLinearColor(0.2f, 0.25f, 0.35f, 1.0f))
							]
						]
					]
				]
			]
		];

		// 2. MIDDLE OPEN 3D CHARACTER SHOWCASE WINDOW (Large 330px unobstructed 3D Stage Window!)
		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
		[
			SNew(SBox)
			.HeightOverride(330.0f)
			[
				SNew(SOverlay)

				// Left / Right 3D Stage Rotation & Character Prev/Next Arrows
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						VTTheme::MakeButton(
							TEXT("<< VORHERIGER"),
							FLinearColor(0.01f, 0.03f, 0.08f, 0.85f), VTTheme::Cyan, 8, FMargin(8.0f, 12.0f),
							FOnClicked::CreateLambda([this, GM]()
							{
								GM->PlayTowerSFX(TEXT("Click"), 0.95f, 0.9f);
								GM->SelectClimberDirection(-1);
								RebuildModalContent();
								return FReply::Handled();
							}))
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(SSpacer)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						VTTheme::MakeButton(
							TEXT("NAECHSTER >>"),
							FLinearColor(0.01f, 0.03f, 0.08f, 0.85f), VTTheme::Cyan, 8, FMargin(8.0f, 12.0f),
							FOnClicked::CreateLambda([this, GM]()
							{
								GM->PlayTowerSFX(TEXT("Click"), 1.08f, 0.9f);
								GM->SelectClimberDirection(1);
								RebuildModalContent();
								return FReply::Handled();
							}))
					]
				]

				// Bottom Badge inside the 3D Stage Window (Below Pedestal)
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Bottom)
				.Padding(0.0f, 0.0f, 0.0f, 2.0f)
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(bSpotUnlocked ? VTTheme::Emerald : PitchBlack)
					.Padding(FMargin(1.5f))
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(0.01f, 0.02f, 0.06f, 0.92f))
						.Padding(FMargin(10.0f, 2.5f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(bSpotUnlocked
								? FString::Printf(TEXT("3D-SHOWROOM: #%02d %s (DETAILLIERTES 3D-MODELL FREIGESCHALTET)"), Spot.RosterNumber, *Spot.Name)
								: FString::Printf(TEXT("3D-SHOWROOM: SCHWARZE 3D-SILHOUETTE (GESPERRT — %d CORES)"), Spot.CostCores)))
							.Font(VTTheme::Font(7, true))
							.ColorAndOpacity(bSpotUnlocked ? VTTheme::Emerald : VTTheme::Gold)
						]
					]
				]
			]
		];

		// 3. BOTTOM ROSTER DOCK: Classic 4x5 Grid (Unlocked = Colored Figure, Locked = Pure Black Silhouette!) + Launch Bar
		TSharedRef<SVerticalBox> RosterDockBox = SNew(SVerticalBox);

		for (int32 Row = 0; Row < 4; ++Row)
		{
			TSharedRef<SHorizontalBox> GridRow = SNew(SHorizontalBox);
			for (int32 Col = 0; Col < 5; ++Col)
			{
				const int32 Idx = Row * 5 + Col;
				if (!GM->Climbers.IsValidIndex(Idx)) continue;

				const FVTClimberInfo C = GM->Climbers[Idx];
				const bool bUnlocked = GM->IsClimberUnlocked(C.Id);
				const bool bSelected = (GM->PreviewClimberId == C.Id);

				GridRow->AddSlot()
				.FillWidth(1.0f)
				.Padding(1.5f)
				[
					SNew(SButton)
					.IsFocusable(false)
					.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
					.ContentPadding(FMargin(0.0f))
					.OnClicked_Lambda([this, GM, C]()
					{
						GM->PlayTowerSFX(TEXT("Click"), 1.0f + float(C.RosterNumber) * 0.02f, 0.9f);
						GM->PreviewClimberOnStage(C.Id);
						RebuildModalContent();
						return FReply::Handled();
					})
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(bSelected ? FLinearColor::White : (bUnlocked ? VTTheme::Emerald : FLinearColor(0.20f, 0.28f, 0.42f, 0.75f)))
						.Padding(FMargin(bSelected ? 1.5f : 1.0f))
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(bSelected
								? FLinearColor(0.05f, 0.15f, 0.30f, 0.98f)
								: (bUnlocked ? FLinearColor(0.022f, 0.045f, 0.105f, 0.96f) : FLinearColor(0.08f, 0.11f, 0.18f, 0.95f)))
							.Padding(FMargin(2.0f, 2.5f))
							[
								SNew(SVerticalBox)

								// Character Avatar Icon: Colored 3D-style figure if Unlocked, PURE BLACK SILHOUETTE if Locked!
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
								[
									SNew(SVerticalBox)
									// Head
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
									[
										SNew(SBox).WidthOverride(9.0f).HeightOverride(7.0f)
										[
											SNew(SBorder)
											.BorderImage(WhiteBrush)
											.BorderBackgroundColor(bUnlocked ? C.VisorColor : PitchBlack)
										]
									]
									// Torso / Shoulders
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 1.0f, 0.0f, 0.0f)
									[
										SNew(SBox).WidthOverride(20.0f).HeightOverride(10.0f)
										[
											SNew(SBorder)
											.BorderImage(WhiteBrush)
											.BorderBackgroundColor(bUnlocked ? C.HoodieColor : PitchBlack)
											.HAlign(HAlign_Center).VAlign(VAlign_Center)
											[
												SNew(STextBlock)
												.Text(FText::FromString(bUnlocked ? FString::Printf(TEXT("#%02d"), C.RosterNumber) : TEXT("?")))
												.Font(VTTheme::Font(6, true))
												.ColorAndOpacity(bUnlocked ? FLinearColor(0.01f, 0.02f, 0.05f, 1.0f) : FLinearColor(0.32f, 0.40f, 0.52f, 0.85f))
											]
										]
									]
								]

								// Name (Full Name if Unlocked, "???" Silhouette if Locked)
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 1.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(bUnlocked ? C.Name : FString::Printf(TEXT("#%02d ???"), C.RosterNumber)))
									.Font(VTTheme::Font(6, true))
									.ColorAndOpacity(bUnlocked ? FLinearColor::White : FLinearColor(0.45f, 0.52f, 0.64f, 1.0f))
								]

								// Status / Cost
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(bUnlocked ? TEXT("[FREI]") : FString::Printf(TEXT("[%d C]"), C.CostCores)))
									.Font(VTTheme::Font(6, true))
									.ColorAndOpacity(bUnlocked ? VTTheme::Emerald : VTTheme::Gold)
								]
							]
						]
					]
				];
			}

			RosterDockBox->AddSlot().AutoHeight().Padding(0.0f, 0.5f)
			[
				GridRow
			];
		}

		// Launch / Unlock Action Row inside Bottom Dock
		RosterDockBox->AddSlot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				VTTheme::MakeButton(
					TEXT("<< MODUS"),
					VTTheme::CardBg, VTTheme::Cyan, 8, FMargin(8.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]()
					{
						GM->PlayTowerSFX(TEXT("Click"));
						OpenModal(EVTModalOverlay::ModeSelect);
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 5.0f, 0.0f)
			[
				VTTheme::MakeButton(
					TEXT("THEMES >>"),
					VTTheme::CardBg, VTTheme::Gold, 8, FMargin(8.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]()
					{
						GM->PlayTowerSFX(TEXT("Click"));
						OpenModal(EVTModalOverlay::ThemeSelect);
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				VTTheme::MakeButton(
					bSpotUnlocked
						? FString::Printf(TEXT("MIT %s STARTEN! // CLIMB NOW >>"), *Spot.Name)
						: FString::Printf(TEXT("SILHOUETTE FREISCHALTEN (%d CORES) & STARTEN!"), Spot.CostCores),
					bSpotUnlocked ? VTTheme::Emerald : VTTheme::Gold,
					FLinearColor::White,
					9, FMargin(10.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM, Spot, bSpotUnlocked]()
					{
						if (bSpotUnlocked)
						{
							GM->BuyOrEquipClimber(Spot.Id);
							GM->PlayTowerSFX(TEXT("SuperLaunch"), 1.05f, 1.0f);
							CloseModal();
							GM->StartNewRun();
						}
						else
						{
							if (GM->BuyOrEquipClimber(Spot.Id))
							{
								GM->PlayTowerSFX(TEXT("LevelUp"), 1.0f, 1.0f);
								CloseModal();
								GM->StartNewRun();
							}
							else
							{
								GM->PlayTowerSFX(TEXT("Click"), 0.8f, 1.0f);
								RebuildModalContent();
							}
						}
						return FReply::Handled();
					}))
			]
		];

		ModalBodyBox->AddSlot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(VTTheme::Cyan)
			.Padding(FMargin(1.5f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.010f, 0.020f, 0.052f, 0.95f))
				.Padding(FMargin(8.0f, 6.0f))
				[
					RosterDockBox
				]
			]
		];
	}

	// =========================================================================
	// MODAL B2: 3D THEME SELECTION (REAL-TIME 3D SHOWROOM & LEVEL THEME STAGE)
	// =========================================================================
	else if (ActiveModal == EVTModalOverlay::ThemeSelect)
	{
		const FVTGameThemeInfo Spot = GM->GetThemeInfo(GM->PreviewThemeId);
		const bool bSpotUnlocked = GM->IsThemeUnlocked(Spot.Id);
		const bool bSpotEquipped = (Spot.Id == GM->SelectedThemeId);

		auto MakeColorChip = [WhiteBrush](const FString& ChipLabel, const FLinearColor& ChipCol) -> TSharedRef<SWidget>
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 3.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(12.0f).HeightOverride(10.0f)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(ChipCol)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(ChipLabel)).Font(VTTheme::Font(6, true)).ColorAndOpacity(VTTheme::MutedText)
				];
		};

		// 1. TOP CARD: Header + Spotlight Info
		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(bSpotEquipped ? VTTheme::Cyan : (bSpotUnlocked ? VTTheme::Emerald : VTTheme::Gold))
			.Padding(FMargin(1.5f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.010f, 0.020f, 0.052f, 0.94f))
				.Padding(FMargin(10.0f, 7.0f))
				[
					SNew(SVerticalBox)

					// Compact Top Header Row
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("TURM-THEMES (%d/8 FREI) // AKTIV: %s"),
								GM->GetUnlockedThemeCount(), *GM->GetThemeInfo(GM->SelectedThemeId).Name)))
							.Font(VTTheme::Font(8, true))
							.ColorAndOpacity(VTTheme::Gold)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							VTTheme::MakeButton(TEXT("X SCHLIESSEN"), VTTheme::CardBg, VTTheme::Pink, 7, FMargin(8.0f, 3.0f),
								FOnClicked::CreateLambda([this, GM]() { GM->PlayTowerSFX(TEXT("Click")); CloseModal(); return FReply::Handled(); }))
						]
					]

					// Theme Info Row
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)

						// Left Theme Swatch Box (layered mini tower block)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(bSpotUnlocked ? Spot.PrimaryColor : FLinearColor(0.25f, 0.35f, 0.50f, 0.85f))
							.Padding(FMargin(1.5f))
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(FLinearColor(0.05f, 0.08f, 0.15f, 1.0f))
								.Padding(FMargin(6.0f, 5.0f))
								[
									SNew(SVerticalBox)
									// Top Wall Emissive Stripe
									+ SVerticalBox::Slot().AutoHeight()
									[
										SNew(SBox).WidthOverride(38.0f).HeightOverride(6.0f)
										[
											SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(Spot.WallEmissiveColor)
										]
									]
									// Center Wall Block with Theme Index
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.5f)
									[
										SNew(SBox).WidthOverride(38.0f).HeightOverride(22.0f)
										[
											SNew(SBorder)
											.BorderImage(WhiteBrush)
											.BorderBackgroundColor(Spot.WallBaseColor)
											.HAlign(HAlign_Center).VAlign(VAlign_Center)
											[
												SNew(STextBlock)
												.Text(FText::FromString(FString::Printf(TEXT("#%02d"), Spot.ThemeNumber)))
												.Font(VTTheme::Font(8, true))
												.ColorAndOpacity(Spot.PrimaryColor)
											]
										]
									]
									// Platform Glow Stripe
									+ SVerticalBox::Slot().AutoHeight()
									[
										SNew(SBox).WidthOverride(38.0f).HeightOverride(5.0f)
										[
											SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(Spot.PrimaryColor)
										]
									]
								]
							]
						]

						// Center Details & Color Palette
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString::Printf(TEXT("#%02d %s"), Spot.ThemeNumber, *Spot.Name)))
									.Font(VTTheme::Font(11, true))
									.ColorAndOpacity(FLinearColor::White)
								]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f)
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(bSpotEquipped ? VTTheme::Cyan : (bSpotUnlocked ? VTTheme::Emerald : FLinearColor::Black))
									.Padding(FMargin(5.0f, 1.5f))
									[
										SNew(STextBlock)
										.Text(FText::FromString(bSpotEquipped
											? TEXT("AKTIV AUSGERUESTET")
											: (bSpotUnlocked ? TEXT("FREIGESCHALTET") : FString::Printf(TEXT("LOCKED (%d CORES)"), Spot.CostCores))))
										.Font(VTTheme::Font(6, true))
										.ColorAndOpacity(bSpotEquipped ? FLinearColor::White : (bSpotUnlocked ? FLinearColor(0.01f, 0.02f, 0.05f, 1.0f) : VTTheme::Gold))
									]
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 2.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(Spot.Subtitle))
								.Font(VTTheme::Font(7, true))
								.ColorAndOpacity(Spot.PrimaryColor)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 4.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(Spot.Description))
								.Font(VTTheme::Font(6, false))
								.ColorAndOpacity(VTTheme::MutedText)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[
									MakeColorChip(TEXT("WAND"), Spot.WallBaseColor)
								]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									MakeColorChip(TEXT("PLATTFORM"), Spot.PrimaryColor)
								]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									MakeColorChip(TEXT("EMISSIVE"), Spot.WallEmissiveColor)
								]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									MakeColorChip(TEXT("AKZENT"), Spot.SecondaryColor)
								]
							]
						]
					]
				]
			]
		];

		// 2. MIDDLE OPEN 3D STAGE WINDOW (Large 330px unobstructed 3D Tower Viewport)
		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
		[
			SNew(SBox)
			.HeightOverride(330.0f)
			[
				SNew(SOverlay)

				// Left / Right Prev / Next Theme Buttons
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						VTTheme::MakeButton(
							TEXT("<< VORHERIGES THEME"),
							FLinearColor(0.01f, 0.03f, 0.08f, 0.85f), VTTheme::Gold, 8, FMargin(8.0f, 12.0f),
							FOnClicked::CreateLambda([this, GM]()
							{
								GM->PlayTowerSFX(TEXT("Click"), 0.95f, 0.9f);
								GM->SelectThemeDirection(-1);
								RebuildModalContent();
								return FReply::Handled();
							}))
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(SSpacer)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						VTTheme::MakeButton(
							TEXT("NAECHSTES THEME >>"),
							FLinearColor(0.01f, 0.03f, 0.08f, 0.85f), VTTheme::Gold, 8, FMargin(8.0f, 12.0f),
							FOnClicked::CreateLambda([this, GM]()
							{
								GM->PlayTowerSFX(TEXT("Click"), 1.08f, 0.9f);
								GM->SelectThemeDirection(1);
								RebuildModalContent();
								return FReply::Handled();
							}))
					]
				]

				// Bottom Badge inside 3D Window
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Bottom)
				.Padding(0.0f, 0.0f, 0.0f, 2.0f)
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(bSpotEquipped ? VTTheme::Cyan : (bSpotUnlocked ? VTTheme::Emerald : VTTheme::Gold))
					.Padding(FMargin(1.5f))
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(0.01f, 0.02f, 0.06f, 0.92f))
						.Padding(FMargin(10.0f, 2.5f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("3D-TURMVORSCHAU: #%02d %s (%s)"),
								Spot.ThemeNumber, *Spot.Name,
								bSpotEquipped ? TEXT("AKTIV AUSGERUESTET") : (bSpotUnlocked ? TEXT("FREIGESCHALTET") : TEXT("VORSCHAU-MODUS")))))
							.Font(VTTheme::Font(7, true))
							.ColorAndOpacity(bSpotEquipped ? VTTheme::Cyan : (bSpotUnlocked ? VTTheme::Emerald : VTTheme::Gold))
						]
					]
				]
			]
		];

		// 3. BOTTOM THEMES DOCK: 2x4 Grid + Action Bar
		TSharedRef<SVerticalBox> ThemeDockBox = SNew(SVerticalBox);

		for (int32 Row = 0; Row < 2; ++Row)
		{
			TSharedRef<SHorizontalBox> GridRow = SNew(SHorizontalBox);
			for (int32 Col = 0; Col < 4; ++Col)
			{
				const int32 Idx = Row * 4 + Col;
				if (!GM->Themes.IsValidIndex(Idx)) continue;

				const FVTGameThemeInfo T = GM->Themes[Idx];
				const bool bUnlocked = GM->IsThemeUnlocked(T.Id);
				const bool bEquipped = (T.Id == GM->SelectedThemeId);
				const bool bSelected = (GM->PreviewThemeId == T.Id);

				GridRow->AddSlot()
				.FillWidth(1.0f)
				.Padding(2.0f)
				[
					SNew(SButton)
					.IsFocusable(false)
					.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
					.ContentPadding(FMargin(0.0f))
					.OnClicked_Lambda([this, GM, T]()
					{
						GM->PlayTowerSFX(TEXT("Click"), 1.0f + float(T.ThemeNumber) * 0.03f, 0.9f);
						GM->PreviewThemeOnStage(T.Id);
						RebuildModalContent();
						return FReply::Handled();
					})
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(bSelected ? FLinearColor::White : (bEquipped ? VTTheme::Cyan : (bUnlocked ? VTTheme::Emerald : FLinearColor(0.20f, 0.28f, 0.42f, 0.75f))))
						.Padding(FMargin(bSelected ? 2.0f : 1.0f))
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(bSelected
								? FLinearColor(0.06f, 0.16f, 0.32f, 0.98f)
								: (bUnlocked ? FLinearColor(0.022f, 0.045f, 0.105f, 0.96f) : FLinearColor(0.08f, 0.11f, 0.18f, 0.95f)))
							.Padding(FMargin(4.0f, 5.0f))
							[
								SNew(SVerticalBox)

								// 4-Color Palette Bar
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 3.0f)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().FillWidth(1.0f)
									[
										SNew(SBox).HeightOverride(4.0f)
										[
											SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(T.WallBaseColor)
										]
									]
									+ SHorizontalBox::Slot().FillWidth(1.0f)
									[
										SNew(SBox).HeightOverride(4.0f)
										[
											SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(T.PrimaryColor)
										]
									]
									+ SHorizontalBox::Slot().FillWidth(1.0f)
									[
										SNew(SBox).HeightOverride(4.0f)
										[
											SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(T.WallEmissiveColor)
										]
									]
									+ SHorizontalBox::Slot().FillWidth(1.0f)
									[
										SNew(SBox).HeightOverride(4.0f)
										[
											SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(T.SecondaryColor)
										]
									]
								]

								// Theme Title
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 1.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString::Printf(TEXT("#%02d %s"), T.ThemeNumber, *T.Name)))
									.Font(VTTheme::Font(7, true))
									.ColorAndOpacity(bSelected ? FLinearColor::White : (bUnlocked ? VTTheme::Cyan : FLinearColor(0.55f, 0.62f, 0.72f, 1.0f)))
								]

								// Theme Subtitle
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.5f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(T.Subtitle))
									.Font(VTTheme::Font(5, false))
									.ColorAndOpacity(VTTheme::MutedText)
								]

								// Status Badge
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(bEquipped ? TEXT("[AKTIV]") : (bUnlocked ? TEXT("[FREI]") : FString::Printf(TEXT("[%d C]"), T.CostCores))))
									.Font(VTTheme::Font(6, true))
									.ColorAndOpacity(bEquipped ? VTTheme::Cyan : (bUnlocked ? VTTheme::Emerald : VTTheme::Gold))
								]
							]
						]
					]
				];
			}

			ThemeDockBox->AddSlot().AutoHeight().Padding(0.0f, 1.0f)
			[
				GridRow
			];
		}

		// Action Bar inside Theme Dock
		ThemeDockBox->AddSlot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 5.0f, 0.0f)
			[
				VTTheme::MakeButton(
					TEXT("<< CHARAKTERE"),
					VTTheme::CardBg, VTTheme::Cyan, 8, FMargin(8.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]()
					{
						GM->PlayTowerSFX(TEXT("Click"));
						OpenModal(EVTModalOverlay::CharacterSelect);
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				VTTheme::MakeButton(
					bSpotEquipped
						? FString::Printf(TEXT("THEME '%s' IST AKTIV // JETZT KLETTERN >>"), *Spot.Name)
						: (bSpotUnlocked
							? FString::Printf(TEXT("THEME '%s' AUSRUESTEN & KLETTERN >>"), *Spot.Name)
							: FString::Printf(TEXT("THEME '%s' FREISCHALTEN (%d CORES) & AUSRUESTEN!"), *Spot.Name, Spot.CostCores)),
					bSpotEquipped ? VTTheme::Emerald : (bSpotUnlocked ? VTTheme::Cyan : VTTheme::Gold),
					FLinearColor::White,
					9, FMargin(10.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM, Spot, bSpotUnlocked, bSpotEquipped]()
					{
						if (bSpotEquipped)
						{
							GM->PlayTowerSFX(TEXT("SuperLaunch"), 1.05f, 1.0f);
							CloseModal();
							GM->StartNewRun();
						}
						else if (bSpotUnlocked)
						{
							GM->BuyOrEquipTheme(Spot.Id);
							GM->PlayTowerSFX(TEXT("SuperLaunch"), 1.05f, 1.0f);
							CloseModal();
							GM->StartNewRun();
						}
						else
						{
							if (GM->BuyOrEquipTheme(Spot.Id))
							{
								GM->PlayTowerSFX(TEXT("LevelUp"), 1.0f, 1.0f);
								RebuildModalContent();
							}
							else
							{
								GM->PlayTowerSFX(TEXT("Click"), 0.8f, 1.0f);
								RebuildModalContent();
							}
						}
						return FReply::Handled();
					}))
			]
		];

		ModalBodyBox->AddSlot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(VTTheme::Gold)
			.Padding(FMargin(1.5f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(0.010f, 0.020f, 0.052f, 0.95f))
				.Padding(FMargin(8.0f, 6.0f))
				[
					ThemeDockBox
				]
			]
		];
	}

	// =========================================================================
	// MODAL C: CONSOLIDATED SETTINGS MENU (AUDIO, HUD OVERLAY, 6 CONTROLS, ACCOUNT)
	// =========================================================================
	else if (ActiveModal == EVTModalOverlay::SettingsAndControls || ActiveModal == EVTModalOverlay::ControlPreview)
	{
		AddModalHeader(
			TEXT("EINSTELLUNGEN // AUDIO, HANDY-STEUERUNG, HUD-OVERLAY & KONTO"),
			TEXT("SPIEL-EINSTELLUNGEN & STEUERUNGS-CENTER"),
			VTTheme::Cyan);

		const bool bAudioMuted = GM->ActiveSaveGame ? GM->ActiveSaveGame->bAudioMuted : false;
		const bool bVoiceOn = GM->ActiveSaveGame ? GM->ActiveSaveGame->bVoiceEnabled : true;

		// 1. AUDIO & SYNTHWAVE SOUND ENGINE SECTION
		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(VTTheme::Emerald)
			.Padding(FMargin(1.5f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(VTTheme::CardBg)
				.Padding(FMargin(12.0f, 8.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("1. SYNTHWAVE MUSIK & DOPAMIN SOUND-ENGINE (44.1 kHz STEREO):")))
						.Font(VTTheme::Font(9, true))
						.ColorAndOpacity(VTTheme::Emerald)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f)
						[
							VTTheme::MakeButton(
								bAudioMuted ? TEXT("MUSIK & SOUND: STUMM [EINSCHALTEN]") : TEXT("MUSIK & SOUND: AKTIV (AN)"),
								bAudioMuted ? VTTheme::Pink : VTTheme::Emerald,
								FLinearColor::White, 8, FMargin(8.0f, 7.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									GM->ToggleAudioMute();
									RebuildModalContent();
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f)
						[
							VTTheme::MakeButton(
								bVoiceOn ? TEXT("ANNOUNCER CALLOUTS: AN") : TEXT("ANNOUNCER CALLOUTS: AUS"),
								bVoiceOn ? VTTheme::Cyan : VTTheme::PillBg,
								FLinearColor::White, 8, FMargin(8.0f, 7.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									GM->ToggleVoiceEnabled();
									RebuildModalContent();
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f, 0.0f)
						[
							VTTheme::MakeButton(
								TEXT("SOUND-TEST (FANFARE)"),
								VTTheme::Gold, FLinearColor::White, 8, FMargin(10.0f, 7.0f),
								FOnClicked::CreateLambda([GM]()
								{
									GM->PlayTowerSFX(TEXT("LevelUp"), 1.0f, 1.0f);
									return FReply::Handled();
								}))
						]
					]
				]
			]
		];

		// 2. TOUCH-BUTTON POSITIONIERUNG, EINSTELLUNGEN & SPIELTEMPO
		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(VTTheme::Gold)
			.Padding(FMargin(1.5f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(VTTheme::CardBg)
				.Padding(FMargin(12.0f, 8.0f))
				[
					SNew(SVerticalBox)

					// Section Title
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("2. TOUCH-BUTTONS FREI POSITIONIEREN & ANPASSEN:")))
						.Font(VTTheme::Font(9, true))
						.ColorAndOpacity(VTTheme::Gold)
					]

					// Prominent Live Drag & Drop Button
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						VTTheme::MakeButton(
							TEXT("🖐 LIVE MIT DEM DAUMEN FREI VERSCHIEBEN (DRAG & DROP)"),
							VTTheme::Gold, FLinearColor::White, 9, FMargin(10.0f, 8.0f),
							FOnClicked::CreateLambda([this, GM]()
							{
								GM->PlayTowerSFX(TEXT("Click"));
								bHudDragEditMode = true;
								CloseModal();
								return FReply::Handled();
							}))
					]

					// POSITION CONTROLS FOR BUTTON LINKS
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("BUTTON LINKS (◀) // X=%.0f px, Y=%.0f px:"),
							GM->ActiveSaveGame ? GM->ActiveSaveGame->HudConfig.LeftClusterOffsetX : 24.0f,
							GM->ActiveSaveGame ? GM->ActiveSaveGame->HudConfig.LeftClusterOffsetY : 24.0f)))
						.Font(VTTheme::Font(8, true))
						.ColorAndOpacity(VTTheme::Cyan)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("◀ -15"), VTTheme::PillBg, VTTheme::Cyan, 7, FMargin(4.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.LeftClusterOffsetX = FMath::Clamp(GM->ActiveSaveGame->HudConfig.LeftClusterOffsetX - 15.0f, 4.0f, 420.0f);
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("+15 ▶"), VTTheme::PillBg, VTTheme::Cyan, 7, FMargin(4.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.LeftClusterOffsetX = FMath::Clamp(GM->ActiveSaveGame->HudConfig.LeftClusterOffsetX + 15.0f, 4.0f, 420.0f);
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("▲ +15"), VTTheme::PillBg, VTTheme::Cyan, 7, FMargin(4.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.LeftClusterOffsetY = FMath::Clamp(GM->ActiveSaveGame->HudConfig.LeftClusterOffsetY + 15.0f, 4.0f, 420.0f);
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("-15 ▼"), VTTheme::PillBg, VTTheme::Cyan, 7, FMargin(4.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.LeftClusterOffsetY = FMath::Clamp(GM->ActiveSaveGame->HudConfig.LeftClusterOffsetY - 15.0f, 4.0f, 420.0f);
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
					]

					// POSITION CONTROLS FOR BUTTON RECHTS
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("BUTTON RECHTS (▶) // X=%.0f px, Y=%.0f px:"),
							GM->ActiveSaveGame ? GM->ActiveSaveGame->HudConfig.RightSteerOffsetX : 108.0f,
							GM->ActiveSaveGame ? GM->ActiveSaveGame->HudConfig.RightSteerOffsetY : 24.0f)))
						.Font(VTTheme::Font(8, true))
						.ColorAndOpacity(VTTheme::Cyan)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("◀ -15"), VTTheme::PillBg, VTTheme::Cyan, 7, FMargin(4.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.RightSteerOffsetX = FMath::Clamp(GM->ActiveSaveGame->HudConfig.RightSteerOffsetX - 15.0f, 4.0f, 420.0f);
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("+15 ▶"), VTTheme::PillBg, VTTheme::Cyan, 7, FMargin(4.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.RightSteerOffsetX = FMath::Clamp(GM->ActiveSaveGame->HudConfig.RightSteerOffsetX + 15.0f, 4.0f, 420.0f);
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("▲ +15"), VTTheme::PillBg, VTTheme::Cyan, 7, FMargin(4.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.RightSteerOffsetY = FMath::Clamp(GM->ActiveSaveGame->HudConfig.RightSteerOffsetY + 15.0f, 4.0f, 420.0f);
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("-15 ▼"), VTTheme::PillBg, VTTheme::Cyan, 7, FMargin(4.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.RightSteerOffsetY = FMath::Clamp(GM->ActiveSaveGame->HudConfig.RightSteerOffsetY - 15.0f, 4.0f, 420.0f);
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
					]

					// POSITION CONTROLS FOR JUMP BUTTON
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("JUMP-BUTTON // X=%.0f px, Y=%.0f px:"),
							GM->ActiveSaveGame ? GM->ActiveSaveGame->HudConfig.RightClusterOffsetX : 26.0f,
							GM->ActiveSaveGame ? GM->ActiveSaveGame->HudConfig.RightClusterOffsetY : 28.0f)))
						.Font(VTTheme::Font(8, true))
						.ColorAndOpacity(VTTheme::Pink)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("◀ INNEN (+15)"), VTTheme::PillBg, VTTheme::Pink, 7, FMargin(4.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.RightClusterOffsetX = FMath::Clamp(GM->ActiveSaveGame->HudConfig.RightClusterOffsetX + 15.0f, 4.0f, 420.0f);
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("AUSSEN (-15) ▶"), VTTheme::PillBg, VTTheme::Pink, 7, FMargin(4.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.RightClusterOffsetX = FMath::Clamp(GM->ActiveSaveGame->HudConfig.RightClusterOffsetX - 15.0f, 4.0f, 420.0f);
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("▲ HÖHER (+15)"), VTTheme::PillBg, VTTheme::Pink, 7, FMargin(4.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.RightClusterOffsetY = FMath::Clamp(GM->ActiveSaveGame->HudConfig.RightClusterOffsetY + 15.0f, 4.0f, 400.0f);
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("TIEFER (-15) ▼"), VTTheme::PillBg, VTTheme::Pink, 7, FMargin(4.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.RightClusterOffsetY = FMath::Clamp(GM->ActiveSaveGame->HudConfig.RightClusterOffsetY - 15.0f, 4.0f, 400.0f);
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
					]

					// BUTTON SIZE CONTROLS
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("TASTEN-GROESSE (%d%%):"),
							GM->ActiveSaveGame ? GM->ActiveSaveGame->HudConfig.SizePct : 100)))
						.Font(VTTheme::Font(8, true))
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("80% KLEIN"), (GM->ActiveSaveGame && GM->ActiveSaveGame->HudConfig.SizePct <= 85) ? VTTheme::Cyan : VTTheme::PillBg, FLinearColor::White, 7, FMargin(3.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]() { if (GM->ActiveSaveGame) { GM->ActiveSaveGame->HudConfig.SizePct = 80; GM->SaveGameToDisk(); RebuildModalContent(); } return FReply::Handled(); }))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("100% NORMAL"), (GM->ActiveSaveGame && GM->ActiveSaveGame->HudConfig.SizePct > 85 && GM->ActiveSaveGame->HudConfig.SizePct <= 110) ? VTTheme::Cyan : VTTheme::PillBg, FLinearColor::White, 7, FMargin(3.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]() { if (GM->ActiveSaveGame) { GM->ActiveSaveGame->HudConfig.SizePct = 100; GM->SaveGameToDisk(); RebuildModalContent(); } return FReply::Handled(); }))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("125% GROSS"), (GM->ActiveSaveGame && GM->ActiveSaveGame->HudConfig.SizePct > 110 && GM->ActiveSaveGame->HudConfig.SizePct <= 130) ? VTTheme::Cyan : VTTheme::PillBg, FLinearColor::White, 7, FMargin(3.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]() { if (GM->ActiveSaveGame) { GM->ActiveSaveGame->HudConfig.SizePct = 125; GM->SaveGameToDisk(); RebuildModalContent(); } return FReply::Handled(); }))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("140% XXL"), (GM->ActiveSaveGame && GM->ActiveSaveGame->HudConfig.SizePct > 130) ? VTTheme::Cyan : VTTheme::PillBg, FLinearColor::White, 7, FMargin(3.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]() { if (GM->ActiveSaveGame) { GM->ActiveSaveGame->HudConfig.SizePct = 140; GM->SaveGameToDisk(); RebuildModalContent(); } return FReply::Handled(); }))
						]
					]

					// QUICK PRESETS
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 6.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("STANDARD ECKEN"), VTTheme::PillBg, VTTheme::Gold, 7, FMargin(3.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.LeftClusterOffsetX = 26.0f;
										GM->ActiveSaveGame->HudConfig.LeftClusterOffsetY = 28.0f;
										GM->ActiveSaveGame->HudConfig.RightClusterOffsetX = 26.0f;
										GM->ActiveSaveGame->HudConfig.RightClusterOffsetY = 28.0f;
										GM->ActiveSaveGame->HudConfig.SizePct = 100;
										GM->ActiveSaveGame->HudConfig.OpacityPct = 65;
										GM->SaveGameToDisk();
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("DAUMEN ERHÖHT"), VTTheme::PillBg, VTTheme::Gold, 7, FMargin(3.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.LeftClusterOffsetX = 32.0f;
										GM->ActiveSaveGame->HudConfig.LeftClusterOffsetY = 90.0f;
										GM->ActiveSaveGame->HudConfig.RightClusterOffsetX = 32.0f;
										GM->ActiveSaveGame->HudConfig.RightClusterOffsetY = 90.0f;
										GM->ActiveSaveGame->HudConfig.SizePct = 110;
										GM->ActiveSaveGame->HudConfig.OpacityPct = 75;
										GM->SaveGameToDisk();
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("KOMPAKT INNEN"), VTTheme::PillBg, VTTheme::Gold, 7, FMargin(3.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										GM->ActiveSaveGame->HudConfig.LeftClusterOffsetX = 70.0f;
										GM->ActiveSaveGame->HudConfig.LeftClusterOffsetY = 36.0f;
										GM->ActiveSaveGame->HudConfig.RightClusterOffsetX = 70.0f;
										GM->ActiveSaveGame->HudConfig.RightClusterOffsetY = 36.0f;
										GM->SaveGameToDisk();
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("SEITEN TAUSCHEN"), VTTheme::PillBg, VTTheme::Pink, 7, FMargin(3.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									if (GM->ActiveSaveGame)
									{
										const float TmpX = GM->ActiveSaveGame->HudConfig.LeftClusterOffsetX;
										const float TmpY = GM->ActiveSaveGame->HudConfig.LeftClusterOffsetY;
										GM->ActiveSaveGame->HudConfig.LeftClusterOffsetX = GM->ActiveSaveGame->HudConfig.RightClusterOffsetX;
										GM->ActiveSaveGame->HudConfig.LeftClusterOffsetY = GM->ActiveSaveGame->HudConfig.RightClusterOffsetY;
										GM->ActiveSaveGame->HudConfig.RightClusterOffsetX = TmpX;
										GM->ActiveSaveGame->HudConfig.RightClusterOffsetY = TmpY;
										GM->SaveGameToDisk();
										GM->PlayTowerSFX(TEXT("Click"));
										RebuildModalContent();
									}
									return FReply::Handled();
								}))
						]
					]

					// TEMPO SELECTION & ACCOUNT
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("SPIELTEMPO / GESCHWINDIGKEIT:")))
						.Font(VTTheme::Font(8, true))
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("TEMPO: 60% CHILL"), FMath::IsNearlyEqual(GM->BaseSpeedRatio, 0.60f, 0.05f) ? VTTheme::Cyan : VTTheme::PillBg, FLinearColor::White, 8, FMargin(4.0f, 6.0f),
								FOnClicked::CreateLambda([this, GM]() { GM->BaseSpeedRatio = 0.60f; GM->SaveGameToDisk(); GM->PlayTowerSFX(TEXT("Click")); RebuildModalContent(); return FReply::Handled(); }))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("TEMPO: 80% NORMAL"), FMath::IsNearlyEqual(GM->BaseSpeedRatio, 0.80f, 0.05f) ? VTTheme::Cyan : VTTheme::PillBg, FLinearColor::White, 8, FMargin(4.0f, 6.0f),
								FOnClicked::CreateLambda([this, GM]() { GM->BaseSpeedRatio = 0.80f; GM->SaveGameToDisk(); GM->PlayTowerSFX(TEXT("Click")); RebuildModalContent(); return FReply::Handled(); }))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
						[
							VTTheme::MakeButton(TEXT("TEMPO: 100% TURBO"), FMath::IsNearlyEqual(GM->BaseSpeedRatio, 1.00f, 0.05f) ? VTTheme::Gold : VTTheme::PillBg, FLinearColor::White, 8, FMargin(4.0f, 6.0f),
								FOnClicked::CreateLambda([this, GM]() { GM->BaseSpeedRatio = 1.00f; GM->SaveGameToDisk(); GM->PlayTowerSFX(TEXT("Click")); RebuildModalContent(); return FReply::Handled(); }))
						]
					]
				]
			]
		];

		// 3. ALL 6 MOBILE CONTROL TECHNIQUES WITH CLEAR EXPLANATIONS
		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("3. ALLE 6 HANDY-STEUERUNGSTECHNIKEN (INKL. PC-CONTROLLER SUPPORT):")))
			.Font(VTTheme::Font(9, true))
			.ColorAndOpacity(VTTheme::Cyan)
		];

		auto AddControlMethodCard = [this, GM, WhiteBrush](
			EVTControlMode Mode,
			const FString& Title,
			const FString& FamousGamesRef,
			const FString& HowToSteerText,
			const FString& HowToJumpText,
			const FLinearColor& AccentCol)
		{
			const bool bIsActive = (GM->CurrentControlMode == Mode);
			ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 3.0f)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(bIsActive ? VTTheme::Emerald : AccentCol)
				.Padding(FMargin(bIsActive ? 2.5f : 1.5f))
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(bIsActive ? FLinearColor(0.018f, 0.075f, 0.065f, 0.98f) : VTTheme::CardBg)
					.Padding(FMargin(10.0f, 7.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(Title))
									.Font(VTTheme::Font(9, true))
									.ColorAndOpacity(bIsActive ? VTTheme::Emerald : AccentCol)
								]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FamousGamesRef))
									.Font(VTTheme::Font(6, true))
									.ColorAndOpacity(VTTheme::Gold)
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 1.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(FString::Printf(TEXT("LENKEN: %s"), *HowToSteerText)))
								.Font(VTTheme::Font(7, true))
								.ColorAndOpacity(FLinearColor::White)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(FString::Printf(TEXT("SPRINGEN: %s"), *HowToJumpText)))
								.Font(VTTheme::Font(7, true))
								.ColorAndOpacity(VTTheme::Cyan)
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
						[
							VTTheme::MakeButton(
								bIsActive ? TEXT("AKTIV") : TEXT("WAEHLEN"),
								bIsActive ? VTTheme::Emerald : VTTheme::PillBg,
								bIsActive ? FLinearColor::White : AccentCol,
								8, FMargin(10.0f, 8.0f),
								FOnClicked::CreateLambda([this, GM, Mode]()
								{
									GM->PlayTowerSFX(TEXT("Click"));
									GM->SetControlModeDirect(Mode);
									RebuildModalContent();
									return FReply::Handled();
								}))
						]
					]
				]
			];
		};

		AddControlMethodCard(
			EVTControlMode::VirtualJoystick,
			TEXT("1. VIRTUELLER ANALOG-JOYSTICK + JUMP-BUTTON"),
			TEXT("(Brawl Stars / Fortnite Mobile)"),
			TEXT("Ziehe den subtilen Joystick links unten stufenlos nach links/rechts."),
			TEXT("Tippe rechts unten auf den JUMP-Button (halten = voller High-Jump)."),
			VTTheme::Cyan);

		AddControlMethodCard(
			EVTControlMode::ClassicButtons,
			TEXT("2. KLASSISCHE D-PAD TASTEN + JUMP-BUTTON"),
			TEXT("(Sonic / Super Mario Run)"),
			TEXT("Zwei direkte Richtungstasten links unten (< LEFT und RIGHT >)."),
			TEXT("Rechts unten auf den JUMP-Button tippen oder gedrueckt halten."),
			VTTheme::SkyBlue);

		AddControlMethodCard(
			EVTControlMode::OneThumbSlide,
			TEXT("3. 1-DAUMEN SLIDE & RELEASE (EINHAND-MODUS)"),
			TEXT("(Subway Surfers / Hypercasual)"),
			TEXT("Einen Daumen halten und stufenlos nach links/rechts gleiten lassen."),
			TEXT("Daumen kurz anheben (Loslassen) oder kurz nach oben wischen = Sprung!"),
			VTTheme::Gold);

		AddControlMethodCard(
			EVTControlMode::FullscreenSplit,
			TEXT("4. FULLSCREEN SPLIT-ZONES (LINKS / RECHTS)"),
			TEXT("(Geometry Dash Platformer)"),
			TEXT("Linke Bildschirmhaelfte halten = Links, rechte Haelfte halten = Rechts."),
			TEXT("Mit zweitem Daumen tippen oder ins obere Drittel tippen = Sprung."),
			VTTheme::Pink);

		AddControlMethodCard(
			EVTControlMode::SwipeGestures,
			TEXT("5. DYNAMIC SWIPE & FLICK GESTEN"),
			TEXT("(Temple Run / Blades of Brim)"),
			TEXT("Schnell nach links oder rechts wischen fuer sofortigen Richtungs-Sprint."),
			TEXT("Nach oben wischen (Swipe Up) fuer sofortigen Sprung."),
			VTTheme::Purple);

		AddControlMethodCard(
			EVTControlMode::AutoBounce,
			TEXT("6. AUTO-BOUNCE ZEN (DAUER-SPRINGEN)"),
			TEXT("(Doodle Jump / Auto-Hop)"),
			TEXT("Nur nach links oder rechts lenken."),
			TEXT("VOLLAUTOMATISCH bei jeder Landung auf einer Plattform!"),
			VTTheme::Emerald);

		// SECRET DEVELOPER VERSION FOOTER (5x Tap opens Hidden Admin / Godmode Console!)
		ModalBodyBox->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 10.0f, 0.0f, 2.0f)
		[
			SNew(SButton)
			.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
			.ContentPadding(FMargin(8.0f, 4.0f))
			.OnClicked_Lambda([this, GM]()
			{
				SecretAdminTapCount++;
				if (SecretAdminTapCount >= 5)
				{
					SecretAdminTapCount = 0;
					GM->PlayTowerSFX(TEXT("LevelUp"), 1.1f, 1.0f);
					OpenModal(EVTModalOverlay::AdminGodMode);
				}
				else
				{
					GM->PlayTowerSFX(TEXT("Click"), 0.8f + float(SecretAdminTapCount) * 0.1f, 0.5f);
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("VELOCITY TOWER v2.6.0-APEX // UNREAL ENGINE 5.8 BUILD 508")))
				.Font(VTTheme::Font(6, false))
				.ColorAndOpacity(FLinearColor(0.35f, 0.42f, 0.52f, 0.75f))
			]
		];
	}

	// =========================================================================
	// MODAL D: SECRET DEVELOPER ADMIN & GODMODE CONSOLE (HIDDEN FROM NORMAL USERS!)
	// =========================================================================
	else if (ActiveModal == EVTModalOverlay::AdminGodMode)
	{
		AddModalHeader(
			TEXT("DEVELOPER ONLY // GEHEIMES ADMIN & GODMODE TEST-LABOR (HOTKEY: [F12] ODER [G])"),
			TEXT("ADMIN GODMODE & LIVE-VARIABLEN KONSOLE"),
			VTTheme::Gold);

		// Section 1: Core Godmode & Cheat Toggles
		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(VTTheme::Gold)
			.Padding(FMargin(1.5f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(VTTheme::CardBg)
				.Padding(FMargin(10.0f, 8.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("1. GODMODE & UNENDLICHE FAEHIGKEITEN (ECHTZEIT-TOGGLES):")))
						.Font(VTTheme::Font(9, true))
						.ColorAndOpacity(VTTheme::Gold)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f)
						[
							VTTheme::MakeButton(
								GM->bAdminGodMode ? TEXT("GODMODE (KEIN TOD): AN") : TEXT("GODMODE (KEIN TOD): AUS"),
								GM->bAdminGodMode ? VTTheme::Emerald : VTTheme::PillBg,
								FLinearColor::White, 8, FMargin(8.0f, 8.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									GM->bAdminGodMode = !GM->bAdminGodMode;
									GM->PlayTowerSFX(TEXT("ComboUp"));
									RebuildModalContent();
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f)
						[
							VTTheme::MakeButton(
								GM->bAdminInfiniteJump ? TEXT("INFINITE AIR-JUMP: AN") : TEXT("INFINITE AIR-JUMP: AUS"),
								GM->bAdminInfiniteJump ? VTTheme::Cyan : VTTheme::PillBg,
								FLinearColor::White, 8, FMargin(8.0f, 8.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									GM->bAdminInfiniteJump = !GM->bAdminInfiniteJump;
									GM->PlayTowerSFX(TEXT("ComboUp"));
									RebuildModalContent();
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f)
						[
							VTTheme::MakeButton(
								GM->bAdminInfinitePowerups ? TEXT("DAUER-MAGNET & SCHILD: AN") : TEXT("DAUER-MAGNET & SCHILD: AUS"),
								GM->bAdminInfinitePowerups ? VTTheme::Pink : VTTheme::PillBg,
								FLinearColor::White, 8, FMargin(8.0f, 8.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									GM->bAdminInfinitePowerups = !GM->bAdminInfinitePowerups;
									GM->PlayTowerSFX(TEXT("ComboUp"));
									RebuildModalContent();
									return FReply::Handled();
								}))
						]
					]
				]
			]
		];

		// Section 2: Live Physics & Camera Variable Sliders / Steppers
		auto AddAdminVariableRow = [this, GM, WhiteBrush](
			const FString& VarName,
			float CurrentVal,
			float MinVal,
			float MaxVal,
			float StepVal,
			TFunction<void(float)> OnSetVal)
		{
			ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 2.5f)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.16f))
				.Padding(FMargin(1.0f))
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(VTTheme::CardBg)
					.Padding(FMargin(10.0f, 6.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%s:  %.2fx"), *VarName, CurrentVal)))
							.Font(VTTheme::Font(9, true))
							.ColorAndOpacity(VTTheme::Cyan)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f, 0.0f)
						[
							VTTheme::MakeButton(TEXT("- VERRINGERN"), VTTheme::PillBg, VTTheme::Pink, 8, FMargin(10.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM, CurrentVal, MinVal, MaxVal, StepVal, OnSetVal]()
								{
									OnSetVal(FMath::Clamp(CurrentVal - StepVal, MinVal, MaxVal));
									GM->PlayTowerSFX(TEXT("Click"), 0.9f, 0.8f);
									RebuildModalContent();
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f, 0.0f)
						[
							VTTheme::MakeButton(TEXT("1.0x STD"), VTTheme::PillBg, FLinearColor::White, 8, FMargin(8.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM, OnSetVal]()
								{
									OnSetVal(1.0f);
									GM->PlayTowerSFX(TEXT("Click"), 1.0f, 0.8f);
									RebuildModalContent();
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f, 0.0f)
						[
							VTTheme::MakeButton(TEXT("+ ERHOEHEN"), VTTheme::PillBg, VTTheme::Emerald, 8, FMargin(10.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM, CurrentVal, MinVal, MaxVal, StepVal, OnSetVal]()
								{
									OnSetVal(FMath::Clamp(CurrentVal + StepVal, MinVal, MaxVal));
									GM->PlayTowerSFX(TEXT("Click"), 1.15f, 0.8f);
									RebuildModalContent();
									return FReply::Handled();
								}))
						]
					]
				]
			];
		};

		AddAdminVariableRow(TEXT("SCHWERKRAFT (GRAVITY SCALE)"), GM->AdminGravityScale, 0.20f, 2.50f, 0.15f,
			[GM](float V) { GM->AdminGravityScale = V; });

		AddAdminVariableRow(TEXT("SPRUNGKRAFT (JUMP FORCE MULT)"), GM->AdminJumpForceScale, 0.50f, 3.00f, 0.20f,
			[GM](float V) { GM->AdminJumpForceScale = V; });

		AddAdminVariableRow(TEXT("LAUFTEMPO (HORIZONTAL SPEED MULT)"), GM->AdminMoveSpeedScale, 0.40f, 3.00f, 0.20f,
			[GM](float V) { GM->AdminMoveSpeedScale = V; });

		AddAdminVariableRow(TEXT("KAMERA-SCROLLTEMPO (0.0 = STOP)"), GM->AdminCameraSpeedScale, 0.00f, 3.00f, 0.25f,
			[GM](float V) { GM->AdminCameraSpeedScale = V; });

		AddAdminVariableRow(TEXT("PLATTFORM-BREITE (PLATFORM WIDTH)"), GM->AdminPlatformWidthScale, 0.50f, 1.80f, 0.15f,
			[GM](float V) { GM->AdminPlatformWidthScale = V; });

		// Section 3: Instant Floor / Biome Warp
		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("2. SOFORT-TELEPORT (ETAGEN & BIOM WARP):")))
			.Font(VTTheme::Font(9, true))
			.ColorAndOpacity(VTTheme::Gold)
		];

		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.5f)
			[
				VTTheme::MakeButton(TEXT("+10 ETAGEN"), VTTheme::CardBg, VTTheme::Cyan, 8, FMargin(6.0f, 7.0f),
					FOnClicked::CreateLambda([this, GM]() { CloseModal(); GM->AdminWarpFloors(10); return FReply::Handled(); }))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.5f)
			[
				VTTheme::MakeButton(TEXT("+50 ETAGEN"), VTTheme::CardBg, VTTheme::Cyan, 8, FMargin(6.0f, 7.0f),
					FOnClicked::CreateLambda([this, GM]() { CloseModal(); GM->AdminWarpFloors(50); return FReply::Handled(); }))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.5f)
			[
				VTTheme::MakeButton(TEXT("+100 ETAGEN (NAECHSTES BIOM)"), VTTheme::CardBg, VTTheme::Pink, 8, FMargin(6.0f, 7.0f),
					FOnClicked::CreateLambda([this, GM]() { CloseModal(); GM->AdminWarpFloors(100); return FReply::Handled(); }))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.5f)
			[
				VTTheme::MakeButton(TEXT("WARP ETAGE 500 (VOID)"), VTTheme::CardBg, VTTheme::Gold, 8, FMargin(6.0f, 7.0f),
					FOnClicked::CreateLambda([this, GM]() { CloseModal(); GM->MaxFloor = 0; GM->AdminWarpFloors(500); return FReply::Handled(); }))
			]
		];

		// Section 4: Economy, XP, Combo & 20-Character Roster Cheats
		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("3. ECONOMY, XP LEVEL-UP, COMBO & 20-CHARAKTER ROSTER TOOLS:")))
			.Font(VTTheme::Font(9, true))
			.ColorAndOpacity(VTTheme::Emerald)
		];

		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.5f)
			[
				VTTheme::MakeButton(TEXT("+1.000 NEON CORES"), VTTheme::Gold, FLinearColor::White, 8, FMargin(6.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]()
					{
						GM->AwardNeonCores(1000, TEXT("Admin Grant"));
						GM->PlayTowerSFX(TEXT("CorePickup"), 1.2f, 1.0f);
						RebuildModalContent();
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.5f)
			[
				VTTheme::MakeButton(TEXT("+1.000 XP (LEVEL-UP!)"), VTTheme::Emerald, FLinearColor::White, 8, FMargin(6.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]()
					{
						GM->AwardPlayerXP(1000, TEXT("Admin XP Boost"));
						RebuildModalContent();
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.5f)
			[
				VTTheme::MakeButton(TEXT("25x GODLIKE COMBO!"), VTTheme::Pink, FLinearColor::White, 8, FMargin(6.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]()
					{
						GM->AdminTriggerMegaCombo(25);
						CloseModal();
						return FReply::Handled();
					}))
			]
		];

		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.5f)
			[
				VTTheme::MakeButton(TEXT("ALLE 20 CHARAKTERE FREISCHALTEN"), VTTheme::Purple, FLinearColor::White, 8, FMargin(6.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]()
					{
						GM->AdminUnlockAllClimbers();
						RebuildModalContent();
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.5f)
			[
				VTTheme::MakeButton(TEXT("ROSTER RESET (NUR #01 KID FLEX)"), VTTheme::CardBg, VTTheme::Pink, 8, FMargin(6.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]()
					{
						GM->AdminResetRosterToStarter();
						RebuildModalContent();
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.5f)
			[
				VTTheme::MakeButton(TEXT("ALLE PHYSIK-WERTE AUF 1.0x"), VTTheme::CardBg, VTTheme::Cyan, 8, FMargin(6.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]()
					{
						GM->AdminResetPhysicsDefaults();
						RebuildModalContent();
						return FReply::Handled();
					}))
			]
		];
	}

	// =========================================================================
	// MODAL E: NEON SHOP & 20-CHARACTER ROSTER UNLOCK SYSTEM
	// =========================================================================
	else if (ActiveModal == EVTModalOverlay::NeonShop)
	{
		AddModalHeader(
			FString::Printf(TEXT("NEON SHOP // %d VON 20 CHARAKTEREN FREIGESCHALTET // GUTHABEN: %d CORES"), GM->GetUnlockedClimberCount(), GM->NeonCores),
			TEXT("HELDEN-ROSTER (20 CHARS) & NEON-SHOP"),
			VTTheme::Pink);

		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
			[
				VTTheme::MakeButton(TEXT("20 CHARAKTERE"), ActiveShopTab == 2 ? VTTheme::Pink : VTTheme::CardBg, FLinearColor::White, 8, FMargin(4.0f, 7.0f),
					FOnClicked::CreateLambda([this, GM]() { GM->PlayTowerSFX(TEXT("Click")); ActiveShopTab = 2; RebuildModalContent(); return FReply::Handled(); }))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
			[
				VTTheme::MakeButton(TEXT("3D SPUREN (TRAILS)"), ActiveShopTab == 3 ? VTTheme::Cyan : VTTheme::CardBg, FLinearColor::White, 8, FMargin(4.0f, 7.0f),
					FOnClicked::CreateLambda([this, GM]() { GM->PlayTowerSFX(TEXT("Click")); ActiveShopTab = 3; RebuildModalContent(); return FReply::Handled(); }))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
			[
				VTTheme::MakeButton(TEXT("GRATIS CORES"), ActiveShopTab == 0 ? VTTheme::Emerald : VTTheme::CardBg, FLinearColor::White, 8, FMargin(4.0f, 7.0f),
					FOnClicked::CreateLambda([this, GM]() { GM->PlayTowerSFX(TEXT("Click")); ActiveShopTab = 0; RebuildModalContent(); return FReply::Handled(); }))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
			[
				VTTheme::MakeButton(TEXT("START-ETAGE & BOOST"), ActiveShopTab == 1 ? VTTheme::Gold : VTTheme::CardBg, FLinearColor::White, 8, FMargin(4.0f, 7.0f),
					FOnClicked::CreateLambda([this, GM]() { GM->PlayTowerSFX(TEXT("Click")); ActiveShopTab = 1; RebuildModalContent(); return FReply::Handled(); }))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
			[
				VTTheme::MakeButton(TEXT("VIP PASS"), ActiveShopTab == 4 ? VTTheme::Purple : VTTheme::CardBg, FLinearColor::White, 8, FMargin(4.0f, 7.0f),
					FOnClicked::CreateLambda([this, GM]() { GM->PlayTowerSFX(TEXT("Click")); ActiveShopTab = 4; RebuildModalContent(); return FReply::Handled(); }))
			]
		];

		if (ActiveShopTab == 2)
		{
			ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(VTTheme::Gold)
				.Padding(FMargin(1.5f))
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(VTTheme::PillBg)
					.Padding(FMargin(10.0f, 6.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("SAMMLE NEON CORES IM TURM ODER OEFFNE DIE KLASSISCHE 4x5 CHARAKTER-AUSWAHL:")))
							.Font(VTTheme::Font(8, true))
							.ColorAndOpacity(VTTheme::Gold)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.0f, 0.0f)
						[
							VTTheme::MakeButton(TEXT("KLASSISCHES 4x5 GRID"), VTTheme::Cyan, FLinearColor::White, 8, FMargin(8.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									GM->PlayTowerSFX(TEXT("Click"));
									OpenModal(EVTModalOverlay::CharacterSelect);
									return FReply::Handled();
								}))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							VTTheme::MakeButton(TEXT("+100 NEON CORES"), VTTheme::Gold, FLinearColor::White, 8, FMargin(8.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM]()
								{
									GM->AwardNeonCores(100, TEXT("Shop Core Supply Drop"));
									GM->PlayTowerSFX(TEXT("CorePickup"));
									RebuildModalContent();
									return FReply::Handled();
								}))
						]
					]
				]
			];

			for (const FVTClimberInfo& C : GM->Climbers)
			{
				const bool bUnlocked = GM->IsClimberUnlocked(C.Id);
				const bool bEquipped = (GM->SelectedClimberId == C.Id);

				ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 3.0f)
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush)
					.BorderBackgroundColor(bEquipped ? VTTheme::Emerald : C.RarityColor)
					.Padding(FMargin(bEquipped ? 2.5f : 1.5f))
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(bEquipped ? FLinearColor(0.015f, 0.065f, 0.055f, 0.98f) : VTTheme::CardBg)
						.Padding(FMargin(10.0f, 8.0f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(C.VisorColor)
								.Padding(FMargin(2.0f))
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(C.HoodieColor)
									.Padding(FMargin(10.0f, 8.0f))
									[
										SNew(STextBlock)
										.Text(FText::FromString(FString::Printf(TEXT("#%02d"), C.RosterNumber)))
										.Font(VTTheme::Font(11, true))
										.ColorAndOpacity(FLinearColor(0.01f, 0.02f, 0.05f, 1.0f))
									]
								]
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[
										SNew(STextBlock).Text(FText::FromString(C.Name)).Font(VTTheme::Font(11, true)).ColorAndOpacity(FLinearColor::White)
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f)
									[
										SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(C.RarityColor).Padding(FMargin(6.0f, 1.5f))
										[
											SNew(STextBlock).Text(FText::FromString(C.Rarity)).Font(VTTheme::Font(6, true)).ColorAndOpacity(FLinearColor(0.01f, 0.02f, 0.05f, 1.0f))
										]
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[
										SNew(STextBlock).Text(FText::FromString(C.Style)).Font(VTTheme::Font(8, true)).ColorAndOpacity(VTTheme::Cyan)
									]
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
								[
									SNew(STextBlock).Text(FText::FromString(C.Description)).Font(VTTheme::Font(7, false)).ColorAndOpacity(VTTheme::MutedText)
								]
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								VTTheme::MakeButton(
									bEquipped ? TEXT("AUSGERUESTET") : (bUnlocked ? TEXT("AUSRUESTEN") : FString::Printf(TEXT("KAUFEN (%d CORES)"), C.CostCores)),
									bEquipped ? VTTheme::Emerald : (bUnlocked ? VTTheme::Cyan : VTTheme::Gold),
									FLinearColor::White, 8, FMargin(10.0f, 7.0f),
									FOnClicked::CreateLambda([this, GM, C]()
									{
										if (GM->BuyOrEquipClimber(C.Id))
										{
											GM->PlayTowerSFX(TEXT("ComboUp"));
										}
										else
										{
											GM->PlayTowerSFX(TEXT("Click"), 0.8f);
										}
										RebuildModalContent();
										return FReply::Handled();
									}))
							]
						]
					]
				];
			}
		}
		else if (ActiveShopTab == 3)
		{
			for (const FVTTrailInfo& T : GM->Trails)
			{
				const bool bUnlocked = GM->ActiveSaveGame && (GM->ActiveSaveGame->bIsVip || GM->ActiveSaveGame->UnlockedTrails.Contains(T.Id) || T.CostCores == 0);
				const bool bEquipped = (GM->SelectedTrailId == T.Id);

				ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
				[
					SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(T.Color).Padding(FMargin(1.5f))
					[
						SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(VTTheme::CardBg).Padding(FMargin(10.0f, 8.0f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock).Text(FText::FromString(T.Name)).Font(VTTheme::Font(10, true)).ColorAndOpacity(T.Color)
								]
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock).Text(FText::FromString(T.Description)).Font(VTTheme::Font(8, false)).ColorAndOpacity(VTTheme::MutedText)
								]
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								VTTheme::MakeButton(
									bEquipped ? TEXT("AKTIV") : (bUnlocked ? TEXT("AUSRUESTEN") : FString::Printf(TEXT("%d CORES"), T.CostCores)),
									bEquipped ? VTTheme::Emerald : (bUnlocked ? VTTheme::Cyan : VTTheme::Gold),
									FLinearColor::White, 8, FMargin(10.0f, 7.0f),
									FOnClicked::CreateLambda([this, GM, T]() { GM->BuyOrEquipTrail(T.Id); GM->PlayTowerSFX(TEXT("ComboUp")); RebuildModalContent(); return FReply::Handled(); }))
							]
						]
					]
				];
			}
		}
		else if (ActiveShopTab == 0)
		{
			ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
			[
				VTTheme::MakeButton(TEXT("WERBESPOT ANSEHEN (+25 NEON CORES GRATIS)"), VTTheme::Emerald, FLinearColor::White, 10, FMargin(12.0f, 10.0f),
					FOnClicked::CreateLambda([this]()
					{
						StartSimulatedAd([this]() { if (AVTTowerGameMode* InnerGM = GetTowerGM()) InnerGM->AwardNeonCores(25, TEXT("Rewarded Video Spot")); OpenModal(EVTModalOverlay::NeonShop); });
						return FReply::Handled();
					}))
			];
			ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
			[
				VTTheme::MakeButton(TEXT("MEGA-CORE-TREASURY (+150 NEON CORES)"), VTTheme::Gold, FLinearColor::White, 10, FMargin(12.0f, 10.0f),
					FOnClicked::CreateLambda([this, GM]() { GM->AwardNeonCores(150, TEXT("Mega Core Paket")); GM->PlayTowerSFX(TEXT("CorePickup")); RebuildModalContent(); return FReply::Handled(); }))
			];
		}
		else if (ActiveShopTab == 1)
		{
			ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f)
				[
					VTTheme::MakeButton(TEXT("START ETAGE 0 (STANDARD)"), GM->StartFloor == 0 ? VTTheme::Cyan : VTTheme::CardBg, FLinearColor::White, 8, FMargin(8.0f, 8.0f),
						FOnClicked::CreateLambda([this, GM]() { GM->StartFloor = 0; GM->SaveGameToDisk(); RebuildModalContent(); return FReply::Handled(); }))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f)
				[
					VTTheme::MakeButton(TEXT("HEADSTART ETAGE 50"), GM->StartFloor == 50 ? VTTheme::Gold : VTTheme::CardBg, FLinearColor::White, 8, FMargin(8.0f, 8.0f),
						FOnClicked::CreateLambda([this, GM]() { GM->StartFloor = 50; GM->SaveGameToDisk(); RebuildModalContent(); return FReply::Handled(); }))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f)
				[
					VTTheme::MakeButton(TEXT("WARP ETAGE 100"), GM->StartFloor == 100 ? VTTheme::Pink : VTTheme::CardBg, FLinearColor::White, 8, FMargin(8.0f, 8.0f),
						FOnClicked::CreateLambda([this, GM]() { GM->StartFloor = 100; GM->SaveGameToDisk(); RebuildModalContent(); return FReply::Handled(); }))
				]
			];
		}
		else
		{
			const bool bVip = GM->ActiveSaveGame && GM->ActiveSaveGame->bIsVip;
			ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 6.0f)
			[
				VTTheme::MakeButton(
					bVip ? TEXT("VIP PASS IST AKTIV (ALLE 20 CHARAKTERE FREI) [DEAKTIVIEREN]") : TEXT("VIP FOUNDER PASS AKTIVIEREN (SCHALTET ALLE 20 CHARS SOFORT FREI)"),
					VTTheme::Purple, FLinearColor::White, 10, FMargin(12.0f, 12.0f),
					FOnClicked::CreateLambda([this, GM]() { GM->ToggleVipStatus(); GM->PlayTowerSFX(TEXT("LevelUp")); RebuildModalContent(); return FReply::Handled(); }))
			];
		}
	}

	// =========================================================================
	// MODAL F: SUBTLE HUD OVERLAY CUSTOMIZER & DRAG-EDITOR
	// =========================================================================
	else if (ActiveModal == EVTModalOverlay::HudEditor)
	{
		AddModalHeader(TEXT("SUBTILES HANDY-OVERLAY // FREI VERSCHIEBBAR PER DRAG & DROP"), TEXT("HUD & JOYSTICK OVERLAY ANPASSEN"), VTTheme::Gold);

		FVTHudConfig& Cfg = GM->ActiveSaveGame->HudConfig;

		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
		[
			VTTheme::MakeButton(
				TEXT("LIVE DRAG & DROP MODUS STARTEN (JOYSTICK & JUMP FREI VERSCHIEBEN)"),
				VTTheme::Gold, FLinearColor::White, 11, FMargin(14.0f, 11.0f),
				FOnClicked::CreateLambda([this]()
				{
					bHudDragEditMode = true;
					CloseModal();
					return FReply::Handled();
				}))
		];

		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(VTTheme::CardBg).Padding(FMargin(12.0f, 10.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("AKTUELLE POSITION LINKS (JOYSTICK/D-PAD): X=%.0f px, Y=%.0f px | RECHTS (JUMP): X=%.0f px, Y=%.0f px"),
						Cfg.LeftClusterOffsetX, Cfg.LeftClusterOffsetY, Cfg.RightClusterOffsetX, Cfg.RightClusterOffsetY)))
					.Font(VTTheme::Font(8, true)).ColorAndOpacity(VTTheme::Cyan)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("SUBTILITAET / TRANSPARENZ: %d%% | ELEMENT-GROESSE: %d%%"), Cfg.OpacityPct, Cfg.SizePct)))
					.Font(VTTheme::Font(8, true)).ColorAndOpacity(VTTheme::Gold)
				]
			]
		];

		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f)
			[
				VTTheme::MakeButton(TEXT("SEHR SUBTIL (25% DECKKRAFT)"), Cfg.OpacityPct <= 30 ? VTTheme::Cyan : VTTheme::PillBg, FLinearColor::White, 8, FMargin(6.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]() { GM->ActiveSaveGame->HudConfig.OpacityPct = 25; GM->SaveGameToDisk(); RebuildModalContent(); return FReply::Handled(); }))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f)
			[
				VTTheme::MakeButton(TEXT("AUSGEWOGEN SUBTIL (42%)"), (Cfg.OpacityPct > 30 && Cfg.OpacityPct <= 55) ? VTTheme::Cyan : VTTheme::PillBg, FLinearColor::White, 8, FMargin(6.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]() { GM->ActiveSaveGame->HudConfig.OpacityPct = 42; GM->SaveGameToDisk(); RebuildModalContent(); return FReply::Handled(); }))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f)
			[
				VTTheme::MakeButton(TEXT("KRAEFTIG SICHTBAR (80%)"), Cfg.OpacityPct > 55 ? VTTheme::Cyan : VTTheme::PillBg, FLinearColor::White, 8, FMargin(6.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]() { GM->ActiveSaveGame->HudConfig.OpacityPct = 80; GM->SaveGameToDisk(); RebuildModalContent(); return FReply::Handled(); }))
			]
		];
	}

	// =========================================================================
	// MODAL G-1: DEDICATED LEADERBOARD & FRIENDS COMPARISON (GLOBAL VS. FRIENDS)
	// =========================================================================
	else if (ActiveModal == EVTModalOverlay::FriendsAndRivals)
	{
		AddModalHeader(
			TEXT("BESTENLISTE // VERGLEICHE DICH WELTWEIT ODER NUR MIT DEINEN FREUNDEN"),
			TEXT("LIVE BESTENLISTE & FREUNDE-RANKING"),
			VTTheme::Emerald);

		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				VTTheme::MakeButton(
					TEXT("WELTWEIT (GLOBAL TOP RANKING)"),
					!bLeaderboardFriendsOnly ? VTTheme::Cyan : VTTheme::CardBg,
					FLinearColor::White, 9, FMargin(10.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]()
					{
						GM->PlayTowerSFX(TEXT("Click"));
						bLeaderboardFriendsOnly = false;
						RebuildModalContent();
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				VTTheme::MakeButton(
					TEXT("NUR MIT FREUNDEN VERGLEICHEN (9)"),
					bLeaderboardFriendsOnly ? VTTheme::Emerald : VTTheme::CardBg,
					FLinearColor::White, 9, FMargin(10.0f, 8.0f),
					FOnClicked::CreateLambda([this, GM]()
					{
						GM->PlayTowerSFX(TEXT("Click"));
						bLeaderboardFriendsOnly = true;
						RebuildModalContent();
						return FReply::Handled();
					}))
			]
		];

		TSharedRef<SVerticalBox> ModalLbBox = SNew(SVerticalBox);
		PopulateLeaderboardRows(ModalLbBox, bLeaderboardFriendsOnly);
		ModalBodyBox->AddSlot().AutoHeight()
		[
			ModalLbBox
		];
	}

	// =========================================================================
	// MODAL G-2: REWARDS HUB, FORTUNE WHEEL, 7-DAY STREAK, QUESTS & TROPHIES
	// =========================================================================
	else if (ActiveModal == EVTModalOverlay::RewardsHub || ActiveModal == EVTModalOverlay::FortuneWheel || ActiveModal == EVTModalOverlay::LoginStreak || ActiveModal == EVTModalOverlay::DailyQuests || ActiveModal == EVTModalOverlay::TrophiesShowcase)
	{
		AddModalHeader(TEXT("BELOHNUNGEN, GLUECKSRAD, QUESTS & TROPHAEEN"), TEXT("APEX REWARDS & TROPHY CENTER"), VTTheme::Emerald);

		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
		[
			SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(VTTheme::Gold).Padding(FMargin(1.5f))
			[
				SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(VTTheme::CardBg).Padding(FMargin(12.0f, 10.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("NEON GLUECKSRAD (JACKPOT BIS ZU 100 CORES)"))).Font(VTTheme::Font(10, true)).ColorAndOpacity(VTTheme::Gold)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
					[
						SNew(STextBlock).Text(FText::FromString(LastWheelResultText)).Font(VTTheme::Font(11, true)).ColorAndOpacity(VTTheme::Cyan)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
					[
						VTTheme::MakeButton(TEXT("GLUECKSRAD JETZT DREHEN!"), VTTheme::Gold, FLinearColor::White, 10, FMargin(12.0f, 8.0f),
							FOnClicked::CreateLambda([this, GM]()
							{
								GM->PlayTowerSFX(TEXT("ComboUp"), 1.25f, 1.0f);
								LastWheelResultText = FString::Printf(TEXT("GEWONNEN: %s"), *GM->SpinFortuneWheel(true));
								RebuildModalContent();
								return FReply::Handled();
							}))
					]
				]
			]
		];

		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
		[
			VTTheme::MakeButton(TEXT("7-TAGE LOGIN-STREAK BELOHNUNG ABHOLEN (+XP & CORES)"), VTTheme::Emerald, FLinearColor::White, 10, FMargin(12.0f, 9.0f),
				FOnClicked::CreateLambda([this, GM]() { GM->ClaimDailyStreakReward(); GM->PlayTowerSFX(TEXT("CorePickup")); RebuildModalContent(); return FReply::Handled(); }))
		];

		if (GM->ActiveSaveGame)
		{
			for (int32 i = 0; i < GM->ActiveSaveGame->DailyQuests.Num(); ++i)
			{
				const FVTQuest& Q = GM->ActiveSaveGame->DailyQuests[i];
				ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 3.0f)
				[
					SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(VTTheme::CardBg).Padding(FMargin(10.0f, 7.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("QUEST: %s (%d/%d) -> +%d CORES"), *Q.Title, Q.Progress, Q.Target, Q.RewardCores)))
							.Font(VTTheme::Font(8, true)).ColorAndOpacity(FLinearColor::White)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							VTTheme::MakeButton(Q.bClaimed ? TEXT("ABGEHOLT") : TEXT("EINLOESEN"), VTTheme::Cyan, FLinearColor::White, 8, FMargin(8.0f, 5.0f),
								FOnClicked::CreateLambda([this, GM, i]() { GM->ClaimQuestReward(i); GM->PlayTowerSFX(TEXT("CorePickup")); RebuildModalContent(); return FReply::Handled(); }))
						]
					]
				];
			}
		}

		for (const FVTAchievement& Ach : GM->Achievements)
		{
			ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
			[
				SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(Ach.bUnlocked ? VTTheme::Emerald : VTTheme::CardBg).Padding(FMargin(10.0f, 5.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("[%s] %s — %s (+%d CORES)"), Ach.bUnlocked ? TEXT("FREI") : TEXT("LOCKED"), *Ach.Title, *Ach.Description, Ach.RewardCores)))
					.Font(VTTheme::Font(7, true)).ColorAndOpacity(FLinearColor::White)
				]
			];
		}

		// QUAKE-STYLE ENGLISH MALE ANNOUNCER SOUNDBOARD & UNLOCK TRACKER (24 VOICE LINES!)
		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 3.0f)
		[
			SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(VTTheme::Gold).Padding(FMargin(1.5f))
			[
				SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(VTTheme::CardBg).Padding(FMargin(10.0f, 8.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("QUAKE-ANNOUNCER STIMMEN FREIGESCHALTET: %d / 24 (+10 CORES JE NEUER STIMME!)"), GM->GetUnlockedVoiceCount())))
						.Font(VTTheme::Font(9, true))
						.ColorAndOpacity(VTTheme::Gold)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Steigere deine Kombi (2x bis 50x+) oder rette dich knapp vor dem Abgrund ('Oh, that was close!'), um alle 24 englischen Männerstimmen freizuschalten!")))
						.Font(VTTheme::Font(7, false))
						.ColorAndOpacity(VTTheme::MutedText)
						.AutoWrapText(true)
					]
				]
			]
		];

		struct FVoicePreviewEntry { FString Id; FString Label; };
		static const FVoicePreviewEntry AllVoices[25] = {
			{ TEXT("VO_Combo_01"), TEXT("01. [2x COMBO] DOUBLE JUMP! NICE!") },
			{ TEXT("VO_Combo_02"), TEXT("02. [3x COMBO] TRIPLE STREAK! SWEET!") },
			{ TEXT("VO_Combo_04"), TEXT("03. [6x COMBO] SUPER COMBO!") },
			{ TEXT("VO_Combo_05"), TEXT("04. [12x COMBO] IMPRESSIVE!") },
			{ TEXT("VO_Combo_06"), TEXT("05. [20x COMBO] EXCELLENT!") },
			{ TEXT("VO_Combo_07"), TEXT("06. [30x COMBO] OUTSTANDING! OVERDRIVE!") },
			{ TEXT("VO_Combo_13"), TEXT("07. [40x COMBO] DOMINATING!") },
			{ TEXT("VO_Combo_14"), TEXT("08. [55x COMBO] RAMPAGE!") },
			{ TEXT("VO_Combo_12"), TEXT("09. [70x COMBO] UNSTOPPABLE!") },
			{ TEXT("VO_Combo_10"), TEXT("10. [85x COMBO] SPECTACULAR!") },
			{ TEXT("VO_Combo_11"), TEXT("11. [105x COMBO] UNBELIEVABLE!") },
			{ TEXT("VO_Combo_16"), TEXT("12. [125x COMBO] ULTRA COMBO!") },
			{ TEXT("VO_Combo_17"), TEXT("13. [145x COMBO] MONSTER COMBO!") },
			{ TEXT("VO_Combo_18"), TEXT("14. [165x COMBO] LUDICROUS SPEED!") },
			{ TEXT("VO_Combo_HolyShit"), TEXT("15. [185x COMBO] HOLY SHIT!") },
			{ TEXT("VO_Combo_19"), TEXT("16. [200x COMBO] GODLIKE! APEX EMPEROR!") },
			{ TEXT("VO_Clutch_Close"), TEXT("17. [KNAPP GERETTET] OH, THAT WAS CLOSE!") },
			{ TEXT("VO_Clutch_Barely"), TEXT("18. [KNAPP GERETTET] BARELY MADE IT! WHAT A SAVE!") },
			{ TEXT("VO_Wall_Ricochet"), TEXT("19. [WANDBOUNCE] WICKED WALL BOUNCE!") },
			{ TEXT("VO_HighScore"), TEXT("20. [REKORD] NEW HIGH SCORE! KEEP CLIMBING!") },
			{ TEXT("VO_GameOver"), TEXT("21. [FINALE] WIPEOUT! GAME OVER!") },
			{ TEXT("VO_Combo_03"), TEXT("22. [STREAK BONUS] GREAT MOVE!") },
			{ TEXT("VO_Combo_08"), TEXT("23. [STREAK BONUS] AMAZING!") },
			{ TEXT("VO_Combo_09"), TEXT("24. [STREAK BONUS] INCREDIBLE!") },
			{ TEXT("VO_Combo_15"), TEXT("25. [STREAK BONUS] MULTI COMBO!") }
		};
		for (int32 v = 0; v < 25; ++v)
		{
			const FVoicePreviewEntry VEntry = AllVoices[v];
			const bool bVUnlocked = GM->ActiveSaveGame && GM->ActiveSaveGame->UnlockedVoiceLines.Contains(VEntry.Id);
			ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 1.5f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					VTTheme::MakeButton(
						bVUnlocked ? FString::Printf(TEXT("[FREI - ANHOEREN] %s"), *VEntry.Label) : FString::Printf(TEXT("[GESPERRT] %s"), *VEntry.Label),
						bVUnlocked ? FLinearColor(0.03f, 0.14f, 0.26f, 0.96f) : VTTheme::CardBg,
						bVUnlocked ? VTTheme::Emerald : VTTheme::MutedText,
						7, FMargin(8.0f, 4.0f),
						FOnClicked::CreateLambda([GM, VEntry, bVUnlocked]()
						{
							if (bVUnlocked)
							{
								GM->PlayTowerSFX(VEntry.Id, 1.0f, 1.25f);
							}
							else
							{
								GM->PlayTowerSFX(TEXT("Click"), 0.8f, 0.8f);
							}
							return FReply::Handled();
						}))
				]
			];
		}
	}

	// =========================================================================
	// MODAL H: ACCOUNT PORTAL & AD PLAYER
	// =========================================================================
	else if (ActiveModal == EVTModalOverlay::AccountCloudSync)
	{
		AddModalHeader(TEXT("GLOBAL APEX ID // GAST, NEUES KONTO ODER 3RD-PARTY LOGIN"), TEXT("KONTO & CLOUD-SYNC PORTAL"), VTTheme::Cyan);

		ModalBodyBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
		[
			VTTheme::MakeButton(
				TEXT("VOLLSTAENDIGEN ANMELDE-SCREEN OEFFNEN (GAST / NEUES KONTO / APPS)"),
				VTTheme::Cyan, FLinearColor::White, 10, FMargin(14.0f, 10.0f),
				FOnClicked::CreateLambda([this, GM]()
				{
					GM->PlayTowerSFX(TEXT("Click"));
					CloseModal();
					GM->BootFlowState = EVTBootFlowState::AccountGate;
					return FReply::Handled();
				}))
		];
	}
	else if (ActiveModal == EVTModalOverlay::AdPlayer)
	{
		AddModalHeader(TEXT("SPONSORED REWARD STREAM"), TEXT("GRATIS NEON CORES WERBESPOT"), VTTheme::Gold);
		ModalBodyBox->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 20.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Belohnung wird freigeschaltet in %.1f Sekunden..."), AdCountdownRemaining)))
			.Font(VTTheme::Font(12, true)).ColorAndOpacity(VTTheme::Cyan)
		];
	}
}

void UVTMainHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AVTTowerGameMode* GM = GetTowerGM();
	if (!GM) return;

	AVTPlayerController* PC = GetTowerPC();

	// 1. Update Boot Flow Overlays (Unreal Engine 5.8 Splash -> Teaser Intro -> Account Login Gate -> Ready)
	if (EngineSplashOverlayBorder.IsValid())
	{
		const bool bShowSplash = (GM->BootFlowState == EVTBootFlowState::EngineSplash);
		EngineSplashOverlayBorder->SetVisibility(bShowSplash ? EVisibility::Visible : EVisibility::Collapsed);
		if (bShowSplash)
		{
			const float Pct = FMath::Clamp(GM->BootTimerSec / 2.5f, 0.0f, 1.0f);
			if (SplashProgressFillBox.IsValid()) SplashProgressFillBox->SetWidthOverride(FMath::Max(12.0f, Pct * 416.0f));
			if (TxtSplashPercent.IsValid()) TxtSplashPercent->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Pct * 100.0f))));
			if (TxtSplashStatus.IsValid())
			{
				if (Pct < 0.35f) TxtSplashStatus->SetText(FText::FromString(TEXT("INITIALISIERE UNREAL ENGINE 5.8 LUMEN GLOBAL ILLUMINATION...")));
				else if (Pct < 0.70f) TxtSplashStatus->SetText(FText::FromString(TEXT("LADE SYNTHWAVE AUDIO-ENGINE & 20 CLIMBER-CHARAKTERE...")));
				else TxtSplashStatus->SetText(FText::FromString(TEXT("NANO BANANA 2 UI & 60HZ APEX-PHYSIK BEREIT!")));
			}
		}
	}

	if (TeaserIntroOverlayBorder.IsValid())
	{
		const bool bShowTeaser = (GM->BootFlowState == EVTBootFlowState::TeaserIntro);
		TeaserIntroOverlayBorder->SetVisibility(bShowTeaser ? EVisibility::Visible : EVisibility::Collapsed);
		if (bShowTeaser)
		{
			const float TeaserElapsed = GM->BootTimerSec - 2.6f;
			if (TxtTeaserHeadline.IsValid() && TxtTeaserSubline.IsValid())
			{
				if (TeaserElapsed < 1.8f)
				{
					TxtTeaserHeadline->SetText(FText::FromString(TEXT("NEO-VICE // JAHR 2099")));
					TxtTeaserSubline->SetText(FText::FromString(TEXT("Jenseits der Kryo-Wolkenkratzer erhebt sich der endlose Eis-Turm in die Stratosphaere.")));
				}
				else if (TeaserElapsed < 3.6f)
				{
					TxtTeaserHeadline->SetText(FText::FromString(TEXT("20 ELITE-CLIMBER // 1 LEGENDE")));
					TxtTeaserSubline->SetText(FText::FromString(TEXT("Waehle deinen Modus, stelle dich der klassischen 20-Charakter-Auswahl und brich den Rekord!")));
				}
				else
				{
					TxtTeaserHeadline->SetText(FText::FromString(TEXT("VELOCITY TOWER // VICE & ICE APEX")));
					TxtTeaserSubline->SetText(FText::FromString(TEXT("Unreal Engine 5.8 3D Edition — Bereit fuer den Aufstieg!")));
				}
			}
		}
	}

	if (AccountGateOverlayBorder.IsValid())
	{
		const bool bShowGate = (GM->BootFlowState == EVTBootFlowState::AccountGate);
		AccountGateOverlayBorder->SetVisibility(bShowGate ? EVisibility::Visible : EVisibility::Collapsed);
	}

	// 2. Main Lobby vs Play vs Game Over Visibility
	const bool bBootReady = (GM->BootFlowState == EVTBootFlowState::Ready);
	const bool bHas1to1ThemeFrame = ThemeFrameBrushes.Contains(GM->SelectedThemeId);
	if (InGameTopHudBorder.IsValid())
	{
		const bool bShowInGame = bBootReady && (GM->TowerRunState == EVTGameState::Play) && !bHas1to1ThemeFrame;
		InGameTopHudBorder->SetVisibility(bShowInGame ? EVisibility::Visible : EVisibility::Collapsed);
	}
	if (TxtLiveThemeSwitcherLabel.IsValid())
	{
		const FVTGameThemeInfo CurT = GM->GetThemeInfo(GM->SelectedThemeId);
		TxtLiveThemeSwitcherLabel->SetText(FText::FromString(FString::Printf(TEXT("WELT %d/%d: %s"), CurT.ThemeNumber, GM->Themes.Num(), *CurT.Name)));
	}
	if (MainMenuOverlayBorder.IsValid())
	{
		const bool bShowLobby = bBootReady && (GM->TowerRunState == EVTGameState::Menu) && !bHudDragEditMode && (ActiveModal == EVTModalOverlay::None);
		MainMenuOverlayBorder->SetVisibility(bShowLobby ? EVisibility::Visible : EVisibility::Collapsed);
	}
	if (GameOverOverlayBorder.IsValid())
	{
		GameOverOverlayBorder->SetVisibility((bBootReady && GM->TowerRunState == EVTGameState::GameOver) ? EVisibility::Visible : EVisibility::Collapsed);
	}
	if (HudDragBannerBorder.IsValid())
	{
		HudDragBannerBorder->SetVisibility(bHudDragEditMode ? EVisibility::Visible : EVisibility::Collapsed);
	}
	if (AdminBadgeBorder.IsValid())
	{
		const bool bAnyAdminActive = GM->bAdminGodMode || GM->bAdminInfiniteJump || GM->bAdminInfinitePowerups;
		AdminBadgeBorder->SetVisibility(bAnyAdminActive ? EVisibility::Visible : EVisibility::Collapsed);
	}

	// 3. Subtle Draggable Mobile Joystick / Button Overlay Update
	if (TouchHudContainer.IsValid())
	{
		const bool bShowTouchOverlay = bHudDragEditMode || (bBootReady && GM->TowerRunState == EVTGameState::Play);
		TouchHudContainer->SetVisibility(bShowTouchOverlay ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);

		if (GM->ActiveSaveGame)
		{
			FVTHudConfig& Cfg = GM->ActiveSaveGame->HudConfig;

			if (PC && DraggingClusterIndex != 0)
			{
				float PointerX = 0.0f, PointerY = 0.0f;
				bool bPointerDown = false;
				PC->GetInputTouchState(ETouchIndex::Touch1, PointerX, PointerY, bPointerDown);
				if (!bPointerDown)
				{
					if (PC->IsInputKeyDown(EKeys::LeftMouseButton))
					{
						bPointerDown = PC->GetMousePosition(PointerX, PointerY);
					}
				}

				if (bPointerDown)
				{
					const FVector2D CurPointer(PointerX, PointerY);
					if (bDragJustStarted)
					{
						DragLastMousePos = CurPointer;
						bDragJustStarted = false;
					}
					else
					{
						FVector2D Delta = CurPointer - DragLastMousePos;
						DragLastMousePos = CurPointer;
						Delta.X = FMath::Clamp(Delta.X, -45.0f, 45.0f);
						Delta.Y = FMath::Clamp(Delta.Y, -45.0f, 45.0f);

						if (DraggingClusterIndex == 1) // Button LINKS
						{
							Cfg.LeftClusterOffsetX = FMath::Clamp(Cfg.LeftClusterOffsetX + Delta.X, 4.0f, 650.0f);
							Cfg.LeftClusterOffsetY = FMath::Clamp(Cfg.LeftClusterOffsetY - Delta.Y, 4.0f, 850.0f);
						}
						else if (DraggingClusterIndex == 2) // Button RECHTS
						{
							Cfg.RightSteerOffsetX = FMath::Clamp(Cfg.RightSteerOffsetX + Delta.X, 4.0f, 650.0f);
							Cfg.RightSteerOffsetY = FMath::Clamp(Cfg.RightSteerOffsetY - Delta.Y, 4.0f, 850.0f);
						}
						else if (DraggingClusterIndex == 3) // Button JUMP
						{
							Cfg.RightClusterOffsetX = FMath::Clamp(Cfg.RightClusterOffsetX - Delta.X, 4.0f, 650.0f);
							Cfg.RightClusterOffsetY = FMath::Clamp(Cfg.RightClusterOffsetY - Delta.Y, 4.0f, 850.0f);
						}
					}
				}
				else
				{
					DraggingClusterIndex = 0;
				}
			}

			// Active Button Visual Indicators (Toolbar title, toolbar tabs, and on-screen borders)
			const float PulseAnim = 0.5f + 0.5f * FMath::Sin((GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) * 6.5f);
			const FLinearColor ActiveGold = FMath::Lerp(FLinearColor(1.0f, 0.82f, 0.15f, 1.0f), FLinearColor(1.0f, 0.98f, 0.55f, 1.0f), PulseAnim);
			const FLinearColor InactiveBorder(0.2f, 0.35f, 0.5f, 0.45f);

			if (TxtActiveButtonTitle.IsValid())
			{
				FString ActiveName;
				float PosXVal = 0.0f, PosYVal = 0.0f;
				if (SelectedEditButtonIndex == 1)
				{
					ActiveName = TEXT("1. [ ◀ LINKS ]");
					PosXVal = Cfg.LeftClusterOffsetX;
					PosYVal = Cfg.LeftClusterOffsetY;
				}
				else if (SelectedEditButtonIndex == 2)
				{
					ActiveName = TEXT("2. [ RECHTS ▶ ]");
					PosXVal = Cfg.RightSteerOffsetX;
					PosYVal = Cfg.RightSteerOffsetY;
				}
				else
				{
					ActiveName = TEXT("3. [ ▲▲ JUMP ]");
					PosXVal = Cfg.RightClusterOffsetX;
					PosYVal = Cfg.RightClusterOffsetY;
				}
				TxtActiveButtonTitle->SetText(FText::FromString(FString::Printf(TEXT("AKTIV: %s  //  X=%.0f px, Y=%.0f px"), *ActiveName, PosXVal, PosYVal)));
			}

			if (TabBorderBtnLinks.IsValid())
			{
				TabBorderBtnLinks->SetBorderBackgroundColor(SelectedEditButtonIndex == 1 ? ActiveGold : InactiveBorder);
			}
			if (TabBorderBtnRechts.IsValid())
			{
				TabBorderBtnRechts->SetBorderBackgroundColor(SelectedEditButtonIndex == 2 ? ActiveGold : InactiveBorder);
			}
			if (TabBorderBtnJump.IsValid())
			{
				TabBorderBtnJump->SetBorderBackgroundColor(SelectedEditButtonIndex == 3 ? ActiveGold : InactiveBorder);
			}

			if (bHudDragEditMode)
			{
				if (LeftButtonHighlightBorder.IsValid())
				{
					LeftButtonHighlightBorder->SetBorderBackgroundColor(SelectedEditButtonIndex == 1 ? ActiveGold : InactiveBorder);
				}
				if (RightButtonHighlightBorder.IsValid())
				{
					RightButtonHighlightBorder->SetBorderBackgroundColor(SelectedEditButtonIndex == 2 ? ActiveGold : InactiveBorder);
				}
				if (JumpButtonHighlightBorder.IsValid())
				{
					JumpButtonHighlightBorder->SetBorderBackgroundColor(SelectedEditButtonIndex == 3 ? ActiveGold : InactiveBorder);
				}
			}
			else
			{
				if (LeftButtonHighlightBorder.IsValid()) LeftButtonHighlightBorder->SetBorderBackgroundColor(FLinearColor(0.0f, 0.961f, 0.831f, 0.85f));
				if (RightButtonHighlightBorder.IsValid()) RightButtonHighlightBorder->SetBorderBackgroundColor(FLinearColor(0.0f, 0.961f, 0.831f, 0.85f));
				if (JumpButtonHighlightBorder.IsValid()) JumpButtonHighlightBorder->SetBorderBackgroundColor(FLinearColor(0.969f, 0.145f, 0.522f, 0.85f));
			}

			if (PC && bVirtualJoystickMouseDrag && !bHudDragEditMode)
			{
				float MX = 0.0f, MY = 0.0f;
				if (PC->GetMousePosition(MX, MY))
				{
					const float Dx = (MX - VirtualJoystickMouseOrigin.X) / 48.0f;
					const float Dy = (MY - VirtualJoystickMouseOrigin.Y) / 48.0f;
					PC->JoystickVisualX = FMath::Clamp(Dx, -1.0f, 1.0f);
					PC->JoystickVisualY = FMath::Clamp(Dy, -1.0f, 1.0f);
					PC->AnalogSteer = PC->JoystickVisualX;
					PC->bKeyLeft = (PC->JoystickVisualX < -0.15f);
					PC->bKeyRight = (PC->JoystickVisualX > 0.15f);
				}
			}

			const float Alpha = FMath::Clamp(float(Cfg.OpacityPct) / 100.0f, 0.15f, 0.95f);
			TouchHudContainer->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, Alpha));

			if (LeftTouchClusterBorder.IsValid())
			{
				LeftTouchClusterBorder->SetPadding(FMargin(Cfg.LeftClusterOffsetX, 0.0f, 0.0f, Cfg.LeftClusterOffsetY));
			}
			if (RightSteerClusterBorder.IsValid())
			{
				RightSteerClusterBorder->SetPadding(FMargin(Cfg.RightSteerOffsetX, 0.0f, 0.0f, Cfg.RightSteerOffsetY));
			}
			if (RightTouchClusterBorder.IsValid())
			{
				RightTouchClusterBorder->SetPadding(FMargin(0.0f, 0.0f, Cfg.RightClusterOffsetX, Cfg.RightClusterOffsetY));
			}

			const float ScaleMult = FMath::Clamp(float(Cfg.SizePct) / 100.0f, 0.65f, 1.50f);
			if (LeftClusterSizeBox.IsValid())
			{
				LeftClusterSizeBox->SetWidthOverride(80.0f * ScaleMult);
				LeftClusterSizeBox->SetHeightOverride(70.0f * ScaleMult);
			}
			if (RightSteerSizeBox.IsValid())
			{
				RightSteerSizeBox->SetWidthOverride(80.0f * ScaleMult);
				RightSteerSizeBox->SetHeightOverride(70.0f * ScaleMult);
			}
			if (RightClusterSizeBox.IsValid())
			{
				RightClusterSizeBox->SetWidthOverride(92.0f * ScaleMult);
				RightClusterSizeBox->SetHeightOverride(92.0f * ScaleMult);
			}

			if (TxtEditorOpacity.IsValid())
			{
				TxtEditorOpacity->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), Cfg.OpacityPct)));
			}
			if (TxtEditorSize.IsValid())
			{
				TxtEditorSize->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), Cfg.SizePct)));
			}

			const bool bUseDpadButtons = (GM->CurrentControlMode == EVTControlMode::ClassicButtons);
			if (VirtualJoystickModeBox.IsValid()) VirtualJoystickModeBox->SetVisibility((!bHas1to1ThemeFrame && !bUseDpadButtons) ? EVisibility::Visible : EVisibility::Collapsed);
			if (ClassicDpadModeBox.IsValid()) ClassicDpadModeBox->SetVisibility((bHudDragEditMode || (!bHas1to1ThemeFrame && bUseDpadButtons)) ? EVisibility::Visible : EVisibility::Collapsed);

			if (JoystickKnobBorder.IsValid() && PC)
			{
				const float KnobShiftX = PC->JoystickVisualX * 26.0f;
				const float KnobShiftY = PC->JoystickVisualY * 26.0f;
				JoystickKnobBorder->SetPadding(FMargin(
					FMath::Max(0.0f, KnobShiftX * 2.0f),
					FMath::Max(0.0f, KnobShiftY * 2.0f),
					FMath::Max(0.0f, -KnobShiftX * 2.0f),
					FMath::Max(0.0f, -KnobShiftY * 2.0f)));
			}
		}
	}

	// 4. Update Top HUD, Dopamine Level/XP Bar & Juicy Combo Fever Meter
	const FVTBiomeInfo Biome = GM->GetBiomeForFloor(GM->MaxFloor);
	if (TxtBiomeLabel.IsValid())
	{
		TxtBiomeLabel->SetText(FText::FromString(FString::Printf(TEXT("BIOME %d // %s"), Biome.BiomeIndex, *Biome.Name)));
		TxtBiomeLabel->SetColorAndOpacity(Biome.TopColor);
	}
	if (TxtScoreVal.IsValid())
	{
		TxtScoreVal->SetText(FText::AsNumber(GM->CurrentScore));
		if (GM->ScorePulseAnim > 0.0f)
		{
			TxtScoreVal->SetColorAndOpacity(FMath::Lerp(FLinearColor::White, VTTheme::Gold, GM->ScorePulseAnim));
		}
		else
		{
			TxtScoreVal->SetColorAndOpacity(FLinearColor::White);
		}
	}
	if (TxtTopBestScoreVal.IsValid())
	{
		const int32 DisplayBest = FMath::Max(GM->CurrentScore, GM->BestScore);
		TxtTopBestScoreVal->SetText(FText::AsNumber(DisplayBest));
		if (TopBestScoreBorder.IsValid())
		{
			if (GM->bNewHighScoreAchieved)
			{
				const float FlashPulse = FMath::Sin((GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) * 12.0f) * 0.5f + 0.5f;
				TopBestScoreBorder->SetBorderBackgroundColor(FMath::Lerp(VTTheme::Gold, VTTheme::Pink, FlashPulse));
			}
			else
			{
				TopBestScoreBorder->SetBorderBackgroundColor(VTTheme::Gold);
			}
		}
	}
	if (TxtFloorVal.IsValid()) TxtFloorVal->SetText(FText::AsNumber(GM->MaxFloor));
	if (TxtBpmVal.IsValid()) TxtBpmVal->SetText(FText::AsNumber(GM->CurrentBpm));
	if (TxtCoresVal.IsValid()) TxtCoresVal->SetText(FText::AsNumber(GM->NeonCores));

	// Update Dedicated Live Tower Altitude & Height Overlay
	if (LiveAltitudeHudBorder.IsValid())
	{
		const bool bInGamePlay = (GM->TowerRunState == EVTGameState::Play && !GM->bIsDeathSequenceActive && !bHas1to1ThemeFrame);
		LiveAltitudeHudBorder->SetVisibility(bInGamePlay ? EVisibility::Visible : EVisibility::Collapsed);

		if (bInGamePlay)
		{
			if (GM->MaxFloor > LastRecordedFloorForAltitudePulse)
			{
				LastRecordedFloorForAltitudePulse = GM->MaxFloor;
				AltitudePulseTimerSec = 0.45f; // Flash border when ascending to higher floor!
			}

			if (AltitudePulseTimerSec > 0.0f)
			{
				AltitudePulseTimerSec = FMath::Max(0.0f, AltitudePulseTimerSec - InDeltaTime);
				const float PulseAlpha = AltitudePulseTimerSec / 0.45f;
				const FLinearColor PulseCol = FMath::Lerp(FLinearColor(0.0f, 0.961f, 0.831f, 0.85f), FLinearColor::White, PulseAlpha);
				LiveAltitudeHudBorder->SetBorderBackgroundColor(PulseCol);
			}
			else
			{
				LiveAltitudeHudBorder->SetBorderBackgroundColor(FLinearColor(0.0f, 0.961f, 0.831f, 0.85f));
			}

			if (TxtLiveFloorNumber.IsValid())
			{
				TxtLiveFloorNumber->SetText(FText::FromString(FString::Printf(TEXT("ETAGE %d"), GM->MaxFloor)));
			}
			if (TxtLiveAltitudeMeters.IsValid())
			{
				TxtLiveAltitudeMeters->SetText(FText::FromString(FString::Printf(TEXT("%d METER"), GM->MaxFloor * 3)));
			}
		}
	}

	// 1. TOP LEFT: COMBO-STICK // CONSECUTIVE COMBO STREAK & ESCALATION BAR
	const float ComboRatio = FMath::Clamp(float(GM->CurrentCombo) / 20.0f, 0.0f, 1.0f);
	if (TxtTopComboBadge.IsValid())
	{
		if (GM->CurrentCombo > 0)
		{
			TxtTopComboBadge->SetText(FText::FromString(FString::Printf(TEXT("COMBO STICK // %dx MULTI"), GM->CurrentCombo)));
		}
		else
		{
			TxtTopComboBadge->SetText(FText::FromString(TEXT("COMBO STICK // 0x BEREIT")));
		}
	}
	if (TxtTopComboSub.IsValid())
	{
		if (GM->CurrentCombo > 0)
		{
			TxtTopComboSub->SetText(FText::FromString(FString::Printf(TEXT("%.1fs TIMER"), float(GM->ComboTimerFrames) / 60.0f)));
		}
		else
		{
			TxtTopComboSub->SetText(FText::FromString(TEXT("SPRINGE WEITER!")));
		}
	}
	if (TopComboFillBox.IsValid())
	{
		TopComboFillBox->SetWidthOverride(FMath::Max(6.0f, ComboRatio * 250.0f));
	}
	if (TopComboFillBorder.IsValid())
	{
		FLinearColor ComboCol = VTTheme::Cyan;
		if (GM->CurrentCombo >= 20)
		{
			const float FeverP = FMath::Sin((GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) * 12.0f) * 0.5f + 0.5f;
			ComboCol = FMath::Lerp(VTTheme::Pink, VTTheme::Gold, FeverP);
		}
		else if (GM->CurrentCombo >= 10)
		{
			ComboCol = VTTheme::Gold;
		}
		else if (GM->CurrentCombo >= 5)
		{
			ComboCol = FLinearColor(0.25f, 0.95f, 0.55f);
		}
		else if (GM->CurrentCombo > 0)
		{
			ComboCol = VTTheme::Cyan;
		}
		else
		{
			ComboCol = FLinearColor(0.08f, 0.16f, 0.28f, 0.7f);
		}
		TopComboFillBorder->SetBorderBackgroundColor(ComboCol);
	}

	// 2. TOP RIGHT: DOPAMINE LEVEL & XP PROGRESSION BAR
	const float XpRatio = FMath::Clamp(float(GM->CurrentXP) / float(FMath::Max(1, GM->XPToNextLevel)), 0.0f, 1.0f);
	if (TxtTopLevelBadge.IsValid())
	{
		TxtTopLevelBadge->SetText(FText::FromString(FString::Printf(TEXT("LVL %d // %s"), GM->PlayerLevel, *GM->GetPlayerRankTitle())));
	}
	if (TxtTopXpNumbers.IsValid())
	{
		TxtTopXpNumbers->SetText(FText::FromString(FString::Printf(TEXT("%d / %d XP"), GM->CurrentXP, GM->XPToNextLevel)));
	}
	if (TopXpFillBox.IsValid())
	{
		TopXpFillBox->SetWidthOverride(FMath::Max(8.0f, XpRatio * 250.0f));
	}
	if (TopXpFillBorder.IsValid())
	{
		const FLinearColor XpCol = FMath::Lerp(VTTheme::Gold, FLinearColor::White, FMath::Clamp(GM->XpGainPulse, 0.0f, 1.0f));
		TopXpFillBorder->SetBorderBackgroundColor(XpCol);
	}
	if (LevelUpBannerBorder.IsValid() && TxtLevelUpBanner.IsValid())
	{
		const bool bShowLvlBanner = (GM->LevelUpBannerTimerSec > 0.0f);
		LevelUpBannerBorder->SetVisibility(bShowLvlBanner ? EVisibility::Visible : EVisibility::Collapsed);
		if (bShowLvlBanner)
		{
			TxtLevelUpBanner->SetText(FText::FromString(GM->LatestLevelUpText));
		}
	}
	if (BlitzTimerBorder.IsValid() && TxtBlitzTimer.IsValid())
	{
		const bool bShowBlitz = (GM->TowerRunState == EVTGameState::Play && GM->CurrentGameMode == EVTGameModeType::Blitz60s);
		BlitzTimerBorder->SetVisibility(bShowBlitz ? EVisibility::Visible : EVisibility::Collapsed);
		if (bShowBlitz)
		{
			TxtBlitzTimer->SetText(FText::FromString(FString::Printf(TEXT("%.1fs"), float(GM->BlitzFramesRemaining) / 60.0f)));
		}
	}

	if (DeadBannerOverlayBorder.IsValid())
	{
		const bool bShowDead = GM->bIsDeathSequenceActive && GM->bDeadPopupShown;
		DeadBannerOverlayBorder->SetVisibility(bShowDead ? EVisibility::Visible : EVisibility::Collapsed);
		if (bShowDead)
		{
			DeadBannerOverlayBorder->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, GM->DeadPopupAlpha));
		}
	}
	if (HighScoreBannerOverlayBorder.IsValid())
	{
		const bool bShowHigh = GM->HighScoreCelebrationTimer > 0.0f;
		HighScoreBannerOverlayBorder->SetVisibility(bShowHigh ? EVisibility::Visible : EVisibility::Collapsed);
		if (bShowHigh)
		{
			const float Pulse = FMath::Sin((GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) * 10.0f) * 0.5f + 0.5f;
			HighScoreBannerOverlayBorder->SetBorderBackgroundColor(FMath::Lerp(VTTheme::Gold, FLinearColor(1.0f, 0.4f, 0.1f, 1.0f), Pulse));
		}
	}

	if (ToastBannerBorder.IsValid() && TxtToastMessage.IsValid())
	{
		ToastBannerBorder->SetVisibility(GM->ToastTimerSec > 0.0f ? EVisibility::Visible : EVisibility::Collapsed);
		if (GM->ToastTimerSec > 0.0f) TxtToastMessage->SetText(FText::FromString(GM->LatestToastMessage));
	}
	if (HurryBannerBorder.IsValid())
	{
		HurryBannerBorder->SetVisibility(GM->HurryBannerTimerSec > 0.0f ? EVisibility::Visible : EVisibility::Collapsed);
	}
	if (PowerUpBannerBorder.IsValid() && TxtPowerUpBanner.IsValid())
	{
		const bool bHasItemBonus = (GM->ItemBonusTimerSec > 0.0f);
		const bool bHasActiveBuff = (GM->MagnetFrames > 0 || GM->FreezeFrames > 0 || GM->ShieldCount > 0);
		PowerUpBannerBorder->SetVisibility((bHasItemBonus || bHasActiveBuff) ? EVisibility::Visible : EVisibility::Collapsed);
		if (bHasItemBonus)
		{
			TxtPowerUpBanner->SetText(FText::FromString(GM->LatestItemBonusText));
			TxtPowerUpBanner->SetColorAndOpacity(VTTheme::Gold);
		}
		else if (bHasActiveBuff)
		{
			FString BuffStr;
			if (GM->MagnetFrames > 0) BuffStr += FString::Printf(TEXT("[MAGNET %.1fs] "), float(GM->MagnetFrames) / 60.0f);
			if (GM->FreezeFrames > 0) BuffStr += FString::Printf(TEXT("[FREEZE %.1fs] "), float(GM->FreezeFrames) / 60.0f);
			if (GM->ShieldCount > 0)  BuffStr += TEXT("[AEGIS SHIELD AKTIV]");
			TxtPowerUpBanner->SetText(FText::FromString(BuffStr));
			TxtPowerUpBanner->SetColorAndOpacity(VTTheme::Cyan);
		}
	}
	if (AnnouncerBannerBorder.IsValid() && TxtAnnouncerCallout.IsValid())
	{
		const bool bVoiceOn = GM->ActiveSaveGame ? GM->ActiveSaveGame->bVoiceEnabled : true;
		AnnouncerBannerBorder->SetVisibility((bVoiceOn && GM->AnnouncerTimerSec > 0.0f) ? EVisibility::Visible : EVisibility::Collapsed);
		if (GM->AnnouncerTimerSec > 0.0f)
		{
			TxtAnnouncerCallout->SetText(FText::FromString(FString::Printf(TEXT("%s [STIMMEN %d/24]"),
				*GM->LatestAnnouncerCallout, GM->GetUnlockedVoiceCount())));
		}
	}

	// PROMINENT MOBILE COMBO & ITEM-MULTIPLIER PILL UPDATE (Strictly inside the 9:16 phone screen!)
	if (ComboHudBorder.IsValid() && TxtComboMult.IsValid())
	{
		const bool bShowInGameCombo = bBootReady && (GM->TowerRunState == EVTGameState::Play) && (GM->CurrentCombo >= 2);
		ComboHudBorder->SetVisibility(bShowInGameCombo ? EVisibility::Visible : EVisibility::Collapsed);
		TxtComboMult->SetText(FText::FromString(FString::Printf(TEXT("%dx"), GM->CurrentCombo)));

		FLinearColor ComboCol = VTTheme::Cyan;
		FString TierTxt = TEXT("DOUBLE JUMP!");
		if (GM->CurrentCombo >= 50)      { ComboCol = VTTheme::Gold;    TierTxt = TEXT("GODLIKE EMPEROR!"); }
		else if (GM->CurrentCombo >= 45) { ComboCol = VTTheme::Pink;    TierTxt = TEXT("LUDICROUS SPEED!"); }
		else if (GM->CurrentCombo >= 40) { ComboCol = VTTheme::Gold;    TierTxt = TEXT("MONSTER COMBO!"); }
		else if (GM->CurrentCombo >= 35) { ComboCol = VTTheme::Pink;    TierTxt = TEXT("ULTRA COMBO!"); }
		else if (GM->CurrentCombo >= 30) { ComboCol = VTTheme::Gold;    TierTxt = TEXT("MULTI COMBO!"); }
		else if (GM->CurrentCombo >= 26) { ComboCol = VTTheme::Pink;    TierTxt = TEXT("RAMPAGE!"); }
		else if (GM->CurrentCombo >= 23) { ComboCol = VTTheme::Emerald; TierTxt = TEXT("DOMINATING!"); }
		else if (GM->CurrentCombo >= 20) { ComboCol = VTTheme::Gold;    TierTxt = TEXT("UNSTOPPABLE!"); }
		else if (GM->CurrentCombo >= 18) { ComboCol = VTTheme::Pink;    TierTxt = TEXT("UNBELIEVABLE!"); }
		else if (GM->CurrentCombo >= 16) { ComboCol = VTTheme::Cyan;    TierTxt = TEXT("SPECTACULAR!"); }
		else if (GM->CurrentCombo >= 14) { ComboCol = VTTheme::Emerald; TierTxt = TEXT("INCREDIBLE!"); }
		else if (GM->CurrentCombo >= 12) { ComboCol = VTTheme::Gold;    TierTxt = TEXT("AMAZING!"); }
		else if (GM->CurrentCombo >= 10) { ComboCol = VTTheme::Gold;    TierTxt = TEXT("OUTSTANDING 2x!"); }
		else if (GM->CurrentCombo >= 8)  { ComboCol = VTTheme::Pink;    TierTxt = TEXT("EXCELLENT!"); }
		else if (GM->CurrentCombo >= 6)  { ComboCol = VTTheme::Emerald; TierTxt = TEXT("IMPRESSIVE!"); }
		else if (GM->CurrentCombo >= 5)  { ComboCol = VTTheme::Gold;    TierTxt = TEXT("SUPER COMBO!"); }
		else if (GM->CurrentCombo >= 4)  { ComboCol = VTTheme::Cyan;    TierTxt = TEXT("GREAT MOVE!"); }
		else if (GM->CurrentCombo >= 3)  { ComboCol = VTTheme::Emerald; TierTxt = TEXT("TRIPLE SWEET!"); }

		const FLinearColor PulsedCol = FMath::Lerp(ComboCol, FLinearColor::White, FMath::Clamp(GM->ComboHitPulse * 0.7f + GM->BeatPulse * 0.25f, 0.0f, 1.0f));
		ComboHudBorder->SetBorderBackgroundColor(PulsedCol);
		TxtComboMult->SetColorAndOpacity(PulsedCol);
		if (TxtComboTierLabel.IsValid())
		{
			TxtComboTierLabel->SetText(FText::FromString(FString::Printf(TEXT("%s // ITEM-PTS x%d"), *TierTxt, 1 + GM->CurrentCombo)));
			TxtComboTierLabel->SetColorAndOpacity(PulsedCol);
		}
		if (ComboFuseFillBox.IsValid())
		{
			const float FusePct = FMath::Clamp(float(GM->ComboTimerFrames) / 175.0f, 0.0f, 1.0f);
			ComboFuseFillBox->SetWidthOverride(FusePct * 108.0f);
		}
		if (ComboFuseFillBorder.IsValid())
		{
			ComboFuseFillBorder->SetBorderBackgroundColor(PulsedCol);
		}
	}

	// 5. Update Clean Guided Main Menu Labels
	const FVTClimberInfo EquippedInfo = GM->GetClimberInfo(GM->SelectedClimberId);
	if (TxtHomeLevelTitle.IsValid())
	{
		TxtHomeLevelTitle->SetText(FText::FromString(FString::Printf(TEXT("LVL %d"), GM->PlayerLevel)));
	}
	if (TxtHomeXpProgress.IsValid())
	{
		TxtHomeXpProgress->SetText(FText::FromString(FString::Printf(TEXT("%d / %d XP"), GM->CurrentXP, GM->XPToNextLevel)));
	}
	if (HomeXpFillBox.IsValid())
	{
		HomeXpFillBox->SetWidthOverride(FMath::Max(8.0f, XpRatio * 240.0f));
	}
	if (TxtHomeNickname.IsValid() && GM->ActiveSaveGame)
	{
		TxtHomeNickname->SetText(FText::FromString(FString::Printf(TEXT("%s (%s)"), *GM->ActiveSaveGame->PlayerNickname, *GM->GetPlayerRankTitle())));
	}
	if (TxtHomeAccountBadge.IsValid() && GM->ActiveSaveGame)
	{
		TxtHomeAccountBadge->SetText(FText::FromString(FString::Printf(TEXT("LOGIN: %s [WECHSELN]"), *GM->ActiveSaveGame->ActiveAuthProvider)));
	}
	if (TxtHomeCores.IsValid())
	{
		TxtHomeCores->SetText(FText::FromString(FString::Printf(TEXT("NEON CORES: %d [+]"), GM->NeonCores)));
	}
	if (TxtHomeRosterProgress.IsValid())
	{
		TxtHomeRosterProgress->SetText(FText::FromString(FString::Printf(TEXT("HELDEN: %d / %d"), GM->GetUnlockedClimberCount(), GM->Climbers.Num())));
	}
	if (TxtHomeThemeProgress.IsValid())
	{
		TxtHomeThemeProgress->SetText(FText::FromString(FString::Printf(TEXT("THEMES: %d / %d"), GM->GetUnlockedThemeCount(), GM->Themes.Num())));
	}
	if (TxtHomeBestRecord.IsValid() && GM->ActiveSaveGame)
	{
		TxtHomeBestRecord->SetText(FText::FromString(FString::Printf(TEXT("REKORD: ETAGE %d // %s PTS"),
			GM->ActiveSaveGame->MaxFloorRecord, *FText::AsNumber(GM->BestScore).ToString())));
	}
	if (TxtHomeEquippedSummary.IsValid())
	{
		const FVTGameThemeInfo CurrentTheme = GM->GetThemeInfo(GM->SelectedThemeId);
		TxtHomeEquippedSummary->SetText(FText::FromString(FString::Printf(TEXT("AKTIV: #%02d %s // %s // %s"),
			EquippedInfo.RosterNumber, *EquippedInfo.Name, *VTTheme::GetGameModeShortName(GM->CurrentGameMode), *CurrentTheme.Name)));
	}

	// 6. Update Game Over Stats + Live Post-Game Leaderboard Placement (Global & Friends!)
	if (GM->TowerRunState == EVTGameState::GameOver)
	{
		if (TxtOverBadge.IsValid()) TxtOverBadge->SetText(FText::FromString(GM->GameOverBadge));
		if (TxtOverSubtext.IsValid()) TxtOverSubtext->SetText(FText::FromString(GM->GameOverSubtext));
		if (TxtOverScore.IsValid()) TxtOverScore->SetText(FText::AsNumber(GM->CurrentScore));
		if (TxtOverFloor.IsValid()) TxtOverFloor->SetText(FText::AsNumber(GM->MaxFloor));
		if (TxtOverCombo.IsValid()) TxtOverCombo->SetText(FText::FromString(FString::Printf(TEXT("%dx"), GM->MaxCombo)));
		if (TxtOverBpm.IsValid()) TxtOverBpm->SetText(FText::FromString(FString::Printf(TEXT("%d BPM"), GM->PeakBpm)));

		if (LastBuiltLeaderboardScore != GM->CurrentScore || bLastBuiltFriendsMode != bLeaderboardFriendsOnly)
		{
			RebuildGameOverLeaderboard();
		}

		if (ReviveButtonContainer.IsValid())
		{
			const bool bCanRevive = !GM->bHasRevivedThisRun && GM->ReviveSecondsRemaining > 0.0f;
			ReviveButtonContainer->SetVisibility(bCanRevive ? EVisibility::Visible : EVisibility::Collapsed);
			if (bCanRevive && TxtReviveCountdown.IsValid())
			{
				TxtReviveCountdown->SetText(FText::FromString(FString::Printf(TEXT("Noch %.1fs fuer 2. Chance!"), GM->ReviveSecondsRemaining)));
			}
		}
	}

	// Simulated Ad Countdown
	if (ActiveModal == EVTModalOverlay::AdPlayer && AdCountdownRemaining > 0.0f)
	{
		AdCountdownRemaining -= InDeltaTime;
		if (AdCountdownRemaining <= 0.0f)
		{
			AdCountdownRemaining = 0.0f;
			CloseModal();
			if (PendingAdRewardCallback) PendingAdRewardCallback();
		}
		else
		{
			RebuildModalContent();
		}
	}
}

int32 UVTMainHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	AVTTowerGameMode* GM = GetTowerGM();
	AVTPlayerController* PC = GetTowerPC();

	// 1. Draw 1:1 Theme Frame (Top Header, Left/Right Outer Craft/Stone/Coral Borders + Music Meter, and Bottom Control Bar)
	if (GM && GM->BootFlowState == EVTBootFlowState::Ready && GM->TowerRunState == EVTGameState::Play && ActiveModal == EVTModalOverlay::None && !bHudDragEditMode)
	{
		const EVTGameThemeId ThemeId = GM->SelectedThemeId;
		if (const TSharedPtr<FSlateDynamicImageBrush>* FoundBrushPtr = ThemeFrameBrushes.Find(ThemeId))
		{
			if (FoundBrushPtr->IsValid())
			{
				const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
				if (LocalSize.X > 10.0f && LocalSize.Y > 10.0f)
				{
					const FSlateBrush* BaseBrush = FoundBrushPtr->Get();
					auto DrawUVSlice = [&](const FVector2D& Pos, const FVector2D& Size, const FVector2D& UV0, const FVector2D& UV1, int32 DrawLayer)
					{
						FSlateBrush SliceBrush = *BaseBrush;
						SliceBrush.SetUVRegion(FBox2D(UV0, UV1));
						FSlateDrawElement::MakeBox(
							OutDrawElements,
							DrawLayer,
							AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)),
							&SliceBrush,
							ESlateDrawEffect::None,
							FLinearColor::White
						);
					};

					const bool bHasDedicatedBottomBar =
						(ThemeId == EVTGameThemeId::PapercraftToybox ||
						 ThemeId == EVTGameThemeId::AncientRuins ||
						 ThemeId == EVTGameThemeId::DeepSea ||
						 ThemeId == EVTGameThemeId::BioLumenArk ||
						 ThemeId == EVTGameThemeId::RetroSynthwave);

					const float TopH = LocalSize.Y * 0.092f;
					const float BotY = LocalSize.Y * 0.884f;
					const float BotH = LocalSize.Y - BotY;
					const float MidH = BotY - TopH;
					const float LeftW = LocalSize.X * 0.112f;
					const float RightW = LocalSize.X * 0.132f;
					const float RightX = LocalSize.X - RightW;

					// 1A. Left & Right Outer Theme Borders + Right Volume/Music Meter Widget
					DrawUVSlice(FVector2D(0.0f, TopH), FVector2D(LeftW, MidH), FVector2D(0.0f, 0.092f), FVector2D(0.112f, 0.884f), LayerId);
					DrawUVSlice(FVector2D(RightX, TopH), FVector2D(RightW, MidH), FVector2D(0.868f, 0.092f), FVector2D(1.0f, 0.884f), LayerId);

					// 1B. Top 1:1 Theme Header Bar (SCORE / COMBO x25 / STAGE)
					DrawUVSlice(FVector2D(0.0f, 0.0f), FVector2D(LocalSize.X, TopH), FVector2D(0.0f, 0.0f), FVector2D(1.0f, 0.092f), LayerId + 1);

					// 1C. Bottom 1:1 Control Bar (CRAZY CLIMBER! | GLUE | JUMP | SHOP | START!)
					if (bHasDedicatedBottomBar)
					{
						DrawUVSlice(FVector2D(0.0f, BotY), FVector2D(LocalSize.X, BotH), FVector2D(0.0f, 0.884f), FVector2D(1.0f, 1.0f), LayerId + 1);
					}
					else
					{
						// For the 6 grid-reference themes, draw their bottom border strip + themed control buttons
						DrawUVSlice(FVector2D(0.0f, BotY), FVector2D(LocalSize.X, BotH), FVector2D(0.0f, 0.0f), FVector2D(1.0f, 0.092f), LayerId + 1);
						const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
						const FVTGameThemeInfo TInfo = GM->GetThemeInfo(ThemeId);
						FSlateDrawElement::MakeBox(
							OutDrawElements,
							LayerId + 2,
							AllottedGeometry.ToPaintGeometry(FVector2D(LocalSize.X, BotH), FSlateLayoutTransform(FVector2D(0.0f, BotY))),
							WhiteBrush,
							ESlateDrawEffect::None,
							FLinearColor(TInfo.WallBaseColor.R * 0.35f, TInfo.WallBaseColor.G * 0.35f, TInfo.WallBaseColor.B * 0.35f, 0.90f)
						);
						FSlateFontInfo BtnFont = VTTheme::Font(11, true);
						const FString BarText = TEXT("[ ◀ LINKS | RECHTS ▶ ]     [ GLUE ]     [ ▲ JUMP ]     [ SHOP ]     [ START! ]");
						FSlateDrawElement::MakeText(
							OutDrawElements,
							LayerId + 3,
							AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(LocalSize.X * 0.06f, BotY + BotH * 0.38f))),
							BarText,
							BtnFont,
							ESlateDrawEffect::None,
							TInfo.PrimaryColor
						);
					}

					// 1D. Live Dynamic Score / Stage / Combo Overlay when climbing (MaxFloor > 0)
					if (GM->MaxFloor > 0 || GM->CurrentCombo > 0)
					{
						const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");
						FSlateFontInfo HudFont = VTTheme::Font(11, true);
						HudFont.OutlineSettings.OutlineSize = 2;
						HudFont.OutlineSettings.OutlineColor = FLinearColor(0.08f, 0.05f, 0.02f, 0.95f);

						const FString LiveScoreStr = FString::Printf(TEXT("SCORE: %s"), *FText::AsNumber(GM->CurrentScore).ToString());
						const FString LiveStageStr = FString::Printf(TEXT("STAGE 1-%d (FLOOR %d)"), 1 + (GM->MaxFloor / 10), GM->MaxFloor);
						FSlateDrawElement::MakeBox(
							OutDrawElements,
							LayerId + 2,
							AllottedGeometry.ToPaintGeometry(FVector2D(LocalSize.X * 0.28f, 22.0f), FSlateLayoutTransform(FVector2D(LocalSize.X * 0.03f, TopH - 24.0f))),
							WhiteBrush,
							ESlateDrawEffect::None,
							FLinearColor(0.14f, 0.09f, 0.05f, 0.82f)
						);
						FSlateDrawElement::MakeText(
							OutDrawElements,
							LayerId + 3,
							AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(LocalSize.X * 0.045f, TopH - 21.0f))),
							LiveScoreStr,
							HudFont,
							ESlateDrawEffect::None,
							FLinearColor(1.0f, 0.95f, 0.78f, 1.0f)
						);

						FSlateDrawElement::MakeBox(
							OutDrawElements,
							LayerId + 2,
							AllottedGeometry.ToPaintGeometry(FVector2D(LocalSize.X * 0.30f, 22.0f), FSlateLayoutTransform(FVector2D(LocalSize.X * 0.67f, TopH - 24.0f))),
							WhiteBrush,
							ESlateDrawEffect::None,
							FLinearColor(0.14f, 0.09f, 0.05f, 0.82f)
						);
						FSlateDrawElement::MakeText(
							OutDrawElements,
							LayerId + 3,
							AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(LocalSize.X * 0.685f, TopH - 21.0f))),
							LiveStageStr,
							HudFont,
							ESlateDrawEffect::None,
							FLinearColor(1.0f, 0.95f, 0.78f, 1.0f)
						);
					}
				}
			}
		}
	}

	int32 MaxLayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 4, InWidgetStyle, bParentEnabled);
	if (!GM || !PC) return MaxLayerId;

	// Render Floating Score Texts in full 3D-to-2D projection!
	if (GM->FloatingTexts.Num() > 0)
	{
		for (const FVTFloatingTextEntry& Entry : GM->FloatingTexts)
		{
			if (Entry.Life <= 0.0f) continue;
			FVector2D WidgetPos;
			if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Entry.WorldPos, WidgetPos, false))
			{
				const FVector2D LocalPos = AllottedGeometry.AbsoluteToLocal(WidgetPos);
				const float Alpha = FMath::Clamp(Entry.Life * 1.6f, 0.0f, 1.0f);
				const float PopCurve = FMath::Sin(FMath::Clamp(1.0f - Entry.Life, 0.0f, 1.0f) * PI);
				const float Scale = 1.0f + PopCurve * 0.45f;
				const int32 ScaledFontSize = FMath::Clamp(FMath::RoundToInt(float(Entry.FontSize) * Scale), 12, 52);

				// Iconic High-Impact Arcade Font (Impact / Cyber Action Arcade Style)
				FSlateFontInfo ArcadeFont;
				const FString CustomFontPath = FPaths::ProjectContentDir() / TEXT("Fonts/ArcadeScoreFont.ttf");
				if (FPaths::FileExists(CustomFontPath))
				{
					ArcadeFont = FSlateFontInfo(CustomFontPath, ScaledFontSize);
				}
				else
				{
					ArcadeFont = FCoreStyle::GetDefaultFontStyle("Bold", ScaledFontSize);
				}
				// 3px deep solid dark outline for maximum arcade action readability against any background
				ArcadeFont.OutlineSettings.OutlineSize = 3;
				ArcadeFont.OutlineSettings.OutlineColor = FLinearColor(0.01f, 0.01f, 0.01f, Alpha * 0.98f);
				ArcadeFont.OutlineSettings.bApplyOutlineToDropShadows = true;

				// Centered text offset
				const float EstHalfWidth = float(Entry.Text.Len()) * float(ScaledFontSize) * 0.35f;
				const FVector2D CenteredPos = LocalPos - FVector2D(EstHalfWidth, float(ScaledFontSize) * 0.5f);

				// 1. Draw Arcade 3D Drop Shadow for deep dimensional pop (+3, +3)
				FSlateDrawElement::MakeText(
					OutDrawElements,
					MaxLayerId + 1,
					AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(CenteredPos + FVector2D(3.0f, 3.0f))),
					Entry.Text,
					ArcadeFont,
					ESlateDrawEffect::None,
					FLinearColor(0.0f, 0.0f, 0.0f, Alpha * 0.92f)
				);

				// 2. Draw Vibrant Glowing Neon Arcade Score Text
				FSlateDrawElement::MakeText(
					OutDrawElements,
					MaxLayerId + 2,
					AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(CenteredPos)),
					Entry.Text,
					ArcadeFont,
					ESlateDrawEffect::None,
					FLinearColor(Entry.Color.R, Entry.Color.G, Entry.Color.B, Alpha)
				);
			}
		}
		MaxLayerId += 3;
	}

	return MaxLayerId;
}

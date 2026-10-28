#include "VTTowerGameMode.h"
#include "VTClimberCharacter.h"
#include "VTPlatformActor.h"
#include "VTPlayerController.h"
#include "VTSaveGame.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"
#include "UnrealClient.h"

AVTTowerGameMode::AVTTowerGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AVTClimberCharacter::StaticClass();
	PlayerControllerClass = AVTPlayerController::StaticClass();

	InitializeStaticCatalogs();
}

void AVTTowerGameMode::InitializeStaticCatalogs()
{
	// 1. 6 Biomes matching HTML BIOMES 1:1
	Biomes.Empty();
	{
		FVTBiomeInfo B1;
		B1.BiomeIndex = 1;
		B1.Name = TEXT("CRYO NEON ICE");
		B1.TopColor = FLinearColor(0.0f, 0.961f, 0.831f);
		B1.BodyColor = FLinearColor(0.008f, 0.518f, 0.780f);
		B1.BgColor1 = FLinearColor(0.016f, 0.027f, 0.067f);
		B1.BgColor2 = FLinearColor(0.027f, 0.075f, 0.149f);
		Biomes.Add(B1);

		FVTBiomeInfo B2;
		B2.BiomeIndex = 2;
		B2.Name = TEXT("VICE SUNSET CHROME");
		B2.TopColor = FLinearColor(0.969f, 0.145f, 0.522f);
		B2.BodyColor = FLinearColor(0.494f, 0.133f, 0.808f);
		B2.BgColor1 = FLinearColor(0.035f, 0.016f, 0.078f);
		B2.BgColor2 = FLinearColor(0.118f, 0.035f, 0.169f);
		Biomes.Add(B2);

		FVTBiomeInfo B3;
		B3.BiomeIndex = 3;
		B3.Name = TEXT("ACID REACTOR CORE");
		B3.TopColor = FLinearColor(0.639f, 0.902f, 0.208f);
		B3.BodyColor = FLinearColor(0.082f, 0.502f, 0.239f);
		B3.BgColor1 = FLinearColor(0.016f, 0.063f, 0.031f);
		B3.BgColor2 = FLinearColor(0.039f, 0.141f, 0.078f);
		Biomes.Add(B3);

		FVTBiomeInfo B4;
		B4.BiomeIndex = 4;
		B4.Name = TEXT("SOLAR MAGMA OVERDRIVE");
		B4.TopColor = FLinearColor(1.0f, 0.745f, 0.043f);
		B4.BodyColor = FLinearColor(0.918f, 0.345f, 0.047f);
		B4.BgColor1 = FLinearColor(0.071f, 0.024f, 0.016f);
		B4.BgColor2 = FLinearColor(0.161f, 0.059f, 0.024f);
		Biomes.Add(B4);

		FVTBiomeInfo B5;
		B5.BiomeIndex = 5;
		B5.Name = TEXT("HOLO DEEP GLACIER");
		B5.TopColor = FLinearColor(0.220f, 0.741f, 0.973f);
		B5.BodyColor = FLinearColor(0.145f, 0.388f, 0.922f);
		B5.BgColor1 = FLinearColor(0.012f, 0.035f, 0.090f);
		B5.BgColor2 = FLinearColor(0.039f, 0.098f, 0.212f);
		Biomes.Add(B5);

		FVTBiomeInfo B6;
		B6.BiomeIndex = 6;
		B6.Name = TEXT("QUANTUM VOID MATRIX");
		B6.TopColor = FLinearColor(0.910f, 0.475f, 0.976f);
		B6.BodyColor = FLinearColor(0.388f, 0.400f, 0.945f);
		B6.BgColor1 = FLinearColor(0.020f, 0.012f, 0.051f);
		B6.BgColor2 = FLinearColor(0.075f, 0.043f, 0.161f);
		Biomes.Add(B6);
	}

	// 2. 20 Unlockable Climbers (Only #01 KID FLEX is unlocked initially; #02..#20 are unlocked via the In-Game Shop!)
	Climbers.Empty();
	{
		const FLinearColor ColStarter(0.0f, 0.961f, 0.831f, 1.0f);
		const FLinearColor ColRare(0.220f, 0.741f, 0.973f, 1.0f);
		const FLinearColor ColEpic(0.969f, 0.145f, 0.522f, 1.0f);
		const FLinearColor ColLegendary(1.0f, 0.745f, 0.043f, 1.0f);
		const FLinearColor ColMythic(0.680f, 0.320f, 1.0f, 1.0f);

		auto AddClimber = [this](
			EVTClimberId Id, int32 Num, const FString& Name, const FString& Rarity, const FLinearColor& RarCol,
			const FString& Style, const FString& Desc, int32 Bpm, int32 Cost,
			const FLinearColor& Hoodie, const FLinearColor& Pants, const FLinearColor& Shoes,
			const FLinearColor& Scarf, const FLinearColor& Visor)
		{
			FVTClimberInfo C;
			C.Id = Id;
			C.RosterNumber = Num;
			C.Name = Name;
			C.Rarity = Rarity;
			C.RarityColor = RarCol;
			C.Style = Style;
			C.Description = Desc;
			C.BaseBpm = Bpm;
			C.CostCores = Cost;
			C.HoodieColor = Hoodie;
			C.PantsColor = Pants;
			C.ShoesColor = Shoes;
			C.ScarfColor = Scarf;
			C.VisorColor = Visor;
			C.ScaleNotes = { 110.0f + float(Num) * 2.5f, 130.81f, 146.83f, 164.81f, 196.00f, 220.00f + float(Num) * 3.0f };
			Climbers.Add(C);
		};

		// #01 STARTER (FREE / UNLOCKED AT START)
		AddClimber(EVTClimberId::KidFlex, 1, TEXT("KID FLEX"), TEXT("STARTER"), ColStarter,
			TEXT("Streetwear // 120 BPM"), TEXT("Der originale Street-Runner von Neo-Vice.Von Anfang an freigeschaltet!"), 120, 0,
			FLinearColor(0.0f, 0.961f, 0.831f), FLinearColor(0.118f, 0.161f, 0.231f), FLinearColor(0.969f, 0.145f, 0.522f), FLinearColor(0.969f, 0.145f, 0.522f), FLinearColor(1.0f, 1.0f, 1.0f));

		// #02 - #05 RARE TIER
		AddClimber(EVTClimberId::DiscoDave, 2, TEXT("DISCO DAVE"), TEXT("RARE"), ColRare,
			TEXT("Funk Groove // 122 BPM"), TEXT("Goldener 80s-Synth-Groove mit leuchtendem Disco-Visor."), 122, 20,
			FLinearColor(1.0f, 0.745f, 0.043f), FLinearColor(0.231f, 0.027f, 0.392f), FLinearColor(0.0f, 0.961f, 0.831f), FLinearColor(0.220f, 0.741f, 0.973f), FLinearColor(0.996f, 0.941f, 0.541f));

		AddClimber(EVTClimberId::PixelPenny, 3, TEXT("PIXEL PENNY"), TEXT("RARE"), ColRare,
			TEXT("8-Bit Arcade // 123 BPM"), TEXT("Chiptune-Speedrunnerin aus den Untergrund-Spielhallen."), 123, 25,
			FLinearColor(1.0f, 0.35f, 0.78f), FLinearColor(0.12f, 0.08f, 0.28f), FLinearColor(0.0f, 0.95f, 1.0f), FLinearColor(1.0f, 0.92f, 0.15f), FLinearColor(0.0f, 1.0f, 0.85f));

		AddClimber(EVTClimberId::FrostRunner, 4, TEXT("FROST RUNNER"), TEXT("RARE"), ColRare,
			TEXT("Cryo Sprinter // 124 BPM"), TEXT("Sub-Zero Parkour-Spezialist mit eisblauer Thermo-Panzerung."), 124, 30,
			FLinearColor(0.18f, 0.68f, 1.0f), FLinearColor(0.06f, 0.14f, 0.28f), FLinearColor(0.85f, 0.96f, 1.0f), FLinearColor(0.0f, 0.95f, 1.0f), FLinearColor(0.75f, 0.95f, 1.0f));

		AddClimber(EVTClimberId::NeonVortex, 5, TEXT("NEON VORTEX"), TEXT("RARE"), ColRare,
			TEXT("Synth Rider // 124 BPM"), TEXT("Outrun-Highway-Drifter mit violett-tuerkisem Plasma-Mantel."), 124, 35,
			FLinearColor(0.52f, 0.14f, 0.92f), FLinearColor(0.08f, 0.05f, 0.18f), FLinearColor(0.0f, 0.96f, 0.83f), FLinearColor(0.97f, 0.15f, 0.52f), FLinearColor(0.0f, 0.96f, 0.83f));

		// #06 - #11 EPIC TIER
		AddClimber(EVTClimberId::CyberNinja, 6, TEXT("CYBER NINJA"), TEXT("EPIC"), ColEpic,
			TEXT("Shadow Shinobi // 126 BPM"), TEXT("Lautloser Klingenlaeufer des Arasaka-Distrikts."), 126, 45,
			FLinearColor(0.969f, 0.145f, 0.522f), FLinearColor(0.059f, 0.090f, 0.165f), FLinearColor(0.0f, 0.961f, 0.831f), FLinearColor(0.0f, 0.961f, 0.831f), FLinearColor(0.0f, 0.961f, 0.831f));

		AddClimber(EVTClimberId::AlpenKing, 7, TEXT("ALPEN KING"), TEXT("EPIC"), ColEpic,
			TEXT("Glacier Pro // 125 BPM"), TEXT("Legendaerer Extrem-Bergsteiger mit Titan-Steigeisen."), 125, 55,
			FLinearColor(0.220f, 0.741f, 0.973f), FLinearColor(0.200f, 0.255f, 0.333f), FLinearColor(0.980f, 0.800f, 0.082f), FLinearColor(0.937f, 0.267f, 0.267f), FLinearColor(0.220f, 0.741f, 0.973f));

		AddClimber(EVTClimberId::BlazeStriker, 8, TEXT("BLAZE STRIKER"), TEXT("EPIC"), ColEpic,
			TEXT("Solar Pyro // 127 BPM"), TEXT("Ueberschall-Sprinter mit ueberhitzten Reaktor-Sohlen."), 127, 65,
			FLinearColor(1.0f, 0.28f, 0.05f), FLinearColor(0.18f, 0.06f, 0.04f), FLinearColor(1.0f, 0.82f, 0.10f), FLinearColor(1.0f, 0.55f, 0.0f), FLinearColor(1.0f, 0.90f, 0.25f));

		AddClimber(EVTClimberId::ValkyrieVex, 9, TEXT("VALKYRIE VEX"), TEXT("EPIC"), ColEpic,
			TEXT("Sky Lancer // 128 BPM"), TEXT("Nordische Cyber-Walkuere mit gefluegeltem Holo-Helm."), 128, 75,
			FLinearColor(0.92f, 0.95f, 1.0f), FLinearColor(0.15f, 0.22f, 0.38f), FLinearColor(1.0f, 0.78f, 0.12f), FLinearColor(0.0f, 0.88f, 1.0f), FLinearColor(1.0f, 0.85f, 0.25f));

		AddClimber(EVTClimberId::ToxicViper, 10, TEXT("TOXIC VIPER"), TEXT("EPIC"), ColEpic,
			TEXT("Acid Hazmat // 128 BPM"), TEXT("Bio-Reaktor-Runnerin mit leuchtend gruener Venomschleppe."), 128, 85,
			FLinearColor(0.55f, 0.98f, 0.12f), FLinearColor(0.06f, 0.16f, 0.08f), FLinearColor(0.95f, 1.0f, 0.15f), FLinearColor(0.10f, 0.95f, 0.45f), FLinearColor(0.65f, 1.0f, 0.20f));

		AddClimber(EVTClimberId::AstroNova, 11, TEXT("ASTRO NOVA"), TEXT("EPIC"), ColEpic,
			TEXT("Zero-G Cosmonaut // 129 BPM"), TEXT("Orbitale Springerin mit reduziertem Schwerkraft-Gefuehl."), 129, 95,
			FLinearColor(0.96f, 0.96f, 0.98f), FLinearColor(0.12f, 0.14f, 0.22f), FLinearColor(1.0f, 0.42f, 0.08f), FLinearColor(0.22f, 0.68f, 1.0f), FLinearColor(1.0f, 0.72f, 0.18f));

		// #12 - #16 LEGENDARY TIER
		AddClimber(EVTClimberId::MechaTitan, 12, TEXT("MECHA TITAN"), TEXT("LEGENDARY"), ColLegendary,
			TEXT("Heavy Exo-Suit // 130 BPM"), TEXT("Gepanzerter Kampf-Mech mit hydraulischen Sprungduesen."), 130, 115,
			FLinearColor(0.28f, 0.34f, 0.42f), FLinearColor(0.12f, 0.15f, 0.20f), FLinearColor(1.0f, 0.65f, 0.05f), FLinearColor(0.0f, 0.92f, 1.0f), FLinearColor(1.0f, 0.22f, 0.15f));

		AddClimber(EVTClimberId::PhantomWraith, 13, TEXT("PHANTOM WRAITH"), TEXT("LEGENDARY"), ColLegendary,
			TEXT("Ghost Netrunner // 131 BPM"), TEXT("Phasen-Hacker aus dem Deep-Net mit spektraler Aura."), 131, 130,
			FLinearColor(0.08f, 0.10f, 0.16f), FLinearColor(0.04f, 0.05f, 0.09f), FLinearColor(0.0f, 1.0f, 0.78f), FLinearColor(0.55f, 0.0f, 1.0f), FLinearColor(0.0f, 1.0f, 0.85f));

		AddClimber(EVTClimberId::RoninZero, 14, TEXT("RONIN ZERO"), TEXT("LEGENDARY"), ColLegendary,
			TEXT("Cyber Samurai // 132 BPM"), TEXT("Meisterloser Klingen-Virtuose mit blutrotem Plasma-Schal."), 132, 145,
			FLinearColor(0.88f, 0.08f, 0.18f), FLinearColor(0.08f, 0.08f, 0.12f), FLinearColor(1.0f, 0.80f, 0.15f), FLinearColor(1.0f, 0.08f, 0.22f), FLinearColor(1.0f, 0.92f, 0.60f));

		AddClimber(EVTClimberId::ChromeSiren, 15, TEXT("CHROME SIREN"), TEXT("LEGENDARY"), ColLegendary,
			TEXT("Liquid Chrome Diva // 133 BPM"), TEXT("Spiegelglatte Chrom-Ikone mit hypnotischem Synth-Pop-Beat."), 133, 165,
			FLinearColor(0.82f, 0.88f, 0.96f), FLinearColor(0.24f, 0.28f, 0.38f), FLinearColor(0.98f, 0.18f, 0.65f), FLinearColor(0.0f, 0.95f, 1.0f), FLinearColor(1.0f, 0.35f, 0.82f));

		AddClimber(EVTClimberId::ThunderVolt, 16, TEXT("THUNDER VOLT"), TEXT("LEGENDARY"), ColLegendary,
			TEXT("Tesla Overcharger // 134 BPM"), TEXT("Lebender Hochspannungs-Blitzleiter mit 10.000-Volt-Schuhen."), 134, 185,
			FLinearColor(1.0f, 0.88f, 0.08f), FLinearColor(0.10f, 0.18f, 0.36f), FLinearColor(0.0f, 0.92f, 1.0f), FLinearColor(1.0f, 0.95f, 0.35f), FLinearColor(0.45f, 0.95f, 1.0f));

		// #17 - #20 MYTHIC TIER
		AddClimber(EVTClimberId::MagmaRex, 17, TEXT("MAGMA REX"), TEXT("MYTHIC"), ColMythic,
			TEXT("Obsidian Dragonlord // 135 BPM"), TEXT("Geschmiedet im vulkanischen Reaktorkern von Etage 500."), 135, 210,
			FLinearColor(0.95f, 0.22f, 0.02f), FLinearColor(0.09f, 0.04f, 0.04f), FLinearColor(1.0f, 0.72f, 0.0f), FLinearColor(1.0f, 0.38f, 0.0f), FLinearColor(1.0f, 0.95f, 0.20f));

		AddClimber(EVTClimberId::CelestialOracle, 18, TEXT("CELESTIAL ORACLE"), TEXT("MYTHIC"), ColMythic,
			TEXT("Starlight Seraph // 136 BPM"), TEXT("Astrales Wesen aus reinem Sternenlicht und Gold-Aether."), 136, 240,
			FLinearColor(1.0f, 0.92f, 0.68f), FLinearColor(0.18f, 0.12f, 0.36f), FLinearColor(1.0f, 0.82f, 0.22f), FLinearColor(0.45f, 0.92f, 1.0f), FLinearColor(1.0f, 1.0f, 1.0f));

		AddClimber(EVTClimberId::QuantumGlitch, 19, TEXT("QUANTUM GLITCH"), TEXT("MYTHIC"), ColMythic,
			TEXT("Singularity Shifter // 138 BPM"), TEXT("Existiert gleichzeitig in zwei Dimensionen des Eis-Turms."), 138, 270,
			FLinearColor(0.75f, 0.15f, 1.0f), FLinearColor(0.04f, 0.02f, 0.12f), FLinearColor(0.0f, 1.0f, 0.85f), FLinearColor(1.0f, 0.12f, 0.68f), FLinearColor(0.0f, 1.0f, 0.95f));

		AddClimber(EVTClimberId::ApexEmperor, 20, TEXT("APEX EMPEROR"), TEXT("MYTHIC"), ColMythic,
			TEXT("Grandmaster // 140 BPM"), TEXT("Der unangefochtene Herrscher des Velocity Towers. Ultimatives Prestige!"), 140, 300,
			FLinearColor(1.0f, 0.82f, 0.12f), FLinearColor(0.06f, 0.04f, 0.14f), FLinearColor(1.0f, 0.15f, 0.48f), FLinearColor(0.0f, 0.98f, 0.88f), FLinearColor(1.0f, 0.96f, 0.65f));
	}

	// 3. 5 Ghost Trails matching HTML TRAILS 1:1
	Trails.Empty();
	{
		auto AddTrail = [this](const FString& Id, const FString& Name, const FLinearColor& Col, int32 Cost, const FString& Desc)
		{
			FVTTrailInfo T;
			T.Id = Id;
			T.Name = Name;
			T.Color = Col;
			T.CostCores = Cost;
			T.Description = Desc;
			Trails.Add(T);
		};
		AddTrail(TEXT("cyan"), TEXT("CYAN ICE GLOW"), FLinearColor(0.0f, 0.961f, 0.831f), 0, TEXT("Klassische Neon-Cyan Spur"));
		AddTrail(TEXT("flame"), TEXT("NEON FLAME"), FLinearColor(0.969f, 0.145f, 0.522f), 35, TEXT("Feurige Cyber-Flammenspur bei Saltos"));
		AddTrail(TEXT("matrix"), TEXT("MATRIX CODE"), FLinearColor(0.639f, 0.902f, 0.208f), 50, TEXT("Digitaler Acid-Matrix Partikelregen"));
		AddTrail(TEXT("gold"), TEXT("GOLD SPARKLE"), FLinearColor(1.0f, 0.820f, 0.400f), 75, TEXT("Schwebende glaenzende Gold-Partikel"));
		AddTrail(TEXT("rainbow"), TEXT("RAINBOW APEX"), FLinearColor(0.925f, 0.282f, 0.600f), 100, TEXT("Farbwechselnde Regenbogen-Spur"));
	}

	// 4. 12 Trophies / Achievements matching HTML ACHIEVEMENTS 1:1
	Achievements.Empty();
	{
		auto AddAch = [this](const FString& Id, const FString& Title, const FString& Desc, const FString& Icon, int32 Reward)
		{
			FVTAchievement A;
			A.Id = Id;
			A.Title = Title;
			A.Description = Desc;
			A.Icon = Icon;
			A.RewardCores = Reward;
			Achievements.Add(A);
		};
		AddAch(TEXT("first_steps"), TEXT("Erster Aufstieg"), TEXT("Erreiche Etage 25"), TEXT("STAR"), 15);
		AddAch(TEXT("combo_master"), TEXT("Combo Meister"), TEXT("Erziele eine 10x Combo"), TEXT("ZAP"), 20);
		AddAch(TEXT("wall_runner"), TEXT("Apex Wandlaeufer"), TEXT("Fuehre einen Super Ricochet aus"), TEXT("WALL"), 20);
		AddAch(TEXT("century"), TEXT("Hundert-Club"), TEXT("Erreiche Etage 100"), TEXT("CROWN"), 35);
		AddAch(TEXT("stratosphere"), TEXT("Stratosphaere"), TEXT("Erreiche Etage 200"), TEXT("ROCKET"), 50);
		AddAch(TEXT("ludicrous"), TEXT("Ludicrous Speed"), TEXT("Erziele eine 25x Mega-Combo"), TEXT("TORNADO"), 50);
		AddAch(TEXT("magnetar"), TEXT("Magnetar"), TEXT("Sammle 15 Cores im Turm"), TEXT("MAGNET"), 20);
		AddAch(TEXT("spring_master"), TEXT("Super Sprungfeder"), TEXT("Nutze die Spring Boots 3x"), TEXT("BOOT"), 15);
		AddAch(TEXT("shield_survivor"), TEXT("Unsterblich"), TEXT("Ueberlebe durch den Aegis-Schild"), TEXT("SHIELD"), 25);
		AddAch(TEXT("lucky_spinner"), TEXT("Gluecksritter"), TEXT("Drehe das Neon Gluecksrad"), TEXT("WHEEL"), 15);
		AddAch(TEXT("rival_slayer"), TEXT("Rivalen-Bezwinger"), TEXT("Gewinne ein 1v1-Freunde-Duell"), TEXT("SWORD"), 30);
		AddAch(TEXT("core_tycoon"), TEXT("Core-Millionaer"), TEXT("Sammle insgesamt 100 Neon Cores"), TEXT("GEM"), 50);
	}

	// 5. All 10 Distinct 1:1 Game Themes based on Levelreferenzen (All unlocked & free!)
	Themes.Empty();
	{
		auto AddTheme = [this](EVTGameThemeId Id, int32 Num, const FString& Name, const FString& Subtitle, const FString& Desc,
			const FLinearColor& Prim, const FLinearColor& Sec, const FLinearColor& WallBase, const FLinearColor& WallEmiss,
			const FLinearColor& LightCol, float LightInt, const FLinearColor& EqCol, int32 Cost, bool bUnlocked)
		{
			FVTGameThemeInfo T;
			T.Id = Id;
			T.ThemeNumber = Num;
			T.Name = Name;
			T.Subtitle = Subtitle;
			T.Description = Desc;
			T.PrimaryColor = Prim;
			T.SecondaryColor = Sec;
			T.WallBaseColor = WallBase;
			T.WallEmissiveColor = WallEmiss;
			T.LightColor = LightCol;
			T.LightIntensity = LightInt;
			T.EqualizerColor = EqCol;
			T.CostCores = Cost;
			T.bUnlockedInitially = bUnlocked;
			Themes.Add(T);
		};

		// 1. Hand-Crafted Papercraft & Toybox (1:1 Vorlage Mobile_game_papercraft_UI_theme_20261027193244)
		AddTheme(EVTGameThemeId::PapercraftToybox, 1, TEXT("HAND-CRAFTED PAPERCRAFT"), TEXT("Crafted // Cardboard & Popsicle Sticks"),
			TEXT("1:1 Bastel-Turm im offenen Pappkarton mit bunten Holz-Eisstielen, Xylophon, Stern-Post-It, Tape-Notizzettel und gestrickter Stoffpuppe."),
			FLinearColor(0.961f, 0.620f, 0.043f), FLinearColor(0.925f, 0.282f, 0.600f), FLinearColor(0.64f, 0.45f, 0.27f),
			FLinearColor(0.76f, 0.56f, 0.35f), FLinearColor(1.0f, 0.95f, 0.84f), 22000.0f, FLinearColor(0.961f, 0.620f, 0.043f), 0, true);

		// 2. Ancient Mystical Ruins (1:1 Vorlage Ancient_fantasy_ruins_game_interface)
		AddTheme(EVTGameThemeId::AncientRuins, 2, TEXT("ANCIENT MYSTICAL RUINS"), TEXT("Fantasy // Overgrown Temple"),
			TEXT("1:1 Uralte bemooste Tempelruinen mit leuchtenden Runen, Steintreppen, Fackelschein, Magie-Bändern und kleinem Magier."),
			FLinearColor(0.220f, 0.741f, 0.973f), FLinearColor(0.290f, 0.871f, 0.502f), FLinearColor(0.14f, 0.18f, 0.20f),
			FLinearColor(0.220f, 0.741f, 0.973f), FLinearColor(0.45f, 0.85f, 1.0f), 19500.0f, FLinearColor(0.290f, 0.871f, 0.502f), 0, true);

		// 3. Bioluminescent Deep Sea (1:1 Vorlage Deep_sea_mobile_game_interface)
		AddTheme(EVTGameThemeId::DeepSea, 3, TEXT("BIOLUMINESCENT DEEP SEA"), TEXT("Aquatic // Oceanic Trench"),
			TEXT("1:1 Tiefsee-Graben mit leuchtenden Riesenquallen, Korallenriffen, Kelp-Algen, Luftblasen und Taucher."),
			FLinearColor(0.0f, 0.961f, 0.831f), FLinearColor(0.012f, 0.412f, 0.631f), FLinearColor(0.03f, 0.14f, 0.24f),
			FLinearColor(0.0f, 0.961f, 0.831f), FLinearColor(0.15f, 0.90f, 0.85f), 19500.0f, FLinearColor(0.0f, 0.961f, 0.831f), 0, true);

		// 4. Retro Synthwave 80s (1:1 Vorlage Outrun Grid Sunset)
		AddTheme(EVTGameThemeId::RetroSynthwave, 4, TEXT("RETRO SYNTHWAVE 80S"), TEXT("Outrun // Grid Sunset"),
			TEXT("1:1 80er-Jahre Wireframe-Gitter, grelle Magenta- & Cyan-Neon-Balken, Equalizer-Treppe und glühende Outrun-Sonne."),
			FLinearColor(0.969f, 0.145f, 0.522f), FLinearColor(0.0f, 0.95f, 1.0f), FLinearColor(0.08f, 0.02f, 0.16f),
			FLinearColor(0.969f, 0.145f, 0.522f), FLinearColor(0.97f, 0.25f, 0.62f), 20500.0f, FLinearColor(0.969f, 0.145f, 0.522f), 0, true);

		// 5. Candy Land Dessert (1:1 Vorlage Sweet Kingdom)
		AddTheme(EVTGameThemeId::CandyLand, 5, TEXT("CANDY LAND DESSERT"), TEXT("Sugar Apex // Sweet Kingdom"),
			TEXT("1:1 Zuckerwatte- und Waffelturm mit gestreiften Zuckerstangen, Marshmallow-Böden, Zuckerguss-Keksen und rosa Teddybär."),
			FLinearColor(0.957f, 0.247f, 0.369f), FLinearColor(0.984f, 0.808f, 0.910f), FLinearColor(0.68f, 0.38f, 0.52f),
			FLinearColor(0.98f, 0.45f, 0.68f), FLinearColor(1.0f, 0.90f, 0.94f), 21500.0f, FLinearColor(0.957f, 0.247f, 0.369f), 0, true);

		// 6. Industrial Forge & Lava (1:1 Vorlage Molten Iron)
		AddTheme(EVTGameThemeId::IndustrialForge, 6, TEXT("INDUSTRIAL FORGE & LAVA"), TEXT("Heavy Metal // Molten Iron"),
			TEXT("1:1 Massive Gusseisen-Pfeiler, rotglühende Stahlgitter, rotierende Zahnräder, Dampfrohre und brodelnde Magma."),
			FLinearColor(0.918f, 0.345f, 0.047f), FLinearColor(0.980f, 0.800f, 0.082f), FLinearColor(0.14f, 0.12f, 0.11f),
			FLinearColor(0.918f, 0.345f, 0.047f), FLinearColor(1.0f, 0.48f, 0.12f), 21500.0f, FLinearColor(0.918f, 0.345f, 0.047f), 0, true);

		// 7. Deep Space Orbit (1:1 Vorlage Cosmic Starfield)
		AddTheme(EVTGameThemeId::DeepSpaceOrbit, 7, TEXT("DEEP SPACE ORBIT"), TEXT("Cosmic // Orbital Station"),
			TEXT("1:1 Orbitale Raumstation mit Ringplanet, Nebelfeldern, Ionen-Triebwerken und holografischen Solar-Brücken."),
			FLinearColor(0.220f, 0.741f, 0.973f), FLinearColor(0.506f, 0.549f, 0.973f), FLinearColor(0.06f, 0.09f, 0.16f),
			FLinearColor(0.220f, 0.741f, 0.973f), FLinearColor(0.55f, 0.80f, 1.0f), 20000.0f, FLinearColor(0.506f, 0.549f, 0.973f), 0, true);

		// 8. 8-Bit Pixel Dungeon (1:1 Vorlage Voxel Castle)
		AddTheme(EVTGameThemeId::PixelDungeon, 8, TEXT("8-BIT PIXEL DUNGEON"), TEXT("Retro Arcade // Voxel Keep"),
			TEXT("1:1 8-Bit Voxel-Kerker mit bemoosten Pixel-Steinblöcken, Holzbalken, Eisenketten, Pixel-Fackeln und Voxel-Held."),
			FLinearColor(0.35f, 0.88f, 0.32f), FLinearColor(0.98f, 0.62f, 0.12f), FLinearColor(0.16f, 0.16f, 0.24f),
			FLinearColor(0.98f, 0.62f, 0.12f), FLinearColor(1.0f, 0.82f, 0.55f), 20000.0f, FLinearColor(0.35f, 0.88f, 0.32f), 0, true);

		// 9. Whimsical Toybox (1:1 Vorlage LEGO & Rulers)
		AddTheme(EVTGameThemeId::WhimsicalToybox, 9, TEXT("WHIMSICAL TOYBOX"), TEXT("Playroom // LEGO & Rulers"),
			TEXT("1:1 Kinderzimmer-Kartonschacht mit gelben Holz-Linealen, bunten LEGO-Steinen mit Noppen, Wellpappe und Karton-Roboter."),
			FLinearColor(0.95f, 0.78f, 0.10f), FLinearColor(0.90f, 0.16f, 0.15f), FLinearColor(0.62f, 0.44f, 0.26f),
			FLinearColor(0.95f, 0.78f, 0.10f), FLinearColor(1.0f, 0.94f, 0.82f), 21500.0f, FLinearColor(0.14f, 0.42f, 0.86f), 0, true);

		// 10. Cyberpunk Metropolis (1:1 Vorlage Neon Skyscrapers)
		AddTheme(EVTGameThemeId::CyberpunkDefault, 10, TEXT("CYBERPUNK METROPOLIS"), TEXT("Apex Core // Neon Skyscrapers"),
			TEXT("1:1 Dunkle Cyberpunk-Megastruktur mit leuchtenden Hologramm-Brücken, Stahlträgern, Kanji-Reklamen und fliegenden Autos."),
			FLinearColor(0.0f, 0.961f, 0.831f), FLinearColor(0.02f, 0.52f, 0.78f), FLinearColor(0.04f, 0.07f, 0.15f),
			FLinearColor(0.0f, 0.961f, 0.831f), FLinearColor(0.0f, 0.961f, 0.831f), 19500.0f, FLinearColor(0.0f, 0.961f, 0.831f), 0, true);

		// 11. Bio-Lumen Xenoverse Ark (1:1 Vorlage ref_08_bio_lumen_xenoverse_ark_1793214803988)
		AddTheme(EVTGameThemeId::BioLumenArk, 11, TEXT("BIO-LUMEN XENOVERSE ARK"), TEXT("Organic Sci-Fi // Alien Flora Ark"),
			TEXT("1:1 Organische Raumschiff-Arche mit Wand-Terrarien, leuchtenden Alien-Pflanzen, Ionen-Triebwerken, Cyan- & Violett-Hologrammbrücken mit Ranken und Ringplanet im All."),
			FLinearColor(0.0f, 0.95f, 1.0f), FLinearColor(0.78f, 0.35f, 1.0f), FLinearColor(0.08f, 0.12f, 0.18f),
			FLinearColor(0.0f, 0.95f, 1.0f), FLinearColor(0.12f, 0.92f, 1.0f), 21000.0f, FLinearColor(0.0f, 0.95f, 1.0f), 0, true);
	}
}

void AVTTowerGameMode::BeginPlay()
{
	Super::BeginPlay();

	// In Standalone / Packaged EXE (!GIsEditor): Erzwinge beim Start echten Fenstermodus (9:16 Portrait) mit Titelleiste, Buttons & freier Groessenaenderung!
	if (!GIsEditor && GEngine)
	{
		if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
		{
			UserSettings->SetFullscreenMode(EWindowMode::Windowed);
			UserSettings->SetScreenResolution(FIntPoint(600, 1020));
			UserSettings->ConfirmVideoMode();
			UserSettings->ApplyResolutionSettings(false);
			UserSettings->SaveSettings();
		}
		FSystemResolution::RequestResolutionChange(600, 1020, EWindowMode::Windowed);
		if (GEngine->GameViewport)
		{
			TSharedPtr<SWindow> GameWindow = GEngine->GameViewport->GetWindow();
			if (GameWindow.IsValid())
			{
				GameWindow->SetWindowMode(EWindowMode::Windowed);
				GameWindow->SetSizingRule(ESizingRule::UserSized);
				GameWindow->ReshapeWindow(FVector2D(380.0f, 25.0f), FVector2D(600.0f, 1020.0f));
			}
		}
	}

	// Load or create SaveGame (V6 Clean 20-Character Roster Save)
	if (UGameplayStatics::DoesSaveGameExist(TEXT("VelocityTowerApexSaveV6"), 0))
	{
		ActiveSaveGame = Cast<UVTSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("VelocityTowerApexSaveV6"), 0));
	}
	if (!ActiveSaveGame)
	{
		ActiveSaveGame = Cast<UVTSaveGame>(UGameplayStatics::CreateSaveGameObject(UVTSaveGame::StaticClass()));
	}

	if (ActiveSaveGame)
	{
		// Ensure migration to the new 20-Character Roster where ONLY #01 KID FLEX is unlocked initially!
		if (!ActiveSaveGame->bMigratedTo20Roster)
		{
			ActiveSaveGame->UnlockedClimbers.Empty();
			ActiveSaveGame->UnlockedClimbers.AddUnique(EVTClimberId::KidFlex);
			ActiveSaveGame->SelectedClimber = EVTClimberId::KidFlex;
			ActiveSaveGame->bIsVip = false;
			ActiveSaveGame->ControlMode = EVTControlMode::VirtualJoystick;
			ActiveSaveGame->HudConfig.OpacityPct = 42;
			ActiveSaveGame->NeonCores = FMath::Max(65, ActiveSaveGame->NeonCores);
			ActiveSaveGame->bMigratedTo20Roster = true;
		}
		if (ActiveSaveGame->UnlockedClimbers.Num() == 0)
		{
			ActiveSaveGame->UnlockedClimbers.AddUnique(EVTClimberId::KidFlex);
		}
		if (!ActiveSaveGame->UnlockedClimbers.Contains(ActiveSaveGame->SelectedClimber) && !ActiveSaveGame->bIsVip)
		{
			ActiveSaveGame->SelectedClimber = EVTClimberId::KidFlex;
		}

		BestScore = ActiveSaveGame->BestScore;
		NeonCores = ActiveSaveGame->NeonCores;
		PlayerLevel = FMath::Max(1, ActiveSaveGame->PlayerLevel);
		CurrentXP = FMath::Max(0, ActiveSaveGame->PlayerXP);
		XPToNextLevel = 400 + (PlayerLevel - 1) * 200;
		CurrentControlMode = ActiveSaveGame->ControlMode;
		CurrentGameMode = ActiveSaveGame->GameMode;
		BaseSpeedRatio = ActiveSaveGame->BaseSpeedRatio;
		SelectedClimberId = ActiveSaveGame->SelectedClimber;
		bIsGamePaused = false;
	PreviewClimberId = SelectedClimberId;
		SelectedTrailId = ActiveSaveGame->SelectedTrail;

		for (const FVTGameThemeInfo& T : Themes)
		{
			ActiveSaveGame->UnlockedThemes.AddUnique(T.Id);
		}
		ActiveSaveGame->SelectedTheme = EVTGameThemeId::BioLumenArk;
		SelectedThemeId = EVTGameThemeId::BioLumenArk;
		PreviewThemeId = SelectedThemeId;

		StartFloor = ActiveSaveGame->StartFloor;

		for (FVTAchievement& Ach : Achievements)
		{
			Ach.bUnlocked = ActiveSaveGame->UnlockedAchievements.Contains(Ach.Id);
		}
	}

	// Ensure PIE / Editor audio is ALWAYS audible even if the PIE viewport loses window focus!
	FApp::SetUnfocusedVolumeMultiplier(1.0f);

	BootFlowState = EVTBootFlowState::Ready;
	BootTimerSec = 10.0f;

	BuildTowerEnvironment3D();
	InitializeAudioEngine();

	// Grab or spawn player climber pawn
	ClimberPawn = Cast<AVTClimberCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!ClimberPawn && GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ClimberPawn = GetWorld()->SpawnActor<AVTClimberCharacter>(AVTClimberCharacter::StaticClass(), FVector(0.0f, 0.0f, 44.0f), FRotator::ZeroRotator, SpawnParams);
	}

	// Setup Portrait 9:16 3D CineCamera
	if (GetWorld())
	{
		FActorSpawnParameters CamParams;
		CamParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		TowerCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FVector(0.0f, -960.0f, 420.0f), FRotator(4.0f, 90.0f, 0.0f), CamParams);
		if (TowerCamera && TowerCamera->GetCameraComponent())
		{
			UCameraComponent* CamComp = TowerCamera->GetCameraComponent();
			CamComp->SetFieldOfView(56.0f);
			CamComp->SetAspectRatio(9.0f / 16.0f);
			CamComp->SetConstraintAspectRatio(false); // Fill mobile/PIE portrait window cleanly without black bars

			// 1. Crystal-Clear AAA Exposure & Shaders: Eliminate milky haze completely!
			CamComp->PostProcessSettings.bOverride_AutoExposureMethod = true;
			CamComp->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
			CamComp->PostProcessSettings.bOverride_AutoExposureBias = true;
			CamComp->PostProcessSettings.AutoExposureBias = 0.15f;
			CamComp->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
			CamComp->PostProcessSettings.AutoExposureMinBrightness = 1.0f;
			CamComp->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
			CamComp->PostProcessSettings.AutoExposureMaxBrightness = 1.0f;

			// 2. High-Contrast Cyberpunk Grading (Deep OLED Blacks, Rich Neon Saturation)
			CamComp->PostProcessSettings.bOverride_ColorContrast = true;
			CamComp->PostProcessSettings.ColorContrast = FVector4(1.24f, 1.24f, 1.28f, 1.0f);
			CamComp->PostProcessSettings.bOverride_ColorGamma = true;
			CamComp->PostProcessSettings.ColorGamma = FVector4(0.96f, 0.96f, 0.96f, 1.0f);
			CamComp->PostProcessSettings.bOverride_ColorGain = true;
			CamComp->PostProcessSettings.ColorGain = FVector4(1.05f, 1.05f, 1.05f, 1.0f);
			CamComp->PostProcessSettings.bOverride_ColorSaturation = true;
			CamComp->PostProcessSettings.ColorSaturation = FVector4(1.20f, 1.20f, 1.24f, 1.0f);

			// 3. Strict Bloom Threshold: ONLY true neon lights bloom! Never dark surfaces or characters!
			CamComp->PostProcessSettings.bOverride_BloomMethod = true;
			CamComp->PostProcessSettings.BloomMethod = EBloomMethod::BM_SOG;
			CamComp->PostProcessSettings.bOverride_BloomThreshold = true;
			CamComp->PostProcessSettings.BloomThreshold = 1.25f;
			CamComp->PostProcessSettings.bOverride_BloomIntensity = true;
			CamComp->PostProcessSettings.BloomIntensity = 0.40f;

			// 4. Eliminate Chromatic Aberration Milkiness & Washed-out Ambient
			CamComp->PostProcessSettings.bOverride_SceneFringeIntensity = true;
			CamComp->PostProcessSettings.SceneFringeIntensity = 0.0f;
			CamComp->PostProcessSettings.bOverride_IndirectLightingIntensity = true;
			CamComp->PostProcessSettings.IndirectLightingIntensity = 0.20f;
			CamComp->PostProcessSettings.bOverride_VignetteIntensity = true;
			CamComp->PostProcessSettings.VignetteIntensity = 0.38f;
		}

		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PC->bAutoManageActiveCameraTarget = false;
			PC->SetViewTarget(TowerCamera);
			PC->ConsoleCommand(TEXT("r.Lumen.TraceMeshSDFs 0"), false);
			PC->ConsoleCommand(TEXT("r.DefaultFeature.AutoExposure 0"), false);
			PC->ConsoleCommand(TEXT("r.EyeAdaptationQuality 0"), false);
			PC->ConsoleCommand(TEXT("r.Bloom.Threshold 1.25"), false);
			PC->ConsoleCommand(TEXT("r.Tonemapper.Sharpen 0.85"), false);
			PC->ConsoleCommand(TEXT("r.TonemapperFilm 1"), false);
			PC->ConsoleCommand(TEXT("DISABLEALLSCREENMESSAGES"), false);
			PC->ConsoleCommand(TEXT("au.SetAudioMuted 0"), false);
		}
	}

	// Launch directly into the 1:1 playable Level (Papercraft default, all 10 worlds switchable live!)
	StartNewRun();
	TowerRunState = EVTGameState::Play;
}

void AVTTowerGameMode::BuildTowerEnvironment3D()
{
	if (!GetWorld()) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	EnvironmentHostActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!EnvironmentHostActor) return;

	USceneComponent* Root = NewObject<USceneComponent>(EnvironmentHostActor, TEXT("EnvRoot"));
	EnvironmentHostActor->SetRootComponent(Root);
	Root->RegisterComponent();

	UStaticMesh* WallMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/VelocityTower/Meshes/SM_TowerWall_Segment.SM_TowerWall_Segment"));
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* IceShardMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/VelocityTower/Meshes/SM_IceChunk.SM_IceChunk"));

	UMaterialInterface* CustomWallMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VelocityTower/Materials/M_TowerMegastructure_Master.M_TowerMegastructure_Master"));
	UMaterialInterface* CustomNeonMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VelocityTower/Materials/M_NeonEmissive_Master.M_NeonEmissive_Master"));
	UMaterialInterface* FallbackMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	LeftWallMID = UMaterialInstanceDynamic::Create(CustomNeonMat ? CustomNeonMat : FallbackMat, this);
	LeftWallMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.961f, 0.831f));
	LeftWallMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.961f, 0.831f) * 6.0f);

	RightWallMID = UMaterialInstanceDynamic::Create(CustomNeonMat ? CustomNeonMat : FallbackMat, this);
	RightWallMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.961f, 0.831f));
	RightWallMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.961f, 0.831f) * 6.0f);

	LeftWallSegments.Empty();
	RightWallSegments.Empty();
	LeftWallPillars.Empty();
	RightWallPillars.Empty();
	BackWallPanels.Empty();

	// 1. Solid Continuous Tower Shaft: Left/Right Pillars, Neon Ribs & Continuous Megastructure Back-Wall
	for (int32 i = 0; i < 6; ++i)
	{
		// 1A. Solid Left Structural Pillar
		UStaticMeshComponent* LPillar = NewObject<UStaticMeshComponent>(EnvironmentHostActor, *FString::Printf(TEXT("LeftPillar_%d"), i));
		LPillar->SetupAttachment(Root);
		LPillar->SetStaticMesh(CubeMesh);
		LPillar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LPillar->SetWorldScale3D(FVector(0.48f, 2.2f, 4.05f));
		if (CustomWallMat) LPillar->SetMaterial(0, CustomWallMat);
		LPillar->RegisterComponent();
		LeftWallPillars.Add(LPillar);

		// 1B. Left Neon Accent Rib
		UStaticMeshComponent* LWall = NewObject<UStaticMeshComponent>(EnvironmentHostActor, *FString::Printf(TEXT("LeftWall_%d"), i));
		LWall->SetupAttachment(Root);
		LWall->SetStaticMesh(WallMesh ? WallMesh : CubeMesh);
		LWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LWall->SetWorldScale3D(FVector(1.0f, 1.4f, 1.4f));
		LWall->SetMaterial(0, LeftWallMID);
		LWall->RegisterComponent();
		LeftWallSegments.Add(LWall);

		// 1C. Solid Right Structural Pillar
		UStaticMeshComponent* RPillar = NewObject<UStaticMeshComponent>(EnvironmentHostActor, *FString::Printf(TEXT("RightPillar_%d"), i));
		RPillar->SetupAttachment(Root);
		RPillar->SetStaticMesh(CubeMesh);
		RPillar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RPillar->SetWorldScale3D(FVector(0.48f, 2.2f, 4.05f));
		if (CustomWallMat) RPillar->SetMaterial(0, CustomWallMat);
		RPillar->RegisterComponent();
		RightWallPillars.Add(RPillar);

		// 1D. Right Neon Accent Rib
		UStaticMeshComponent* RWall = NewObject<UStaticMeshComponent>(EnvironmentHostActor, *FString::Printf(TEXT("RightWall_%d"), i));
		RWall->SetupAttachment(Root);
		RWall->SetStaticMesh(WallMesh ? WallMesh : CubeMesh);
		RWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RWall->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
		RWall->SetWorldScale3D(FVector(1.0f, 1.4f, 1.4f));
		RWall->SetMaterial(0, RightWallMID);
		RWall->RegisterComponent();
		RightWallSegments.Add(RWall);

		// 1E. Solid Megastructure Tower Core Back-Plate (Eliminates pitch black void behind platforms!)
		UStaticMeshComponent* BackPanel = NewObject<UStaticMeshComponent>(EnvironmentHostActor, *FString::Printf(TEXT("BackWallPanel_%d"), i));
		BackPanel->SetupAttachment(Root);
		BackPanel->SetStaticMesh(CubeMesh);
		BackPanel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BackPanel->SetWorldScale3D(FVector(8.2f, 0.35f, 4.05f));
		if (CustomWallMat) BackPanel->SetMaterial(0, CustomWallMat);
		BackPanel->RegisterComponent();
		BackWallPanels.Add(BackPanel);
	}

	// 2. 16-Band 3D Audio-Reactive Equalizer Towers in the background shaft (Y = +220cm depth)
	for (int32 i = 0; i < 16; ++i)
	{
		UStaticMeshComponent* EqBar = NewObject<UStaticMeshComponent>(EnvironmentHostActor, *FString::Printf(TEXT("EqBar_%d"), i));
		EqBar->SetupAttachment(Root);
		EqBar->SetStaticMesh(CubeMesh);
		EqBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		EqBar->SetCastShadow(false);

		UMaterialInstanceDynamic* EqMID = UMaterialInstanceDynamic::Create(CustomNeonMat ? CustomNeonMat : FallbackMat, this);
		EqMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.961f, 0.831f));
		EqMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.961f, 0.831f) * 5.0f);
		EqBar->SetMaterial(0, EqMID);
		EqBar->RegisterComponent();

		EqualizerBarMeshes.Add(EqBar);
		EqualizerBarMIDs.Add(EqMID);
	}

	// 3. Deep 3D Cyberpunk Megastructure Background Pillars & Horizon Rings (Y = +450cm to +900cm depth for unmistakable 3D parallax!)
	for (int32 i = 0; i < 12; ++i)
	{
		UStaticMeshComponent* BgPillar = NewObject<UStaticMeshComponent>(EnvironmentHostActor, *FString::Printf(TEXT("BgMegastructure_%d"), i));
		BgPillar->SetupAttachment(Root);
		BgPillar->SetStaticMesh(WallMesh ? WallMesh : CubeMesh);
		BgPillar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (CustomWallMat) BgPillar->SetMaterial(0, CustomWallMat);
		BgPillar->RegisterComponent();
		BgMegastructurePillars.Add(BgPillar);
	}

	// 4. Pool of 28 3D Shattering Ice Shards
	for (int32 i = 0; i < 28; ++i)
	{
		FVTIceShardSim Shard;
		Shard.MeshComp = NewObject<UStaticMeshComponent>(EnvironmentHostActor, *FString::Printf(TEXT("IceShard3D_%d"), i));
		Shard.MeshComp->SetupAttachment(Root);
		Shard.MeshComp->SetStaticMesh(IceShardMesh ? IceShardMesh : CubeMesh);
		Shard.MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shard.MeshComp->SetVisibility(false);
		Shard.MID = UMaterialInstanceDynamic::Create(CustomNeonMat ? CustomNeonMat : FallbackMat, this);
		Shard.MeshComp->SetMaterial(0, Shard.MID);
		Shard.MeshComp->RegisterComponent();
		IceShardPool.Add(Shard);
	}

	// 5. Pool of 8 3D Shockwave Rings
	for (int32 i = 0; i < 8; ++i)
	{
		FVTShockwaveSim Sw;
		Sw.MeshComp = NewObject<UStaticMeshComponent>(EnvironmentHostActor, *FString::Printf(TEXT("Shockwave3D_%d"), i));
		Sw.MeshComp->SetupAttachment(Root);
		Sw.MeshComp->SetStaticMesh(CylinderMesh ? CylinderMesh : CubeMesh);
		Sw.MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Sw.MeshComp->SetVisibility(false);
		Sw.MID = UMaterialInstanceDynamic::Create(CustomNeonMat ? CustomNeonMat : FallbackMat, this);
		Sw.MeshComp->SetMaterial(0, Sw.MID);
		Sw.MeshComp->RegisterComponent();
		ShockwavePool.Add(Sw);
	}

	// 6. Dynamic Following Tower Point Light for Lumen bounce & specular highlights
	TowerMainLight = NewObject<UPointLightComponent>(EnvironmentHostActor, TEXT("TowerMainLight"));
	TowerMainLight->SetupAttachment(Root);
	TowerMainLight->SetIntensity(18000.0f);
	TowerMainLight->SetAttenuationRadius(1800.0f);
	TowerMainLight->SetLightColor(FLinearColor(0.0f, 0.961f, 0.831f));
	TowerMainLight->RegisterComponent();

	// Apply Initial Selected Game Theme
	ApplyThemeVisuals(GetThemeInfo(SelectedThemeId));
}

FVTBiomeInfo AVTTowerGameMode::GetBiomeForFloor(int32 Floor) const
{
	if (Biomes.Num() == 0) return FVTBiomeInfo();
	const int32 Idx = (FMath::Max(0, Floor) / 100) % Biomes.Num();
	return Biomes[Idx];
}

FVTFloorScaling AVTTowerGameMode::GetFloorScaling(int32 Floor) const
{
	FVTFloorScaling Out;
	const float ModeBoost = (CurrentGameMode == EVTGameModeType::Hyper || CurrentGameMode == EVTGameModeType::Overdrive2x) ? 1.25f : 1.0f;
	Out.ProgressiveMult = (1.0f + FMath::Min(0.50f, float(Floor) * 0.0010f)) * ModeBoost;
	Out.SpeedMult = Out.ProgressiveMult * BaseSpeedRatio * FMath::Clamp(AdminMoveSpeedScale, 0.35f, 3.0f);
	Out.WidthPct = FMath::Clamp(FMath::RoundToInt((100.0f - FMath::Min(60.0f, float(Floor) * 0.12f)) * AdminPlatformWidthScale), 35, 180);
	Out.CrackDuration = FMath::Max(0.68f, (1.35f - float(Floor) * 0.0010f) / (ModeBoost * FMath::Max(0.75f, BaseSpeedRatio)));
	return Out;
}

FVTClimberInfo AVTTowerGameMode::GetClimberInfo(EVTClimberId Id) const
{
	for (const FVTClimberInfo& C : Climbers)
	{
		if (C.Id == Id) return C;
	}
	return Climbers.Num() > 0 ? Climbers[0] : FVTClimberInfo();
}

FVTTrailInfo AVTTowerGameMode::GetTrailInfo(const FString& Id) const
{
	for (const FVTTrailInfo& T : Trails)
	{
		if (T.Id == Id) return T;
	}
	return Trails.Num() > 0 ? Trails[0] : FVTTrailInfo();
}

AVTPlatformActor* AVTTowerGameMode::SpawnPlatformForFloor(int32 Floor)
{
	if (!GetWorld()) return nullptr;

	const int32 RelFloor = Floor - StartFloor;
	const float Z = float(RelFloor) * FloorVerticalSpacingCm;
	const float InnerWidthCm = PlayfieldHalfWidthCm * 2.0f; // 688cm
	const bool bIsCheckpoint = (Floor == StartFloor) || (Floor % 50 == 0);
	const FVTFloorScaling Scaling = GetFloorScaling(Floor);
	const FVTBiomeInfo Biome = GetBiomeForFloor(Floor);

	float WidthCm = InnerWidthCm;
	float CenterX = 0.0f;
	float Vx = 0.0f;
	EVTPlatformType PlatType = EVTPlatformType::Normal;
	EVTItemType ItemType = EVTItemType::None;

	// 1:1 Reference 10-Floor Repeating Layout (Screen Left = +X, Screen Right = -X)
	// Matches Mobile_game_papercraft_UI_theme_20261027193244.jpg and all 9 other reference templates!
	static const float RefCenterX[10] = {
		-35.0f,  // Floor 0: Blue stick / Stone steps (Center-Right base)
		-125.0f, // Floor 1: Red-Orange stick (Right)
		 25.0f,  // Floor 2: Yellow stick with Blue Painter's Tape (Wide Center-Left bridge)
		-10.0f,  // Floor 3: Pink stick with cardboard end-spacers (Center)
		-15.0f,  // Floor 4: Green stick + 6-bar Xylophone / Equalizer Stairs / Jellyfish (Wide Center-Right)
		 135.0f, // Floor 5: Orange stick (Left)
		-120.0f, // Floor 6: Purple stick (Right)
		 130.0f, // Floor 7: Forest-Green stick (Left)
		 120.0f, // Floor 8: Golden-Yellow stick (Left)
		-15.0f   // Floor 9: Pink-Red stick (Top Center)
	};
	static const float RefWidthCm[10] = {
		430.0f,  // Floor 0
		280.0f,  // Floor 1
		460.0f,  // Floor 2
		310.0f,  // Floor 3
		490.0f,  // Floor 4
		275.0f,  // Floor 5
		285.0f,  // Floor 6
		265.0f,  // Floor 7
		275.0f,  // Floor 8
		280.0f   // Floor 9
	};

	const int32 PatternIdx = FMath::Abs(RelFloor) % 10;
	const float ScaleFactor = FMath::Clamp(float(Scaling.WidthPct) / 100.0f, 0.72f, 1.25f);
	WidthCm = RefWidthCm[PatternIdx] * ((Floor < StartFloor + 20) ? 1.0f : ScaleFactor);
	CenterX = RefCenterX[PatternIdx];

	if (bIsCheckpoint && Floor > StartFloor)
	{
		WidthCm = InnerWidthCm;
		CenterX = 0.0f;
		PlatType = EVTPlatformType::Checkpoint;
	}
	else if (Floor >= StartFloor + 12)
	{
		// Subtle horizontal variation & moving platforms on higher floors (12+)
		const float JitterX = FMath::Sin(float(Floor) * 1.7f) * 28.0f;
		const float SafeMargin = 16.0f;
		const float MaxAbsX = FMath::Max(0.0f, PlayfieldHalfWidthCm - WidthCm * 0.5f - SafeMargin);
		CenterX = FMath::Clamp(CenterX + JitterX, -MaxAbsX, MaxAbsX);

		if (Floor >= 80 && FMath::FRand() < FMath::Min(0.45f, float(Floor - 60) * 0.002f))
		{
			const float Dir = (FMath::FRand() < 0.5f) ? -1.0f : 1.0f;
			const float ModeMult = (CurrentGameMode == EVTGameModeType::Hyper) ? 1.25f : 1.0f;
			Vx = Dir * (0.9f + FMath::Min(2.4f, float(Floor) * 0.004f)) * ModeMult;
		}

		// Collectibles & Power-Ups on Floor 12+ so Floors 0..9 remain 100% pure 1:1 reference look
		if (FMath::FRand() < 0.14f)
		{
			ItemType = EVTItemType::Core;
		}
		else if (FMath::FRand() < 0.18f)
		{
			const int32 Pick = FMath::RandRange(0, 3);
			if (Pick == 0) ItemType = EVTItemType::Magnet;
			else if (Pick == 1) ItemType = EVTItemType::Spring;
			else if (Pick == 2) ItemType = EVTItemType::Freeze;
			else ItemType = EVTItemType::Shield;
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AVTPlatformActor* NewPlat = GetWorld()->SpawnActor<AVTPlatformActor>(AVTPlatformActor::StaticClass(), FVector(CenterX, 0.0f, Z), FRotator::ZeroRotator, SpawnParams);
	if (NewPlat)
	{
		NewPlat->InitializePlatform(Floor, CenterX, Z, WidthCm, Vx, PlatType, bIsCheckpoint, ItemType, Biome, Scaling.CrackDuration);
		NewPlat->ApplyTheme(GetThemeInfo(SelectedThemeId));
		Platforms.Add(NewPlat);
	}
	return NewPlat;
}

void AVTTowerGameMode::StartNewRun()
{
	// Destroy previous platforms
	for (AVTPlatformActor* P : Platforms)
	{
		if (P) P->Destroy();
	}
	Platforms.Empty();
	FloatingTexts.Empty();

	ApplyThemeVisuals(GetThemeInfo(SelectedThemeId));

	for (int32 i = 0; i < 34; ++i)
	{
		SpawnPlatformForFloor(StartFloor + i);
	}

	const FVTClimberInfo CInfo = GetClimberInfo(SelectedClimberId);
	const FVTTrailInfo TInfo = GetTrailInfo(SelectedTrailId);

	if (ClimberPawn)
	{
		ClimberPawn->ResetClimberForStart(-20.0f, 44.0f, StartFloor, CInfo, TInfo.Color);
		ClimberPawn->ApplyThemeCharacterCostume(SelectedThemeId);
	}

	CameraZ = 450.0f;
	BonusScoreAccumulated = 0;
	LastComboVoiceTier = 0;
	NearDeathVoiceCooldownSec = 0.0f;
	bNewHighScoreAnnouncedThisRun = false;
	bNewHighScoreAchieved = false;
	HighScoreCelebrationTimer = 0.0f;
	ScorePulseAnim = 0.0f;
	bIsDeathSequenceActive = false;
	DeathSequenceTimer = 0.0f;
	bDeadPopupShown = false;
	DeadPopupAlpha = 0.0f;
	CurrentSmoothedMusicPitch = 0.80f;
	TargetMusicPitch = 0.80f;
	ItemBonusTimerSec = 0.0f;
	CurrentScore = StartFloor * 25;
	MaxFloor = StartFloor;
	CurrentCombo = 0;
	MaxCombo = 0;
	ComboTimerFrames = 0;
	HurryLevel = 0;
	HurryTimerFrames = 0;
	HurryBannerTimerSec = 0.0f;
	BlitzFramesRemaining = 60 * 60;
	LeftWallFlash = 0.0f;
	RightWallFlash = 0.0f;
	ScreenShake = 0.0f;
	EnergyBarPercent = 0.0f;
	bHasRevivedThisRun = false;

	MagnetFrames = 0;
	SpringJumpsLeft = 0;
	FreezeFrames = 0;
	ShieldCount = 0;
	if (ClimberPawn) ClimberPawn->SetShieldVisualActive(false);

	CurrentBpm = CInfo.BaseBpm + FMath::FloorToInt(float(StartFloor) * 0.08f);
	PeakBpm = CurrentBpm;

	bInCharacterSelectStage = false;
	TowerRunState = EVTGameState::Play;
	bIsGamePaused = false;
	TriggerToast(TEXT("CLIMB NOW! NUTZE MOMENTUM & WALL-RICOCHETS!"));

	// Display clear High Score to beat right on Game Start!
	if (BestScore > 0 && ClimberPawn)
	{
		SpawnFloatingText(FString::Printf(TEXT("REKORD ZU SCHLAGEN: %s PTS"), *FText::AsNumber(BestScore).ToString()), 0.0f, ClimberPawn->PosZ + 90.0f, FLinearColor(1.0f, 0.82f, 0.15f), 18);
	}
}

void AVTTowerGameMode::ReturnToMainMenu()
{
	bInCharacterSelectStage = false;
	TowerRunState = EVTGameState::Menu;
	bIsGamePaused = false;
	PreviewClimberId = SelectedClimberId;
	if (ClimberPawn)
	{
		ClimberPawn->ApplyClimberVisuals(GetClimberInfo(SelectedClimberId), GetTrailInfo(SelectedTrailId).Color, false);
	}
	SaveGameToDisk();
}

void AVTTowerGameMode::ExecuteSecondChanceRevive()
{
	if (!ClimberPawn) return;

	TowerRunState = EVTGameState::Play;
	bIsGamePaused = false;
	bHasRevivedThisRun = true;

	// Find safe platform near MaxFloor
	AVTPlatformActor* TargetPlat = nullptr;
	for (int32 i = Platforms.Num() - 1; i >= 0; --i)
	{
		if (Platforms[i] && Platforms[i]->FloorIndex >= MaxFloor - 1)
		{
			TargetPlat = Platforms[i];
			break;
		}
	}
	if (!TargetPlat && Platforms.Num() > 0)
	{
		TargetPlat = Platforms.Last();
	}

	if (TargetPlat)
	{
		TargetPlat->bBroken = false;
		TargetPlat->bTouched = false;
		TargetPlat->CrackTime = 0.0f;
		TargetPlat->SetActorHiddenInGame(false);
		ClimberPawn->PosX = TargetPlat->PlatformX;
		ClimberPawn->PosZ = TargetPlat->PlatformZ + 64.0f;
	}

	ClimberPawn->VelX = 0.0f;
	ClimberPawn->VelZ = 18.5f; // Massive upward shockwave boost!
	ClimberPawn->bOnGround = false;
	ClimberPawn->bFlipping = true;
	CameraZ = ClimberPawn->PosZ + 120.0f;
	HurryLevel = FMath::Max(0, HurryLevel - 1);
	HurryTimerFrames = 0;

	SpawnShockwave3D(ClimberPawn->PosX, ClimberPawn->PosZ, 260.0f, FLinearColor(0.969f, 0.145f, 0.522f));
	SpawnShockwave3D(ClimberPawn->PosX, ClimberPawn->PosZ, 180.0f, FLinearColor(0.0f, 0.961f, 0.831f));
	SpawnFloatingText(TEXT("2ND CHANCE REVIVE!"), ClimberPawn->PosX, ClimberPawn->PosZ + 90.0f, FLinearColor(0.969f, 0.145f, 0.522f), 20);
}

void AVTTowerGameMode::TriggerAbyssFallDeath()
{
	if (bIsDeathSequenceActive || TowerRunState != EVTGameState::Play) return;

	bIsDeathSequenceActive = true;
	DeathSequenceTimer = 0.0f;
	bDeadPopupShown = false;
	DeadPopupAlpha = 0.0f;

	if (AVTPlayerController* PC = Cast<AVTPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->TriggerHapticFeedback(95);
		PC->JumpBufferFrames = 0;
		PC->bJumpHeld = false;
	}

	PlayTowerSFX(TEXT("FallScream"), 1.0f, 1.35f);
}

void AVTTowerGameMode::EndCurrentRun(const FString& InBadgeText, const FString& InReasonSubtext)
{
	TowerRunState = EVTGameState::GameOver;
	GameOverBadge = InBadgeText;
	GameOverSubtext = InReasonSubtext;
	ReviveSecondsRemaining = (!bHasRevivedThisRun && MaxFloor >= 3) ? 5.0f : 0.0f;

	// Clear any buffered Jump input so holding Jump on the controller never skips the Highscore/GameOver screen!
	if (AVTPlayerController* PC = Cast<AVTPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->JumpBufferFrames = 0;
		PC->bJumpHeld = false;
	}

	TriggerAnnouncerVoice(TEXT("VO_GameOver"), TEXT("WIPEOUT! GAME OVER!"));

	if (CurrentScore > BestScore)
	{
		BestScore = CurrentScore;
	}
	SaveGameToDisk();
}

void AVTTowerGameMode::HandleWallCollision(int32 SideDir, float MaxVx)
{
	if (!ClimberPawn) return;

	const float BoundaryX = float(SideDir) * (PlayfieldHalfWidthCm - ClimberPawn->RadiusCm);
	ClimberPawn->PosX = BoundaryX;

	if (ClimberPawn->bOnGround)
	{
		ClimberPawn->VelX = 0.0f;
		return;
	}

	const float ImpactSpeed = FMath::Abs(ClimberPawn->VelX);
	const float MinRicochetSpeed = 3.2f * BaseSpeedRatio;

	if (!ClimberPawn->bWallBounceUsed && ImpactSpeed >= MinRicochetSpeed)
	{
		ClimberPawn->bWallBounceUsed = true;
		const float ReflectedSpeed = FMath::Min(MaxVx * 1.04f, FMath::Max(6.2f * BaseSpeedRatio, ImpactSpeed * 1.05f));
		ClimberPawn->VelX = float(-SideDir) * ReflectedSpeed;
		ClimberPawn->FacingDir = -SideDir;

		// HTML vy < 1.8 corresponds to UE VelZ > -1.8
		if (ClimberPawn->VelZ > -1.8f)
		{
			ClimberPawn->VelZ = FMath::Max(ClimberPawn->VelZ + 3.1f, 11.4f);
		}
		else
		{
			ClimberPawn->VelZ = 9.0f;
		}

		ClimberPawn->bFlipping = true;
		ClimberPawn->SquashScaleX = 0.68f;
		ClimberPawn->SquashScaleZ = 1.32f;
		ScreenShake = 6.0f;

		if (SideDir == -1) LeftWallFlash = 1.0f;
		else RightWallFlash = 1.0f;

		RecordQuestProgress(TEXT("wall"), 1);
		UnlockAchievementById(TEXT("wall_runner"));

		if (AVTPlayerController* PC = Cast<AVTPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
		{
			PC->TriggerHapticFeedback(45);
		}

		const int32 WallPts = 250 * (1 + CurrentCombo);
		BonusScoreAccumulated += WallPts;
		CurrentScore += WallPts;

		SpawnShockwave3D(BoundaryX, ClimberPawn->PosZ, 120.0f, FLinearColor(0.0f, 0.961f, 0.831f));
		SpawnFloatingText(FString::Printf(TEXT("WICKED WALL BOUNCE! +%d PTS"), WallPts), BoundaryX, ClimberPawn->PosZ + 50.0f, FLinearColor(0.0f, 0.961f, 0.831f), 15);
		PlayTowerSFX(TEXT("WallRicochet"), 1.05f, 1.0f);
		TriggerAnnouncerVoice(TEXT("VO_Wall_Ricochet"), FString::Printf(TEXT("WICKED WALL BOUNCE! +%d PTS"), WallPts));
		AwardPlayerXP(60, TEXT("Super Ricochet"));
	}
	else
	{
		ClimberPawn->VelX = float(-SideDir) * FMath::Max(2.0f * BaseSpeedRatio, ImpactSpeed * 0.45f);
		ClimberPawn->FacingDir = -SideDir;
		PlayTowerSFX(TEXT("Jump"), 0.78f, 0.45f);
	}
}

void AVTTowerGameMode::CashOutCombo()
{
	if (CurrentCombo <= 0 || !ClimberPawn) return;

	const int32 Mult = (CurrentGameMode == EVTGameModeType::Overdrive2x || CurrentCombo >= 10) ? 2 : 1;
	const int32 Bonus = CurrentCombo * CurrentCombo * 15 * Mult;
	BonusScoreAccumulated += Bonus;
	CurrentScore += Bonus;

	static const TArray<FString> Callouts = {
		TEXT("NICE!"), TEXT("SWEET!"), TEXT("GREAT!"), TEXT("SUPER!"),
		TEXT("EXCELLENT!"), TEXT("AMAZING!"), TEXT("INCREDIBLE!"), TEXT("UNSTOPPABLE!"),
		TEXT("MONSTER COMBO!"), TEXT("ULTRA COMBO!"), TEXT("GODLIKE APEX!!")
	};
	const int32 CIdx = FMath::Min(Callouts.Num() - 1, CurrentCombo / 3);
	const FString CallTxt = Callouts[CIdx];

	LatestAnnouncerCallout = FString::Printf(TEXT("CASHOUT %s +%d PTS!"), *CallTxt, Bonus);
	AnnouncerTimerSec = 1.8f;
	ComboHitPulse = 1.0f;
	PlayTowerSFX(TEXT("ComboUp"), FMath::Clamp(1.05f + float(CurrentCombo) * 0.035f, 1.05f, 1.95f), 0.95f);
	AwardPlayerXP(CurrentCombo * 25, TEXT("Combo Cashout"));

	if (CurrentCombo >= 10)
	{
		UnlockAchievementById(TEXT("combo_master"));
		AwardNeonCores(1, TEXT("10x+ Combo Bonus"));
	}
	if (CurrentCombo >= 25)
	{
		UnlockAchievementById(TEXT("ludicrous"));
		AwardNeonCores(3, TEXT("25x GODLIKE Combo Bonus"));
	}

	SpawnFloatingText(FString::Printf(TEXT("%s +%d PTS"), *CallTxt, Bonus), 0.0f, ClimberPawn->PosZ + 90.0f, FLinearColor(1.0f, 0.82f, 0.15f), 18);
	CurrentCombo = 0;
	ComboTimerFrames = 0;
	LastComboVoiceTier = 0;
}

void AVTTowerGameMode::StepFixedPhysics60Hz()
{
	if (TowerRunState != EVTGameState::Play || !ClimberPawn || bIsDeathSequenceActive || bIsGamePaused) return;

	AVTPlayerController* PC = Cast<AVTPlayerController>(UGameplayStatics::GetPlayerController(this, 0));

	if (NearDeathVoiceCooldownSec > 0.0f) NearDeathVoiceCooldownSec = FMath::Max(0.0f, NearDeathVoiceCooldownSec - FixedStepSeconds);
	if (ItemBonusTimerSec > 0.0f) ItemBonusTimerSec = FMath::Max(0.0f, ItemBonusTimerSec - FixedStepSeconds);

	if (MagnetFrames > 0) MagnetFrames--;
	if (FreezeFrames > 0) FreezeFrames--;
	if (bAdminInfinitePowerups)
	{
		MagnetFrames = 540;
		ShieldCount = FMath::Max(1, ShieldCount);
		ClimberPawn->SetShieldVisualActive(true);
	}

	// 1. Collectibles & Power-Ups Magnetic Pull, Distinct SFX & Combo-Multiplied Score!
	for (AVTPlatformActor* P : Platforms)
	{
		if (!P || P->bBroken || P->ItemType == EVTItemType::None || P->bItemCollected) continue;

		if (MagnetFrames > 0 && P->ItemType == EVTItemType::Core)
		{
			const float Dx = ClimberPawn->PosX - P->ItemWorldLocation.X;
			const float Dz = ClimberPawn->PosZ - P->ItemWorldLocation.Z;
			const float Dist = FMath::Sqrt(Dx * Dx + Dz * Dz);
			if (Dist < 480.0f && Dist > 1.0f)
			{
				P->ItemWorldLocation.X += (Dx / Dist) * 16.0f;
				P->ItemWorldLocation.Z += (Dz / Dist) * 16.0f;
			}
		}

		const float Dx = ClimberPawn->PosX - P->ItemWorldLocation.X;
		const float Dz = ClimberPawn->PosZ - P->ItemWorldLocation.Z;
		if (Dx * Dx + Dz * Dz < 64.0f * 64.0f)
		{
			const EVTItemType CollectedType = P->ItemType;
			P->CollectItem();
			if (PC) PC->TriggerHapticFeedback(35);

			// Higher Combo multiplies collected item points significantly!
			const int32 ItemComboMult = 1 + CurrentCombo;
			const int32 BaseItemPts = (CollectedType == EVTItemType::Core) ? 250 : 400;
			const int32 EarnedItemPts = BaseItemPts * ItemComboMult;
			BonusScoreAccumulated += EarnedItemPts;
			CurrentScore += EarnedItemPts;
			ComboHitPulse = 1.0f;

			AwardPlayerXP(90 + CurrentCombo * 15, TEXT("Item Pickup"));

			if (CollectedType == EVTItemType::Core)
			{
				PlayTowerSFX(TEXT("CorePickup"), FMath::Clamp(1.0f + float(CurrentCombo) * 0.03f, 1.0f, 1.65f), 0.98f);
				AwardNeonCores(1, TEXT("Turm-Fund"));
				LatestItemBonusText = FString::Printf(TEXT("CORE +%d PTS (%dx COMBO BONUS!)"), EarnedItemPts, ItemComboMult);
				ItemBonusTimerSec = 1.5f;
				SpawnFloatingText(FString::Printf(TEXT("+1 CORE // +%d PTS (%dx)"), EarnedItemPts, ItemComboMult), P->ItemWorldLocation.X, P->ItemWorldLocation.Z + 25.0f, FLinearColor(0.0f, 0.961f, 0.831f), 15);
			}
			else if (CollectedType == EVTItemType::Magnet)
			{
				PlayTowerSFX(TEXT("PowerUp_Magnet"), 1.0f, 1.05f);
				MagnetFrames = 540;
				LatestItemBonusText = FString::Printf(TEXT("MAGNET +%d PTS (%dx COMBO BONUS!)"), EarnedItemPts, ItemComboMult);
				ItemBonusTimerSec = 1.7f;
				SpawnFloatingText(FString::Printf(TEXT("MAGNET! +%d PTS (%dx)"), EarnedItemPts, ItemComboMult), P->ItemWorldLocation.X, P->ItemWorldLocation.Z + 30.0f, FLinearColor(0.969f, 0.145f, 0.522f), 16);
			}
			else if (CollectedType == EVTItemType::Spring)
			{
				PlayTowerSFX(TEXT("PowerUp_Spring"), 1.0f, 1.05f);
				SpringJumpsLeft = 3;
				UnlockAchievementById(TEXT("spring_master"));
				LatestItemBonusText = FString::Printf(TEXT("SPRING BOOTS +%d PTS (%dx COMBO BONUS!)"), EarnedItemPts, ItemComboMult);
				ItemBonusTimerSec = 1.7f;
				SpawnFloatingText(FString::Printf(TEXT("3x SPRING BOOTS! +%d PTS"), EarnedItemPts), P->ItemWorldLocation.X, P->ItemWorldLocation.Z + 30.0f, FLinearColor(1.0f, 0.820f, 0.400f), 16);
			}
			else if (CollectedType == EVTItemType::Freeze)
			{
				PlayTowerSFX(TEXT("PowerUp_Freeze"), 1.0f, 1.05f);
				FreezeFrames = 360;
				LatestItemBonusText = FString::Printf(TEXT("TIME FREEZE +%d PTS (%dx COMBO BONUS!)"), EarnedItemPts, ItemComboMult);
				ItemBonusTimerSec = 1.7f;
				SpawnFloatingText(FString::Printf(TEXT("TIME FREEZE! +%d PTS"), EarnedItemPts), P->ItemWorldLocation.X, P->ItemWorldLocation.Z + 30.0f, FLinearColor(0.220f, 0.741f, 0.973f), 16);
			}
			else if (CollectedType == EVTItemType::Shield)
			{
				PlayTowerSFX(TEXT("PowerUp_Shield"), 1.0f, 1.05f);
				ShieldCount = 1;
				ClimberPawn->SetShieldVisualActive(true);
				LatestItemBonusText = FString::Printf(TEXT("AEGIS SHIELD +%d PTS (%dx COMBO BONUS!)"), EarnedItemPts, ItemComboMult);
				ItemBonusTimerSec = 1.7f;
				SpawnFloatingText(FString::Printf(TEXT("AEGIS SHIELD! +%d PTS"), EarnedItemPts), P->ItemWorldLocation.X, P->ItemWorldLocation.Z + 30.0f, FLinearColor(0.0f, 0.961f, 0.831f), 16);
			}
		}
	}

	const FVTFloorScaling Scaling = GetFloorScaling(MaxFloor);
	const float MaxVx = 9.6f * Scaling.SpeedMult;

	// Neon Overdrive activation at 10x Combo
	if (CurrentCombo >= 10 && !ClimberPawn->bWasOverdrive)
	{
		ClimberPawn->bWasOverdrive = true;
		SpawnFloatingText(TEXT("OVERDRIVE (2x SCORE)!"), 0.0f, ClimberPawn->PosZ + 100.0f, FLinearColor(0.969f, 0.145f, 0.522f), 18);
		LatestAnnouncerCallout = TEXT("OVERDRIVE!!");
		AnnouncerTimerSec = 1.8f;
	}
	else if (CurrentCombo < 10 && ClimberPawn->bWasOverdrive)
	{
		ClimberPawn->bWasOverdrive = false;
	}

	BeatPulse *= 0.88f;
	LeftWallFlash *= 0.85f;
	RightWallFlash *= 0.85f;
	if (ScreenShake > 0.0f) ScreenShake *= 0.82f;

	// Blitz 60s Mode Countdown
	if (CurrentGameMode == EVTGameModeType::Blitz60s)
	{
		BlitzFramesRemaining--;
		if (BlitzFramesRemaining <= 0)
		{
			EndCurrentRun(TEXT("TIME UP! BLITZ COMPLETE"), TEXT("Deine 60 Sekunden im Blitz-Modus sind abgelaufen!"));
			return;
		}
	}

	// Squash & Stretch recovery
	ClimberPawn->SquashScaleX += (1.0f - ClimberPawn->SquashScaleX) * 0.22f;
	ClimberPawn->SquashScaleZ += (1.0f - ClimberPawn->SquashScaleZ) * 0.22f;

	// Horizontal Acceleration
	// NOTE: Because TowerCamera sits at Y = -960 looking along +Y (Yaw = +90 deg),
	// World -X is on the RIGHT side of the screen and World +X is on the LEFT side of the screen!
	// Therefore, moving RIGHT on screen (bKeyR or Analog > 0 from Gamepad Left Stick / Virtual Joystick)
	// decreases World X (-Accel), and moving LEFT on screen increases World X (+Accel)!
	const float Accel = (ClimberPawn->bOnGround ? 0.64f : 0.46f) * Scaling.SpeedMult;
	const bool bKeyL = PC ? PC->bKeyLeft : false;
	const bool bKeyR = PC ? PC->bKeyRight : false;
	const float Analog = PC ? PC->AnalogSteer : 0.0f;

	if (FMath::Abs(Analog) > 0.08f)
	{
		ClimberPawn->VelX -= Analog * Accel * 1.15f;
		ClimberPawn->FacingDir = (Analog >= 0.0f) ? -1 : 1;
	}
	else
	{
		if (bKeyL) { ClimberPawn->VelX += Accel; ClimberPawn->FacingDir = 1; }
		if (bKeyR) { ClimberPawn->VelX -= Accel; ClimberPawn->FacingDir = -1; }
	}

	if (!bKeyL && !bKeyR && FMath::Abs(Analog) <= 0.05f)
	{
		ClimberPawn->VelX *= ClimberPawn->bOnGround ? 0.82f : 0.955f;
	}

	ClimberPawn->VelX = FMath::Clamp(ClimberPawn->VelX, -MaxVx, MaxVx);
	ClimberPawn->PosX += ClimberPawn->VelX * 2.0f; // 1px = 2.0cm

	if (ClimberPawn->bOnGround && FMath::Abs(ClimberPawn->VelX) > 0.35f)
	{
		ClimberPawn->RunPhase += FMath::Abs(ClimberPawn->VelX) * 0.08f;
	}
	else if (ClimberPawn->bOnGround)
	{
		ClimberPawn->RunPhase *= 0.8f;
	}

	// Wall Collision & Super Ricochet
	if (ClimberPawn->PosX - ClimberPawn->RadiusCm <= -PlayfieldHalfWidthCm)
	{
		HandleWallCollision(-1, MaxVx);
	}
	else if (ClimberPawn->PosX + ClimberPawn->RadiusCm >= PlayfieldHalfWidthCm)
	{
		HandleWallCollision(1, MaxVx);
	}

	// Coyote Time & Jump Buffer
	if (ClimberPawn->bOnGround)
	{
		ClimberPawn->CoyoteFrames = 7;
	}
	else if (ClimberPawn->CoyoteFrames > 0)
	{
		ClimberPawn->CoyoteFrames--;
	}

	if (PC && PC->JumpBufferFrames > 0)
	{
		PC->JumpBufferFrames--;
	}

	const bool bWantsJump = (PC && (PC->JumpBufferFrames > 0 || PC->bJumpHeld)) ||
		(CurrentControlMode == EVTControlMode::AutoBounce && ClimberPawn->bOnGround);

	const bool bCanJumpNow = (ClimberPawn->CoyoteFrames > 0) || (bAdminInfiniteJump && PC && PC->JumpBufferFrames > 0);
	if (bWantsJump && bCanJumpNow)
	{
		const float NormalizedEnergy = FMath::Min(1.12f, FMath::Abs(ClimberPawn->VelX) / FMath::Max(3.5f, MaxVx));
		const float SpeedBonus = FMath::Pow(NormalizedEnergy, 1.2f) * 7.8f;
		const float JumpMult = FMath::Clamp(AdminJumpForceScale, 0.4f, 3.0f);

		if (SpringJumpsLeft > 0)
		{
			ClimberPawn->VelZ = (10.8f + SpeedBonus) * 1.35f * JumpMult;
			SpringJumpsLeft--;
			SpawnFloatingText(TEXT("SUPER BOUNCE!"), ClimberPawn->PosX, ClimberPawn->PosZ + 50.0f, FLinearColor(1.0f, 0.820f, 0.400f), 15);
			PlayTowerSFX(TEXT("SuperLaunch"), 1.1f, 0.9f);
		}
		else
		{
			ClimberPawn->VelZ = (10.8f + SpeedBonus) * JumpMult;
			PlayTowerSFX(TEXT("Jump"), 0.92f + NormalizedEnergy * 0.30f, 0.78f);
		}

		if (PC)
		{
			PC->JumpBufferFrames = 0;
			PC->TriggerHapticFeedback(18);
		}

		ClimberPawn->bOnGround = false;
		ClimberPawn->CoyoteFrames = 0;
		ClimberPawn->bWallBounceUsed = false;
		ClimberPawn->SquashScaleX = 0.72f;
		ClimberPawn->SquashScaleZ = 1.34f;

		const FVTClimberInfo CInfo = GetClimberInfo(SelectedClimberId);
		if (NormalizedEnergy > 0.58f)
		{
			ClimberPawn->bFlipping = true;
			SpawnShockwave3D(ClimberPawn->PosX, ClimberPawn->PosZ - 36.0f, 95.0f, CInfo.HoodieColor);
		}
	}

	// Improved Jump Physics: Subtle Apex Hang-Time Modulation so high-speed jumps feel extra responsive & heroic!
	const float PrevFeetZ = ClimberPawn->PosZ - ClimberPawn->HalfHeightCm;
	float GravityStep = 0.48f * FMath::Clamp(AdminGravityScale, 0.15f, 2.5f);
	if (!ClimberPawn->bOnGround && FMath::Abs(ClimberPawn->VelZ) < 2.4f)
	{
		GravityStep = 0.38f * FMath::Clamp(AdminGravityScale, 0.15f, 2.5f); // Smooth apex hang-time!
	}
	ClimberPawn->VelZ = FMath::Max(-16.5f, ClimberPawn->VelZ - GravityStep);
	ClimberPawn->PosZ += ClimberPawn->VelZ * 2.0f; // 1px = 2.0cm
	ClimberPawn->bOnGround = false;

	if (ClimberPawn->bFlipping)
	{
		ClimberPawn->FlipAngleRad += 0.31f * (ClimberPawn->VelX >= 0.0f ? 1.0f : -1.0f);
	}
	else
	{
		ClimberPawn->FlipAngleRad = ClimberPawn->VelX * 0.032f;
	}

	// Platform Movement, Cracking & Landing Collision
	const FVTBiomeInfo CurBiome = GetBiomeForFloor(MaxFloor);
	const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	for (AVTPlatformActor* P : Platforms)
	{
		if (!P || P->bBroken) continue;

		if (FMath::Abs(P->VelocityX) > 0.001f)
		{
			P->PlatformX += P->VelocityX * 2.0f;
			const float HalfW = P->PlatformWidth * 0.5f;
			const float LimitX = PlayfieldHalfWidthCm - 12.0f - HalfW;
			if (P->PlatformX <= -LimitX)
			{
				P->PlatformX = -LimitX;
				P->VelocityX = FMath::Abs(P->VelocityX);
			}
			else if (P->PlatformX >= LimitX)
			{
				P->PlatformX = LimitX;
				P->VelocityX = -FMath::Abs(P->VelocityX);
			}
		}

		if (P->bTouched && !P->bIsCheckpoint)
		{
			if (FreezeFrames <= 0)
			{
				P->CrackTime += FixedStepSeconds;
			}
			if (P->CrackTime >= Scaling.CrackDuration)
			{
				P->ShatterPlatform();
				SpawnIceShatter3D(P, CurBiome.TopColor);
				PlayTowerSFX(TEXT("IceShatter"), FMath::RandRange(0.92f, 1.12f), 0.65f);
				continue;
			}
		}

		const float NewFeetZ = ClimberPawn->PosZ - ClimberPawn->HalfHeightCm;
		const float PlatLeft = P->PlatformX - P->PlatformWidth * 0.5f;
		const float PlatRight = P->PlatformX + P->PlatformWidth * 0.5f;

		// Falling onto platform check (matching HTML + ledge forgiveness)
		if (ClimberPawn->VelZ <= 0.0f &&
			PrevFeetZ >= P->PlatformZ - 26.0f &&
			NewFeetZ <= P->PlatformZ + 6.0f &&
			ClimberPawn->PosX + ClimberPawn->RadiusCm * 0.82f >= PlatLeft &&
			ClimberPawn->PosX - ClimberPawn->RadiusCm * 0.82f <= PlatRight)
		{
			if (!P->IsSolidAtTime(TimeSec))
			{
				continue;
			}

			// Boost / Launch-Pad
			if (P->PlatType == EVTPlatformType::Boost)
			{
				ClimberPawn->PosZ = P->PlatformZ + ClimberPawn->HalfHeightCm;
				ClimberPawn->VelZ = 21.5f * FMath::Clamp(AdminJumpForceScale, 0.5f, 3.0f);
				ClimberPawn->bOnGround = false;
				ClimberPawn->bFlipping = true;
				ClimberPawn->SquashScaleX = 0.65f;
				ClimberPawn->SquashScaleZ = 1.45f;
				ScreenShake = 8.0f;
				if (PC) PC->TriggerHapticFeedback(55);
				PlayTowerSFX(TEXT("SuperLaunch"), 1.0f, 1.0f);
				AwardPlayerXP(85, TEXT("Super Launch-Pad"));
				SpawnShockwave3D(P->PlatformX, P->PlatformZ, 160.0f, FLinearColor(1.0f, 0.745f, 0.043f));
				SpawnFloatingText(TEXT("SUPER LAUNCH! +85 XP"), P->PlatformX, P->PlatformZ + 64.0f, FLinearColor(1.0f, 0.745f, 0.043f), 17);
				RecordQuestProgress(TEXT("boost"), 1);
				continue;
			}

			// Mirror Ice Platform
			if (P->PlatType == EVTPlatformType::MirrorIce && FMath::Abs(ClimberPawn->VelX) > 0.5f)
			{
				ClimberPawn->VelX *= 1.18f;
			}

			const float PreLandVelZ = ClimberPawn->VelZ;
			ClimberPawn->PosZ = P->PlatformZ + ClimberPawn->HalfHeightCm;
			ClimberPawn->VelZ = 0.0f;
			ClimberPawn->bOnGround = true;
			ClimberPawn->bFlipping = false;
			ClimberPawn->FlipAngleRad = 0.0f;
			ClimberPawn->bWallBounceUsed = false;

			if (FMath::Abs(P->VelocityX) > 0.001f)
			{
				ClimberPawn->PosX += P->VelocityX * 1.2f;
			}

			ClimberPawn->SquashScaleX = 1.25f;
			ClimberPawn->SquashScaleZ = 0.76f;

			P->TriggerLandingTouch();

			// Near-Death Clutch Save Check ("OH, THAT WAS CLOSE!" / "BARELY MADE IT! WHAT A SAVE!")
			if (PreLandVelZ <= -4.5f && MaxFloor > StartFloor + 4 && (CameraZ - ClimberPawn->PosZ) > 390.0f && NearDeathVoiceCooldownSec <= 0.0f)
			{
				NearDeathVoiceCooldownSec = 4.5f;
				const bool bVariant = FMath::RandBool();
				const int32 ClutchBonus = 600 * (1 + CurrentCombo / 2);
				BonusScoreAccumulated += ClutchBonus;
				CurrentScore += ClutchBonus;
				TriggerAnnouncerVoice(
					bVariant ? TEXT("VO_Clutch_Close") : TEXT("VO_Clutch_Barely"),
					bVariant ? FString::Printf(TEXT("OH, THAT WAS CLOSE! +%d PTS"), ClutchBonus) : FString::Printf(TEXT("BARELY MADE IT! +%d PTS"), ClutchBonus));
				SpawnFloatingText(FString::Printf(TEXT("CLUTCH SAVE! +%d PTS"), ClutchBonus), ClimberPawn->PosX, ClimberPawn->PosZ + 80.0f, FLinearColor(1.0f, 0.35f, 0.15f), 17);
			}

			if (P->FloorIndex > ClimberPawn->CurrentFloor)
			{
				const int32 Diff = P->FloorIndex - ClimberPawn->CurrentFloor;
				ClimberPawn->CurrentFloor = P->FloorIndex;

				if (Diff >= 2)
				{
					CurrentCombo += Diff;
					ComboTimerFrames = 185;
					ComboHitPulse = 1.0f;
					if (CurrentCombo > MaxCombo) MaxCombo = CurrentCombo;
					if (CurrentCombo >= 8) RecordQuestProgress(TEXT("combo"), CurrentCombo);

					const int32 ComboClimbBonus = Diff * 75 * CurrentCombo;
					BonusScoreAccumulated += ComboClimbBonus;

					// Rising Pitch Dopamine Chime
					const float ComboPitch = FMath::Clamp(0.95f + float(CurrentCombo) * 0.045f, 0.95f, 2.15f);
					PlayTowerSFX(TEXT("ComboUp"), ComboPitch, 0.90f);

					const int32 XpEarned = Diff * 20 * FMath::Max(1, CurrentCombo / 2);
					AwardPlayerXP(XpEarned, TEXT("Multi-Floor Combo"));

					// Well-spaced Quake / Unreal Tournament Style Deep Male Announcer Milestones (No spam!)
					int32 VoiceTier = 0;
					FString VoiceKey = TEXT("");
					FString TierWord = TEXT("");
					FLinearColor TierCol(0.0f, 0.961f, 0.831f);

					if (CurrentCombo >= 200)
					{
						VoiceTier = 16 + (CurrentCombo - 200) / 50;
						VoiceKey = (VoiceTier % 2 == 0) ? TEXT("VO_Combo_19") : TEXT("VO_Combo_HolyShit");
						TierWord = TEXT("GODLIKE! APEX EMPEROR!");
						TierCol = FLinearColor(1.0f, 0.84f, 0.0f);
					}
					else if (CurrentCombo >= 185) { VoiceTier = 15; VoiceKey = TEXT("VO_Combo_HolyShit"); TierWord = TEXT("HOLY SHIT!");              TierCol = FLinearColor(1.0f, 0.15f, 0.15f); }
					else if (CurrentCombo >= 165) { VoiceTier = 14; VoiceKey = TEXT("VO_Combo_18");       TierWord = TEXT("LUDICROUS SPEED!");        TierCol = FLinearColor(1.0f, 0.55f, 0.10f); }
					else if (CurrentCombo >= 145) { VoiceTier = 13; VoiceKey = TEXT("VO_Combo_17");       TierWord = TEXT("MONSTER COMBO!");          TierCol = FLinearColor(1.0f, 0.10f, 0.50f); }
					else if (CurrentCombo >= 125) { VoiceTier = 12; VoiceKey = TEXT("VO_Combo_16");       TierWord = TEXT("ULTRA COMBO!");            TierCol = FLinearColor(0.85f, 0.20f, 1.0f); }
					else if (CurrentCombo >= 105) { VoiceTier = 11; VoiceKey = TEXT("VO_Combo_11");       TierWord = TEXT("UNBELIEVABLE!");           TierCol = FLinearColor(0.10f, 0.85f, 1.0f); }
					else if (CurrentCombo >= 85)  { VoiceTier = 10; VoiceKey = TEXT("VO_Combo_10");       TierWord = TEXT("SPECTACULAR!");            TierCol = FLinearColor(0.20f, 1.0f, 0.50f); }
					else if (CurrentCombo >= 70)  { VoiceTier = 9;  VoiceKey = TEXT("VO_Combo_12");       TierWord = TEXT("UNSTOPPABLE!");            TierCol = FLinearColor(1.0f, 0.75f, 0.10f); }
					else if (CurrentCombo >= 55)  { VoiceTier = 8;  VoiceKey = TEXT("VO_Combo_14");       TierWord = TEXT("RAMPAGE!");                TierCol = FLinearColor(1.0f, 0.35f, 0.15f); }
					else if (CurrentCombo >= 40)  { VoiceTier = 7;  VoiceKey = TEXT("VO_Combo_13");       TierWord = TEXT("DOMINATING!");             TierCol = FLinearColor(1.0f, 0.20f, 0.20f); }
					else if (CurrentCombo >= 30)  { VoiceTier = 6;  VoiceKey = TEXT("VO_Combo_07");       TierWord = TEXT("OUTSTANDING! OVERDRIVE!"); TierCol = FLinearColor(0.0f, 0.95f, 0.80f); }
					else if (CurrentCombo >= 20)  { VoiceTier = 5;  VoiceKey = TEXT("VO_Combo_06");       TierWord = TEXT("EXCELLENT!");              TierCol = FLinearColor(0.20f, 0.90f, 0.40f); }
					else if (CurrentCombo >= 12)  { VoiceTier = 4;  VoiceKey = TEXT("VO_Combo_05");       TierWord = TEXT("IMPRESSIVE!");             TierCol = FLinearColor(0.60f, 0.95f, 0.20f); }
					else if (CurrentCombo >= 6)   { VoiceTier = 3;  VoiceKey = TEXT("VO_Combo_04");       TierWord = TEXT("SUPER COMBO!");            TierCol = FLinearColor(1.0f, 0.85f, 0.20f); }
					else if (CurrentCombo >= 3)   { VoiceTier = 2;  VoiceKey = TEXT("VO_Combo_02");       TierWord = TEXT("TRIPLE STREAK! SWEET!");   TierCol = FLinearColor(0.30f, 0.80f, 1.0f); }
					else if (CurrentCombo == 2)   { VoiceTier = 1;  VoiceKey = TEXT("VO_Combo_01");       TierWord = TEXT("DOUBLE JUMP! NICE!");       TierCol = FLinearColor(0.0f, 0.961f, 0.831f); }

					if (VoiceTier > 0 && VoiceTier > LastComboVoiceTier)
					{
						LastComboVoiceTier = VoiceTier;
						TriggerAnnouncerVoice(VoiceKey, FString::Printf(TEXT("%dx %s"), CurrentCombo, *TierWord));
					}
					else if (!TierWord.IsEmpty())
					{
						LatestAnnouncerCallout = FString::Printf(TEXT("%dx %s"), CurrentCombo, *TierWord);
						AnnouncerTimerSec = 1.45f;
					}

					SpawnShockwave3D(ClimberPawn->PosX, ClimberPawn->PosZ - 20.0f, FMath::Min(220.0f, 80.0f + float(CurrentCombo) * 5.0f), TierCol);
					SpawnFloatingText(FString::Printf(TEXT("+%d PTS (%dx %s)"), ComboClimbBonus, CurrentCombo, *TierWord), ClimberPawn->PosX, ClimberPawn->PosZ + 68.0f, TierCol, 18);
					ScorePulseAnim = 1.0f;
				}
				else
				{
					AwardPlayerXP(15, TEXT("Floor Climb"));
					const int32 SingleScoreMult = (CurrentGameMode == EVTGameModeType::Overdrive2x || CurrentCombo >= 10) ? 2 : 1;
					const int32 SingleFloorPts = 25 * SingleScoreMult;
					SpawnFloatingText(FString::Printf(TEXT("+%d"), SingleFloorPts), ClimberPawn->PosX + FMath::RandRange(-20.0f, 20.0f), ClimberPawn->PosZ + 52.0f, FLinearColor(0.0f, 0.961f, 0.831f), 15);
					ScorePulseAnim = 1.0f;
					if (CurrentCombo > 0 && Diff == 1 && CurrentControlMode != EVTControlMode::AutoBounce)
					{
						CashOutCombo();
					}
				}

				if (ClimberPawn->CurrentFloor > MaxFloor)
				{
					const int32 OldMilestone = MaxFloor / 25;
					MaxFloor = ClimberPawn->CurrentFloor;
					const int32 NewMilestone = MaxFloor / 25;
					if (NewMilestone > OldMilestone)
					{
						AwardNeonCores(1, FString::Printf(TEXT("Etage %d Milestone"), MaxFloor));
					}

					if (MaxFloor >= 25) UnlockAchievementById(TEXT("first_steps"));
					if (MaxFloor >= 100) UnlockAchievementById(TEXT("century"));
					if (MaxFloor >= 200) UnlockAchievementById(TEXT("stratosphere"));

					const int32 ScoreMult = (CurrentGameMode == EVTGameModeType::Overdrive2x || CurrentCombo >= 10) ? 2 : 1;
					CurrentScore = MaxFloor * 25 * ScoreMult + BonusScoreAccumulated;

					if (CurrentScore > BestScore && !bNewHighScoreAnnouncedThisRun && BestScore > 0)
					{
						bNewHighScoreAnnouncedThisRun = true;
						bNewHighScoreAchieved = true;
						HighScoreCelebrationTimer = 3.8f;
						TriggerAnnouncerVoice(TEXT("VO_HighScore"), TEXT("NEW HIGH SCORE!"));
						SpawnFloatingText(TEXT("*** NEW HIGH SCORE! ***"), 0.0f, ClimberPawn->PosZ + 120.0f, FLinearColor(1.0f, 0.85f, 0.10f), 22);
						SpawnShockwave3D(0.0f, ClimberPawn->PosZ, 320.0f, FLinearColor(1.0f, 0.85f, 0.10f));
					}
				}
			}
		}
	}

	if (ComboTimerFrames > 0)
	{
		ComboTimerFrames--;
		if (ComboTimerFrames == 0 && CurrentCombo > 0)
		{
			CashOutCombo();
		}
	}

	// Rising Camera & Hurry Up Escalation
	const float TargetCamZ = FMath::Max(CameraZ, ClimberPawn->PosZ + 140.0f);
	CameraZ += (TargetCamZ - CameraZ) * 0.15f;

	if (CurrentGameMode != EVTGameModeType::Zen && MaxFloor >= StartFloor + 5)
	{
		if (FreezeFrames <= 0)
		{
			HurryTimerFrames++;
			if (HurryTimerFrames % 1080 == 0)
			{
				HurryLevel++;
				ScreenShake = 8.0f;
				HurryBannerTimerSec = 2.4f;
			}
			const float ScrollSpeedCm = (0.78f + float(HurryLevel) * 0.38f + float(MaxFloor - StartFloor) * 0.0045f) * Scaling.SpeedMult * 2.0f * FMath::Clamp(AdminCameraSpeedScale, 0.0f, 3.0f);
			CameraZ += ScrollSpeedCm;
		}
	}
	else if (CurrentGameMode == EVTGameModeType::Zen)
	{
		const float ZenCamZ = ClimberPawn->PosZ + 110.0f;
		CameraZ += (ZenCamZ - CameraZ) * 0.12f;
	}

	// Reactive Synth BPM calculation
	const FVTClimberInfo CInfo = GetClimberInfo(SelectedClimberId);
	CurrentBpm = FMath::Min(208, FMath::RoundToInt(
		float(CInfo.BaseBpm) +
		float(MaxFloor - StartFloor) * 0.25f +
		float(HurryLevel * 8) +
		(CurrentCombo > 0 ? FMath::Min(24.0f, float(CurrentCombo) * 0.85f) : 0.0f)
	));
	if (CurrentBpm > PeakBpm) PeakBpm = CurrentBpm;

	// Infinite Procedural Platform Spawning & Culling
	if (Platforms.Num() > 0 && Platforms.Last())
	{
		if (Platforms.Last()->PlatformZ < CameraZ + 1250.0f)
		{
			SpawnPlatformForFloor(Platforms.Last()->FloorIndex + 1);
		}
	}
	while (Platforms.Num() > 72 && CurrentGameMode != EVTGameModeType::Zen)
	{
		if (Platforms[0]) Platforms[0]->Destroy();
		Platforms.RemoveAt(0);
	}

	// 7-Segment 3D Scarf Physics
	if (ClimberPawn->ScarfPoints.Num() >= 7)
	{
		ClimberPawn->ScarfPoints[0] = FVector(ClimberPawn->PosX - float(ClimberPawn->FacingDir) * 8.0f, 12.0f, ClimberPawn->PosZ + 16.0f);
		for (int32 i = 1; i < ClimberPawn->ScarfPoints.Num(); ++i)
		{
			const FVector Prev = ClimberPawn->ScarfPoints[i - 1];
			FVector& Cur = ClimberPawn->ScarfPoints[i];
			const float Wave = FMath::Sin(TimeSec * 15.0f + float(i) * 0.7f) * 3.5f;
			Cur.X += (Prev.X - ClimberPawn->VelX * 2.3f - float(ClimberPawn->FacingDir) * 5.0f - Cur.X) * 0.45f;
			Cur.Y = 12.0f + float(i) * 2.5f;
			Cur.Z += (Prev.Z - 6.4f + Wave * 0.5f - Cur.Z) * 0.45f;
			Cur.X = FMath::Clamp(Cur.X, -PlayfieldHalfWidthCm + 10.0f, PlayfieldHalfWidthCm - 10.0f);
		}
	}

	EnergyBarPercent = FMath::Clamp((FMath::Abs(ClimberPawn->VelX) / FMath::Max(3.5f, MaxVx)) * 100.0f, 0.0f, 100.0f);

	// Death Check below rising camera (with Admin Godmode & Aegis Shield rescue!)
	if (CurrentGameMode != EVTGameModeType::Zen && (CameraZ - ClimberPawn->PosZ) > 680.0f)
	{
		if (bAdminGodMode)
		{
			// GODMODE ACTIVE: Never die! Launch heroically back into the tower!
			ClimberPawn->PosZ = CameraZ - 320.0f;
			ClimberPawn->VelZ = 21.0f * FMath::Clamp(AdminJumpForceScale, 0.5f, 3.0f);
			ClimberPawn->bOnGround = false;
			ClimberPawn->bFlipping = true;
			PlayTowerSFX(TEXT("SuperLaunch"), 1.15f, 0.9f);
			SpawnShockwave3D(ClimberPawn->PosX, ClimberPawn->PosZ, 180.0f, FLinearColor(1.0f, 0.82f, 0.15f));
			SpawnFloatingText(TEXT("GODMODE BOOST!"), 0.0f, ClimberPawn->PosZ + 70.0f, FLinearColor(1.0f, 0.82f, 0.15f), 17);
		}
		else if (ShieldCount > 0)
		{
			ShieldCount--;
			ClimberPawn->SetShieldVisualActive(ShieldCount > 0);
			if (PC) PC->TriggerHapticFeedback(80);
			ClimberPawn->PosZ = CameraZ - 380.0f;
			ClimberPawn->VelZ = 19.5f;
			ClimberPawn->bOnGround = false;
			ClimberPawn->bFlipping = true;
			SpawnShockwave3D(ClimberPawn->PosX, ClimberPawn->PosZ, 160.0f, FLinearColor(0.0f, 0.961f, 0.831f));
			SpawnFloatingText(TEXT("SCHILD RETTUNG!"), 0.0f, ClimberPawn->PosZ + 70.0f, FLinearColor(0.0f, 0.961f, 0.831f), 17);
			UnlockAchievementById(TEXT("shield_survivor"));
		}
		else
		{
			TriggerAbyssFallDeath();
		}
	}
}

void AVTTowerGameMode::Update3DEnvironmentAndCamera(float DeltaTime)
{
	const FVTBiomeInfo CurBiome = GetBiomeForFloor(MaxFloor);
	const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	// Beat pulse generator synchronized with CurrentBpm
	const float SecondsPerBeat = 60.0f / FMath::Max(80.0f, float(CurrentBpm));
	SynthBeatTimer += DeltaTime;
	if (SynthBeatTimer >= SecondsPerBeat)
	{
		SynthBeatTimer -= SecondsPerBeat;
		BeatPulse = 1.0f;
	}

	// Update Climber 3D visuals
	if (ClimberPawn)
	{
		const FVTFloorScaling Scaling = GetFloorScaling(MaxFloor);
		const float MaxVx = 9.6f * Scaling.SpeedMult;
		const float SpeedRatio = FMath::Clamp(FMath::Abs(ClimberPawn->VelX) / FMath::Max(3.5f, MaxVx), 0.0f, 1.0f);
		const bool bNearWallReady = !ClimberPawn->bWallBounceUsed &&
			(FMath::Abs(ClimberPawn->PosX) >= PlayfieldHalfWidthCm - 60.0f) &&
			(FMath::Abs(ClimberPawn->VelX) >= 3.0f * BaseSpeedRatio);
		const bool bJumpGuide = ActiveSaveGame ? ActiveSaveGame->HudConfig.bJumpGuide : true;

		if (TowerRunState == EVTGameState::Menu)
		{
			// 3D Showroom articulated idle pose & rotation in Main Menu & Character Select Stage
			MenuShowroomAngle += DeltaTime * (bInCharacterSelectStage ? 52.0f : 35.0f);
			ClimberPawn->UpdateShowroomStagePose(TimeSec, MenuShowroomAngle, bInCharacterSelectStage);

			// Hide lower foreground platforms during Character Select Stage so the 3D Climber + Pedestal is 100% unobstructed!
			for (int32 i = 0; i < FMath::Min(3, Platforms.Num()); ++i)
			{
				if (Platforms[i]) Platforms[i]->SetActorHiddenInGame(bInCharacterSelectStage);
			}
		}
		else
		{
			for (int32 i = 0; i < FMath::Min(3, Platforms.Num()); ++i)
			{
				if (Platforms[i] && !Platforms[i]->bBroken) Platforms[i]->SetActorHiddenInGame(false);
			}
			ClimberPawn->UpdateVisualState(DeltaTime, bJumpGuide, SpeedRatio, bNearWallReady);
		}
	}

	// Update Portrait 3D CineCamera with Dynamic Perspective, Shake & FOV
	if (TowerCamera && TowerCamera->GetCameraComponent())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (PC->GetViewTarget() != TowerCamera)
			{
				PC->SetViewTarget(TowerCamera);
			}
		}

		const float ShakeX = (ScreenShake > 0.2f) ? FMath::RandRange(-1.0f, 1.0f) * ScreenShake * 2.2f : 0.0f;
		const float ShakeZ = (ScreenShake > 0.2f) ? FMath::RandRange(-1.0f, 1.0f) * ScreenShake * 2.2f : 0.0f;

		if (TowerRunState == EVTGameState::Menu)
		{
			if (BootFlowState == EVTBootFlowState::TeaserIntro)
			{
				// Cinematic 3D Teaser Swoop up the illuminated Lumen Neon Tower toward the Hero Pedestal!
				const float IntroAlpha = FMath::Clamp((BootTimerSec - 2.6f) / 5.4f, 0.0f, 1.0f);
				const float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, IntroAlpha);
				const float SwoopZ = FMath::Lerp(960.0f, 220.0f, SmoothAlpha);
				const float SwoopY = FMath::Lerp(-1180.0f, -720.0f, SmoothAlpha);
				const float SwoopX = FMath::Sin(IntroAlpha * PI * 2.0f) * 140.0f * (1.0f - SmoothAlpha);
				const float SwoopPitch = FMath::Lerp(-14.0f, 8.5f, SmoothAlpha);
				TowerCamera->SetActorLocation(FVector(SwoopX, SwoopY, SwoopZ));
				TowerCamera->SetActorRotation(FRotator(SwoopPitch, 90.0f - SwoopX * 0.05f, FMath::Sin(IntroAlpha * PI) * 4.5f));
				TowerCamera->GetCameraComponent()->SetFieldOfView(FMath::Lerp(78.0f, 64.0f, SmoothAlpha));
			}
			else if (bInCharacterSelectStage)
			{
				// Framed squarely in the open middle 3D Showroom Stage Window (completely unobstructed by Top Card or Bottom Roster!)
				const float SubtleSwayX = FMath::Sin(TimeSec * 0.8f) * 4.0f;
				TowerCamera->SetActorLocation(FVector(SubtleSwayX, -245.0f, 46.0f));
				TowerCamera->SetActorRotation(FRotator(0.0f, 90.0f - SubtleSwayX * 0.03f, 0.0f));
				TowerCamera->GetCameraComponent()->SetFieldOfView(48.0f);
			}
			else
			{
				// Focused Hero Showcase Podium Camera in Lobby
				const float OrbitX = FMath::Sin(TimeSec * 0.45f) * 35.0f;
				TowerCamera->SetActorLocation(FVector(OrbitX * 0.35f, -480.0f, 75.0f));
				TowerCamera->SetActorRotation(FRotator(-2.5f, 90.0f, 0.0f));
				TowerCamera->GetCameraComponent()->SetFieldOfView(56.0f);
			}
		}
		else
		{
			const float DynamicRoll = ClimberPawn ? FMath::Clamp(ClimberPawn->VelX * 0.18f, -2.5f, 2.5f) : 0.0f;
			const float DynamicFov = 68.0f + FMath::Clamp(ClimberPawn ? FMath::Max(0.0f, ClimberPawn->VelZ) * 0.15f : 0.0f, 0.0f, 6.0f);
			TowerCamera->SetActorLocation(FVector(ShakeX * 0.5f, -920.0f, CameraZ + ShakeZ));
			TowerCamera->SetActorRotation(FRotator(4.0f, 90.0f, DynamicRoll));
			TowerCamera->GetCameraComponent()->SetFieldOfView(DynamicFov);
		}
	}

	// Scroll 3D Continuous Megastructure Shaft (Pillars, Back Wall & Neon Ribs) infinitely around CameraZ
	const float BaseWallZ = FMath::FloorToFloat((CameraZ - 800.0f) / 400.0f) * 400.0f;
	for (int32 i = 0; i < LeftWallSegments.Num(); ++i)
	{
		const float SegZ = BaseWallZ + float(i) * 400.0f;
		if (LeftWallPillars.IsValidIndex(i) && LeftWallPillars[i])
		{
			LeftWallPillars[i]->SetWorldLocation(FVector(-PlayfieldHalfWidthCm - 32.0f, 0.0f, SegZ));
		}
		if (LeftWallSegments[i])
		{
			LeftWallSegments[i]->SetWorldLocation(FVector(-PlayfieldHalfWidthCm - 12.0f, 0.0f, SegZ));
		}
		if (RightWallPillars.IsValidIndex(i) && RightWallPillars[i])
		{
			RightWallPillars[i]->SetWorldLocation(FVector(PlayfieldHalfWidthCm + 32.0f, 0.0f, SegZ));
		}
		if (RightWallSegments[i])
		{
			RightWallSegments[i]->SetWorldLocation(FVector(PlayfieldHalfWidthCm + 12.0f, 0.0f, SegZ));
		}
		if (BackWallPanels.IsValidIndex(i) && BackWallPanels[i])
		{
			BackWallPanels[i]->SetWorldLocation(FVector(0.0f, 260.0f, SegZ));
		}
	}

	// Wall Rail Dynamic Color matching Active Theme (Matte natural walls for Papercraft/Candy/Toybox/Dungeon!)
	const FVTGameThemeInfo ActiveTheme = GetThemeInfo(bInThemeSelectStage ? PreviewThemeId : SelectedThemeId);
	const bool bNeonWallTheme =
		(ActiveTheme.Id == EVTGameThemeId::CyberpunkDefault ||
		 ActiveTheme.Id == EVTGameThemeId::RetroSynthwave ||
		 ActiveTheme.Id == EVTGameThemeId::DeepSpaceOrbit ||
		 ActiveTheme.Id == EVTGameThemeId::IndustrialForge);
	const bool bBounceReady = ClimberPawn ? !ClimberPawn->bWallBounceUsed : true;
	const FLinearColor BaseRailCol = bNeonWallTheme
		? (bBounceReady ? ActiveTheme.WallEmissiveColor : FLinearColor(0.937f, 0.267f, 0.267f))
		: ActiveTheme.WallBaseColor;
	const float BaseWallEmiss = bNeonWallTheme ? (6.0f + BeatPulse * 8.0f) : 0.26f;
	if (LeftWallMID)
	{
		const FLinearColor LCol = FMath::Lerp(BaseRailCol, FLinearColor::White, FMath::Clamp(LeftWallFlash, 0.0f, 1.0f));
		LeftWallMID->SetVectorParameterValue(TEXT("BaseColor"), LCol);
		LeftWallMID->SetVectorParameterValue(TEXT("EmissiveColor"), LCol * (BaseWallEmiss + LeftWallFlash * (bNeonWallTheme ? 20.0f : 2.0f)));
	}
	if (RightWallMID)
	{
		const FLinearColor RCol = FMath::Lerp(BaseRailCol, FLinearColor::White, FMath::Clamp(RightWallFlash, 0.0f, 1.0f));
		RightWallMID->SetVectorParameterValue(TEXT("BaseColor"), RCol);
		RightWallMID->SetVectorParameterValue(TEXT("EmissiveColor"), RCol * (BaseWallEmiss + RightWallFlash * (bNeonWallTheme ? 20.0f : 2.0f)));
	}

	// Update 16-Band Dynamic Chromatic Audio Visualizer / Music Spectrum (Only visible in Cyber/Synthwave/Space themes)
	const float BarWidthCm = (PlayfieldHalfWidthCm * 2.0f) / 16.0f;
	for (int32 i = 0; i < EqualizerBarMeshes.Num(); ++i)
	{
		if (!EqualizerBarMeshes[i]) continue;
		EqualizerBarMeshes[i]->SetVisibility(bNeonWallTheme);
		if (!bNeonWallTheme) continue;

		// Harmonic frequency bands & high-energy beat reaction
		const float FreqSpread = FMath::Sin(TimeSec * 10.0f + float(i) * 0.65f) * 0.5f + 0.5f;
		const float BassKick = (i < 5) ? (BeatPulse * 1.5f) : (BeatPulse * 0.75f);
		const float HighFlutter = (i > 10) ? (FMath::Sin(TimeSec * 24.0f + float(i) * 1.3f) * 0.25f + 0.25f) : 0.0f;
		const float Level = FMath::Clamp(FreqSpread * 0.38f + BassKick * 0.62f + HighFlutter * 0.25f, 0.10f, 1.35f);

		// Sleek low-profile footer spectrum: 35cm to 190cm max height, anchored cleanly at bottom of screen
		const float BarHeightCm = Level * 180.0f * (float(CurrentBpm) / 115.0f);
		const float BarX = -PlayfieldHalfWidthCm + (float(i) + 0.5f) * BarWidthCm;
		const float BarZ = CameraZ - 660.0f + BarHeightCm * 0.5f;

		EqualizerBarMeshes[i]->SetWorldLocation(FVector(BarX, 260.0f, BarZ));
		EqualizerBarMeshes[i]->SetWorldScale3D(FVector((BarWidthCm - 12.0f) / 100.0f, 0.20f, BarHeightCm / 100.0f));

		if (EqualizerBarMIDs.IsValidIndex(i) && EqualizerBarMIDs[i])
		{
			// Dynamic chromatic neon spectrum hue (Cyan -> Electric Blue -> Magenta -> Hot Pink -> Gold -> Lime)
			const float Hue = FMath::Fmod(float(i) / 16.0f * 290.0f + TimeSec * 28.0f, 360.0f);
			const FLinearColor BarColor = FLinearColor(Hue, 0.96f, 1.0f).HSVToLinearRGB();
			const float EmissiveBoost = 4.0f + BeatPulse * 12.0f;

			EqualizerBarMIDs[i]->SetVectorParameterValue(TEXT("BaseColor"), BarColor);
			EqualizerBarMIDs[i]->SetVectorParameterValue(TEXT("EmissiveColor"), BarColor * EmissiveBoost);
		}
	}

	// Update Active Theme Scenery Props (Gears rotate, Jellyfish bob, Torches flicker, Sun pulses)
	UpdateThemeScenery(DeltaTime, TimeSec);

	// Update Deep 3D Background Megastructure Pillars for dramatic 3D parallax depth
	const float BaseBgZ = FMath::FloorToFloat((CameraZ - 1200.0f) / 600.0f) * 600.0f;
	for (int32 i = 0; i < BgMegastructurePillars.Num(); ++i)
	{
		if (!BgMegastructurePillars[i]) continue;
		BgMegastructurePillars[i]->SetVisibility(bNeonWallTheme);
		if (!bNeonWallTheme) continue;
		const int32 Row = i / 2;
		const float SideSign = (i % 2 == 0) ? -1.0f : 1.0f;
		const float PillarX = SideSign * 220.0f;
		const float PillarY = 580.0f + float(i % 3) * 140.0f;
		const float PillarZ = BaseBgZ + float(Row) * 600.0f;
		BgMegastructurePillars[i]->SetWorldLocation(FVector(PillarX, PillarY, PillarZ));
		BgMegastructurePillars[i]->SetWorldScale3D(FVector(1.4f, 1.4f, 1.5f));
	}

	if (TowerMainLight)
	{
		TowerMainLight->SetWorldLocation(FVector(0.0f, -350.0f, CameraZ + 120.0f));
		TowerMainLight->SetLightColor(ActiveTheme.LightColor);
		TowerMainLight->SetIntensity(ActiveTheme.LightIntensity);
	}

	// Update 3D Shattering Ice Shards
	for (FVTIceShardSim& Shard : IceShardPool)
	{
		if (Shard.Life > 0.0f && Shard.MeshComp)
		{
			Shard.Life -= DeltaTime * 1.6f;
			if (Shard.Life <= 0.0f)
			{
				Shard.Life = 0.0f;
				Shard.MeshComp->SetVisibility(false);
			}
			else
			{
				Shard.Vel.Z -= 980.0f * DeltaTime;
				Shard.Pos += Shard.Vel * DeltaTime;
				Shard.Rot += Shard.RotVel * DeltaTime;
				Shard.MeshComp->SetWorldLocation(Shard.Pos);
				Shard.MeshComp->SetWorldRotation(Shard.Rot);
				Shard.MeshComp->SetWorldScale3D(FVector(Shard.Life * 0.9f));
			}
		}
	}

	// Update 3D Shockwaves
	for (FVTShockwaveSim& Sw : ShockwavePool)
	{
		if (Sw.Life > 0.0f && Sw.MeshComp)
		{
			Sw.Life -= DeltaTime * 3.2f;
			if (Sw.Life <= 0.0f)
			{
				Sw.Life = 0.0f;
				Sw.MeshComp->SetVisibility(false);
			}
			else
			{
				Sw.Radius = FMath::Lerp(Sw.Radius, Sw.MaxRadius, DeltaTime * 12.0f);
				const float ScaleXY = Sw.Radius / 50.0f;
				Sw.MeshComp->SetWorldLocation(Sw.Pos);
				Sw.MeshComp->SetWorldRotation(FRotator(90.0f, 0.0f, 0.0f));
				Sw.MeshComp->SetWorldScale3D(FVector(ScaleXY, ScaleXY, 0.04f));
			}
		}
	}

	// Update Floating Texts
	for (int32 i = FloatingTexts.Num() - 1; i >= 0; --i)
	{
		FloatingTexts[i].WorldPos.Z += 140.0f * DeltaTime;
		FloatingTexts[i].Life -= DeltaTime * 1.35f;
		if (FloatingTexts[i].Life <= 0.0f)
		{
			FloatingTexts.RemoveAt(i);
		}
	}
}

void AVTTowerGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float ClampedDelta = FMath::Min(0.1f, DeltaTime);

	// Advance Boot Sequence (Unreal Engine 5.8 Splash -> 3D Cinematic Teaser Intro -> Account Login Gate)
	if (BootFlowState != EVTBootFlowState::Ready)
	{
		BootTimerSec += ClampedDelta;
		if (BootFlowState == EVTBootFlowState::EngineSplash && BootTimerSec >= 2.6f)
		{
			BootFlowState = EVTBootFlowState::TeaserIntro;
		}
		else if (BootFlowState == EVTBootFlowState::TeaserIntro && BootTimerSec >= 8.0f)
		{
			BootFlowState = EVTBootFlowState::AccountGate;
		}
	}

	PhysicsTimeAccumulator += ClampedDelta;

	while (PhysicsTimeAccumulator >= FixedStepSeconds)
	{
		StepFixedPhysics60Hz();
		PhysicsTimeAccumulator -= FixedStepSeconds;
	}

	// Cinematic Abyss Fall & Death Sequence Simulation
	if (bIsDeathSequenceActive)
	{
		DeathSequenceTimer += ClampedDelta;

		if (ClimberPawn)
		{
			ClimberPawn->VelZ -= 980.0f * ClampedDelta;
			ClimberPawn->PosZ += ClimberPawn->VelZ * ClampedDelta;
			ClimberPawn->FlipAngleRad += 14.0f * ClampedDelta;
			ClimberPawn->PlayAbyssFallPose(FMath::Clamp(DeathSequenceTimer / 1.8f, 0.0f, 1.0f), ClampedDelta);
		}

		// At 0.60s: slam "DEAD" popup onto screen with heavy impact SFX!
		if (DeathSequenceTimer >= 0.60f && !bDeadPopupShown)
		{
			bDeadPopupShown = true;
			PlayTowerSFX(TEXT("DeadImpact"), 1.0f, 1.45f);
			if (AVTPlayerController* PC = Cast<AVTPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
			{
				PC->TriggerHapticFeedback(100);
			}
		}

		if (bDeadPopupShown)
		{
			DeadPopupAlpha = FMath::FInterpTo(DeadPopupAlpha, 1.0f, ClampedDelta, 10.0f);
		}

		// At 1.95s: finalize death and open Game Over
		if (DeathSequenceTimer >= 1.95f)
		{
			bIsDeathSequenceActive = false;
			EndCurrentRun(TEXT("IN DEN ABGRUND GESTÜRZT"), TEXT("Du bist in die Tiefe des Eis-Turms gefallen!"));
		}
	}

	if (HighScoreCelebrationTimer > 0.0f) HighScoreCelebrationTimer = FMath::Max(0.0f, HighScoreCelebrationTimer - DeltaTime);
	if (ScorePulseAnim > 0.0f) ScorePulseAnim = FMath::Max(0.0f, ScorePulseAnim - DeltaTime * 3.5f);

	// Organic Synthwave Music Pitch Progression: Chilled slow start with subtle, gradual floor climb
	if (TowerRunState == EVTGameState::Play && !bIsDeathSequenceActive)
	{
		const float FloorProgression = float(MaxFloor - StartFloor) * 0.0016f;
		const float ComboUrgency = FMath::Min(0.06f, float(CurrentCombo) * 0.0025f);
		const float HurryUrgency = float(HurryLevel) * 0.025f;
		TargetMusicPitch = FMath::Clamp(0.80f + FloorProgression + ComboUrgency + HurryUrgency, 0.78f, 1.45f);
		CurrentSmoothedMusicPitch = FMath::FInterpTo(CurrentSmoothedMusicPitch, TargetMusicPitch, ClampedDelta, 0.65f);
	}
	else
	{
		TargetMusicPitch = 0.84f;
		CurrentSmoothedMusicPitch = FMath::FInterpTo(CurrentSmoothedMusicPitch, TargetMusicPitch, ClampedDelta, 1.2f);
	}

	if (HurryBannerTimerSec > 0.0f) HurryBannerTimerSec = FMath::Max(0.0f, HurryBannerTimerSec - DeltaTime);
	if (ToastTimerSec > 0.0f) ToastTimerSec = FMath::Max(0.0f, ToastTimerSec - DeltaTime);
	if (AnnouncerTimerSec > 0.0f) AnnouncerTimerSec = FMath::Max(0.0f, AnnouncerTimerSec - DeltaTime);
	if (LevelUpBannerTimerSec > 0.0f) LevelUpBannerTimerSec = FMath::Max(0.0f, LevelUpBannerTimerSec - DeltaTime);
	if (XpGainPulse > 0.0f) XpGainPulse = FMath::Max(0.0f, XpGainPulse - DeltaTime * 3.5f);
	if (ComboHitPulse > 0.0f) ComboHitPulse = FMath::Max(0.0f, ComboHitPulse - DeltaTime * 4.0f);

	if (TowerRunState == EVTGameState::GameOver && ReviveSecondsRemaining > 0.0f)
	{
		ReviveSecondsRemaining = FMath::Max(0.0f, ReviveSecondsRemaining - DeltaTime);
	}

	PumpProceduralAudioStream(ClampedDelta);
	Update3DEnvironmentAndCamera(DeltaTime);
}

void AVTTowerGameMode::SkipToTeaserIntro()
{
	BootFlowState = EVTBootFlowState::TeaserIntro;
	BootTimerSec = 2.6f;
}

void AVTTowerGameMode::SkipToAccountGate()
{
	BootFlowState = EVTBootFlowState::AccountGate;
	BootTimerSec = 8.0f;
}

void AVTTowerGameMode::CompleteAccountLogin(const FString& ProviderKey, const FString& DisplayName, const FString& OptionalUsername)
{
	if (ActiveSaveGame)
	{
		ActiveSaveGame->ActiveAuthProvider = DisplayName;
		if (!OptionalUsername.IsEmpty())
		{
			ActiveSaveGame->PlayerNickname = OptionalUsername;
		}
		if (ProviderKey != TEXT("guest") && !ActiveSaveGame->LinkedAuthProviders.Contains(ProviderKey))
		{
			ActiveSaveGame->LinkedAuthProviders.AddUnique(ProviderKey);
			AwardNeonCores(500, FString::Printf(TEXT("%s Account-Bonus"), *DisplayName));
		}
		else
		{
			TriggerToast(FString::Printf(TEXT("Willkommen zurueck via %s!"), *DisplayName));
		}
		SaveGameToDisk();
	}
	BootFlowState = EVTBootFlowState::Ready;
}

void AVTTowerGameMode::ReplayTeaserIntro()
{
	TowerRunState = EVTGameState::Menu;
	BootFlowState = EVTBootFlowState::TeaserIntro;
	BootTimerSec = 2.6f;
}

void AVTTowerGameMode::SpawnFloatingText(const FString& Txt, float X, float Z, const FLinearColor& Color, int32 Size)
{
	FVTFloatingTextEntry Entry;
	Entry.Text = Txt;
	Entry.WorldPos = FVector(FMath::Clamp(X, -PlayfieldHalfWidthCm + 80.0f, PlayfieldHalfWidthCm - 80.0f), 0.0f, Z);
	Entry.Color = Color;
	Entry.FontSize = Size;
	Entry.Life = 1.0f;
	Entry.InitialLife = 1.0f;
	FloatingTexts.Add(Entry);
}

void AVTTowerGameMode::SpawnIceShatter3D(AVTPlatformActor* Plat, const FLinearColor& BiomeColor)
{
	if (!Plat || IceShardPool.Num() == 0) return;
	const int32 Pieces = 7;
	const float StepW = Plat->PlatformWidth / float(Pieces);
	const float LeftX = Plat->PlatformX - Plat->PlatformWidth * 0.5f;

	for (int32 i = 0; i < Pieces; ++i)
	{
		FVTIceShardSim& Shard = IceShardPool[NextIceShardIndex];
		NextIceShardIndex = (NextIceShardIndex + 1) % IceShardPool.Num();

		Shard.Pos = FVector(LeftX + (float(i) + 0.5f) * StepW, 0.0f, Plat->PlatformZ - 8.0f);
		Shard.Vel = FVector(FMath::RandRange(-220.0f, 220.0f), FMath::RandRange(-90.0f, 90.0f), FMath::RandRange(60.0f, 240.0f));
		Shard.Rot = FRotator(FMath::RandRange(0.0f, 360.0f), FMath::RandRange(0.0f, 360.0f), FMath::RandRange(0.0f, 360.0f));
		Shard.RotVel = FRotator(FMath::RandRange(-360.0f, 360.0f), FMath::RandRange(-360.0f, 360.0f), FMath::RandRange(-360.0f, 360.0f));
		Shard.Life = 1.0f;
		if (Shard.MID)
		{
			Shard.MID->SetVectorParameterValue(TEXT("BaseColor"), BiomeColor);
			Shard.MID->SetVectorParameterValue(TEXT("EmissiveColor"), BiomeColor * 6.0f);
		}
		if (Shard.MeshComp)
		{
			Shard.MeshComp->SetWorldLocation(Shard.Pos);
			Shard.MeshComp->SetVisibility(true);
		}
	}
}

void AVTTowerGameMode::SpawnShockwave3D(float X, float Z, float MaxRadiusCm, const FLinearColor& Color)
{
	if (ShockwavePool.Num() == 0) return;
	FVTShockwaveSim& Sw = ShockwavePool[NextShockwaveIndex];
	NextShockwaveIndex = (NextShockwaveIndex + 1) % ShockwavePool.Num();

	Sw.Pos = FVector(X, -15.0f, Z);
	Sw.Radius = 16.0f;
	Sw.MaxRadius = MaxRadiusCm;
	Sw.Life = 1.0f;
	if (Sw.MID)
	{
		Sw.MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Sw.MID->SetVectorParameterValue(TEXT("EmissiveColor"), Color * 14.0f);
	}
	if (Sw.MeshComp)
	{
		Sw.MeshComp->SetWorldLocation(Sw.Pos);
		Sw.MeshComp->SetVisibility(true);
	}
}

void AVTTowerGameMode::TriggerToast(const FString& Msg)
{
	LatestToastMessage = Msg;
	ToastTimerSec = 3.0f;
}

void AVTTowerGameMode::AwardNeonCores(int32 Amount, const FString& Reason)
{
	NeonCores += Amount;
	if (ActiveSaveGame)
	{
		ActiveSaveGame->NeonCores = NeonCores;
		if (Amount > 0)
		{
			ActiveSaveGame->TotalCoresEarned += Amount;
			if (ActiveSaveGame->TotalCoresEarned >= 15) UnlockAchievementById(TEXT("magnetar"));
			if (ActiveSaveGame->TotalCoresEarned >= 100) UnlockAchievementById(TEXT("core_tycoon"));
		}
	}
	SaveGameToDisk();
	if (Amount > 0)
	{
		TriggerToast(FString::Printf(TEXT("+%d CORES: %s"), Amount, *Reason));
	}
}

void AVTTowerGameMode::RecordQuestProgress(const FString& QuestId, int32 Amount)
{
	if (!ActiveSaveGame) return;
	for (FVTQuest& Q : ActiveSaveGame->DailyQuests)
	{
		if (Q.Id == QuestId && !Q.bClaimed)
		{
			const int32 OldProg = Q.Progress;
			Q.Progress = FMath::Min(Q.Target, Q.Progress + Amount);
			if (OldProg < Q.Target && Q.Progress >= Q.Target)
			{
				TriggerToast(FString::Printf(TEXT("QUEST ERFUELLT: %s!"), *Q.Title));
			}
		}
	}
	SaveGameToDisk();
}

bool AVTTowerGameMode::ClaimQuestReward(int32 QuestIndex)
{
	if (!ActiveSaveGame || !ActiveSaveGame->DailyQuests.IsValidIndex(QuestIndex)) return false;
	FVTQuest& Q = ActiveSaveGame->DailyQuests[QuestIndex];
	if (Q.bClaimed || Q.Progress < Q.Target) return false;

	Q.bClaimed = true;
	AwardNeonCores(Q.RewardCores, FString::Printf(TEXT("Quest: %s"), *Q.Title));
	return true;
}

void AVTTowerGameMode::UnlockAchievementById(const FString& AchievementId)
{
	if (!ActiveSaveGame || ActiveSaveGame->UnlockedAchievements.Contains(AchievementId)) return;

	for (FVTAchievement& Ach : Achievements)
	{
		if (Ach.Id == AchievementId)
		{
			Ach.bUnlocked = true;
			ActiveSaveGame->UnlockedAchievements.AddUnique(AchievementId);
			AwardNeonCores(Ach.RewardCores, FString::Printf(TEXT("Trophy: %s"), *Ach.Title));
			break;
		}
	}
}

bool AVTTowerGameMode::ClaimDailyStreakReward()
{
	if (!ActiveSaveGame) return false;
	const FString TodayStr = FDateTime::Now().ToString(TEXT("%Y-%m-%d"));
	if (ActiveSaveGame->LastStreakClaimDate == TodayStr) return false;

	const int32 Day = FMath::Clamp(ActiveSaveGame->StreakDayCount, 1, 7);
	int32 CoreReward = 5;
	if (Day == 2) CoreReward = 10;
	else if (Day == 3) CoreReward = 15;
	else if (Day == 4) CoreReward = 20;
	else if (Day == 5) CoreReward = 25;
	else if (Day == 6) { ShieldCount += 1; CoreReward = 30; }
	else if (Day == 7) CoreReward = 100;

	ActiveSaveGame->LastStreakClaimDate = TodayStr;
	ActiveSaveGame->StreakDayCount = (Day % 7) + 1;
	AwardNeonCores(CoreReward, FString::Printf(TEXT("Tag %d Login-Streak"), Day));
	return true;
}

FString AVTTowerGameMode::SpinFortuneWheel(bool bFreeSpin)
{
	UnlockAchievementById(TEXT("lucky_spinner"));
	if (ActiveSaveGame && bFreeSpin)
	{
		ActiveSaveGame->LastWheelSpinDate = FDateTime::Now().ToString(TEXT("%Y-%m-%d"));
	}

	const int32 Sector = FMath::RandRange(0, 7);
	switch (Sector)
	{
	case 0: AwardNeonCores(5, TEXT("Lucky Spin (+5 Cores)")); return TEXT("+5 NEON CORES");
	case 1: SpringJumpsLeft += 3; TriggerToast(TEXT("+3x SPRING BOOTS ERHALTEN!")); return TEXT("3x SPRING BOOTS");
	case 2: AwardNeonCores(15, TEXT("Lucky Spin (+15 Cores)")); return TEXT("+15 NEON CORES");
	case 3: AwardNeonCores(20, TEXT("Lucky Spin (Revive Bonus)")); return TEXT("+20 REVIVE CORES");
	case 4: ShieldCount += 1; TriggerToast(TEXT("AEGIS SCHILD ERHALTEN!")); return TEXT("AEGIS SCHILD");
	case 5: AwardNeonCores(30, TEXT("Lucky Spin (+30 Cores)")); return TEXT("+30 NEON CORES");
	case 6: MagnetFrames += 540; TriggerToast(TEXT("CORE MAGNET ERHALTEN!")); return TEXT("CORE MAGNET");
	default: AwardNeonCores(100, TEXT("MEGA JACKPOT (100 CORES)!")); return TEXT("100 CORES JACKPOT!");
	}
}

bool AVTTowerGameMode::IsClimberUnlocked(EVTClimberId ClimberId) const
{
	if (ClimberId == EVTClimberId::KidFlex) return true;
	if (!ActiveSaveGame) return false;
	return ActiveSaveGame->bIsVip || ActiveSaveGame->UnlockedClimbers.Contains(ClimberId);
}

int32 AVTTowerGameMode::GetUnlockedClimberCount() const
{
	if (!ActiveSaveGame) return 1;
	if (ActiveSaveGame->bIsVip) return Climbers.Num();
	int32 Count = 0;
	for (const FVTClimberInfo& C : Climbers)
	{
		if (IsClimberUnlocked(C.Id)) Count++;
	}
	return FMath::Max(1, Count);
}

void AVTTowerGameMode::PreviewClimberOnStage(EVTClimberId ClimberId)
{
	PreviewClimberId = ClimberId;
	const FVTClimberInfo Info = GetClimberInfo(ClimberId);
	const bool bUnlocked = IsClimberUnlocked(ClimberId);
	if (ClimberPawn)
	{
		// Unlocked characters show full 3D model & colors; locked characters show pitch-black 3D silhouette!
		ClimberPawn->ApplyClimberVisuals(Info, GetTrailInfo(SelectedTrailId).Color, !bUnlocked);
	}
	if (bUnlocked)
	{
		SelectedClimberId = ClimberId;
		if (ActiveSaveGame) ActiveSaveGame->SelectedClimber = ClimberId;
		SaveGameToDisk();
	}
}

bool AVTTowerGameMode::BuyOrEquipClimber(EVTClimberId ClimberId)
{
	if (!ActiveSaveGame) return false;
	const FVTClimberInfo Info = GetClimberInfo(ClimberId);
	const bool bUnlocked = IsClimberUnlocked(ClimberId);

	PreviewClimberId = ClimberId;

	if (!bUnlocked)
	{
		if (NeonCores < Info.CostCores)
		{
			if (ClimberPawn)
			{
				ClimberPawn->ApplyClimberVisuals(Info, GetTrailInfo(SelectedTrailId).Color, true);
			}
			TriggerToast(FString::Printf(TEXT("Noch %d Neon Cores benoetigt fuer %s!"), Info.CostCores - NeonCores, *Info.Name));
			return false;
		}
		NeonCores -= Info.CostCores;
		ActiveSaveGame->NeonCores = NeonCores;
		ActiveSaveGame->UnlockedClimbers.AddUnique(ClimberId);
		TriggerToast(FString::Printf(TEXT("NEUER CLIMBER FREIGESCHALTET: %s (#%02d/20)!"), *Info.Name, Info.RosterNumber));
	}
	else
	{
		TriggerToast(FString::Printf(TEXT("Climber %s (#%02d/20) ausgeruestet!"), *Info.Name, Info.RosterNumber));
	}

	SelectedClimberId = ClimberId;
	ActiveSaveGame->SelectedClimber = ClimberId;
	if (ClimberPawn)
	{
		ClimberPawn->ApplyClimberVisuals(Info, GetTrailInfo(SelectedTrailId).Color, false);
	}
	SaveGameToDisk();
	return true;
}

bool AVTTowerGameMode::BuyOrEquipTrail(const FString& TrailId)
{
	if (!ActiveSaveGame) return false;
	const FVTTrailInfo Info = GetTrailInfo(TrailId);
	const bool bUnlocked = ActiveSaveGame->bIsVip || ActiveSaveGame->UnlockedTrails.Contains(TrailId) || Info.CostCores == 0;

	if (!bUnlocked)
	{
		if (NeonCores < Info.CostCores)
		{
			TriggerToast(TEXT("Nicht genuegend Neon Cores!"));
			return false;
		}
		NeonCores -= Info.CostCores;
		ActiveSaveGame->NeonCores = NeonCores;
		ActiveSaveGame->UnlockedTrails.AddUnique(TrailId);
	}

	SelectedTrailId = TrailId;
	ActiveSaveGame->SelectedTrail = TrailId;
	if (ClimberPawn)
	{
		ClimberPawn->ApplyClimberVisuals(GetClimberInfo(SelectedClimberId), Info.Color);
	}
	SaveGameToDisk();
	TriggerToast(FString::Printf(TEXT("Trail %s ausgeruestet!"), *Info.Name));
	return true;
}

void AVTTowerGameMode::ToggleVipStatus()
{
	if (!ActiveSaveGame) return;
	ActiveSaveGame->bIsVip = !ActiveSaveGame->bIsVip;
	SaveGameToDisk();
	TriggerToast(ActiveSaveGame->bIsVip ? TEXT("VIP PASS AKTIVIERT (ALLE 20 CHARS FREI)!") : TEXT("VIP PASS DEAKTIVIERT"));
}

bool AVTTowerGameMode::LinkSocialAccount(const FString& ProviderKey, const FString& DisplayName)
{
	if (!ActiveSaveGame) return false;
	if (ActiveSaveGame->LinkedAuthProviders.Contains(ProviderKey))
	{
		ActiveSaveGame->ActiveAuthProvider = DisplayName;
		SaveGameToDisk();
		TriggerToast(FString::Printf(TEXT("Bereits mit %s verknuepft (Aktiv gesetzt)!"), *DisplayName));
		return false;
	}

	ActiveSaveGame->LinkedAuthProviders.AddUnique(ProviderKey);
	ActiveSaveGame->ActiveAuthProvider = DisplayName;
	if (ProviderKey == TEXT("googlePlay")) ActiveSaveGame->bLinkedGooglePlay = true;
	else if (ProviderKey == TEXT("facebook")) ActiveSaveGame->bLinkedFacebook = true;
	else if (ProviderKey == TEXT("instagram")) ActiveSaveGame->bLinkedInstagram = true;
	else if (ProviderKey == TEXT("email")) ActiveSaveGame->bLinkedEmail = true;

	AwardNeonCores(500, FString::Printf(TEXT("%s Cloud-Sync Bonus"), *DisplayName));
	return true;
}

void AVTTowerGameMode::SetControlModeDirect(EVTControlMode NewMode)
{
	CurrentControlMode = NewMode;
	if (ActiveSaveGame) ActiveSaveGame->ControlMode = CurrentControlMode;
	SaveGameToDisk();

	FString ModeName = TEXT("VIRTUELLER JOYSTICK");
	if (CurrentControlMode == EVTControlMode::ClassicButtons) ModeName = TEXT("KLASSISCHE D-PAD TASTEN");
	else if (CurrentControlMode == EVTControlMode::OneThumbSlide) ModeName = TEXT("1-DAUMEN SLIDE");
	else if (CurrentControlMode == EVTControlMode::FullscreenSplit) ModeName = TEXT("FULLSCREEN SPLIT");
	else if (CurrentControlMode == EVTControlMode::SwipeGestures) ModeName = TEXT("SWIPE & FLICK GESTEN");
	else if (CurrentControlMode == EVTControlMode::AutoBounce) ModeName = TEXT("AUTO-BOUNCE ZEN");
	TriggerToast(FString::Printf(TEXT("STEUERUNG: %s AKTIV"), *ModeName));
}

void AVTTowerGameMode::CycleControlMode()
{
	const int32 Next = (int32(CurrentControlMode) + 1) % 6;
	SetControlModeDirect(static_cast<EVTControlMode>(Next));
}

void AVTTowerGameMode::CycleSpeedPreset()
{
	if (FMath::IsNearlyEqual(BaseSpeedRatio, 0.60f, 0.05f)) BaseSpeedRatio = 0.80f;
	else if (FMath::IsNearlyEqual(BaseSpeedRatio, 0.80f, 0.05f)) BaseSpeedRatio = 1.00f;
	else BaseSpeedRatio = 0.60f;

	if (ActiveSaveGame) ActiveSaveGame->BaseSpeedRatio = BaseSpeedRatio;
	SaveGameToDisk();
	TriggerToast(FString::Printf(TEXT("TEMPO: %d%% SPEED AKTIV"), FMath::RoundToInt(BaseSpeedRatio * 100.0f)));
}

void AVTTowerGameMode::SelectClimberDirection(int32 StepDir)
{
	const int32 TotalChars = FMath::Max(1, Climbers.Num());
	const int32 CurIdx = int32(PreviewClimberId);
	const int32 NextIdx = (CurIdx + StepDir + TotalChars) % TotalChars;
	PreviewClimberOnStage(static_cast<EVTClimberId>(NextIdx));
}

FVTGameThemeInfo AVTTowerGameMode::GetThemeInfo(EVTGameThemeId Id) const
{
	for (const FVTGameThemeInfo& T : Themes)
	{
		if (T.Id == Id) return T;
	}
	return Themes.Num() > 0 ? Themes[0] : FVTGameThemeInfo();
}

bool AVTTowerGameMode::IsThemeUnlocked(EVTGameThemeId Id) const
{
	if (!ActiveSaveGame) return (Id == EVTGameThemeId::CyberpunkDefault);
	if (ActiveSaveGame->bIsVip) return true;
	const FVTGameThemeInfo Info = GetThemeInfo(Id);
	if (Info.bUnlockedInitially || Info.CostCores == 0) return true;
	return ActiveSaveGame->UnlockedThemes.Contains(Id);
}

int32 AVTTowerGameMode::GetUnlockedThemeCount() const
{
	int32 Count = 0;
	for (const FVTGameThemeInfo& T : Themes)
	{
		if (IsThemeUnlocked(T.Id)) Count++;
	}
	return Count;
}

void AVTTowerGameMode::PreviewThemeOnStage(EVTGameThemeId ThemeId)
{
	PreviewThemeId = ThemeId;
	SelectedThemeId = ThemeId;
	if (ActiveSaveGame)
	{
		ActiveSaveGame->SelectedTheme = ThemeId;
	}
	const FVTGameThemeInfo Info = GetThemeInfo(ThemeId);
	ApplyThemeVisuals(Info);
	SaveGameToDisk();
	TriggerToast(FString::Printf(TEXT("1:1 LEVEL-WELT #%02d/10: %s"), Info.ThemeNumber, *Info.Name));
}

bool AVTTowerGameMode::BuyOrEquipTheme(EVTGameThemeId ThemeId)
{
	PreviewThemeOnStage(ThemeId);
	PlayTowerSFX(TEXT("Click"), 1.1f);
	return true;
}

void AVTTowerGameMode::SelectThemeDirection(int32 StepDir)
{
	if (Themes.Num() == 0) return;
	int32 CurrentIdx = 0;
	for (int32 i = 0; i < Themes.Num(); ++i)
	{
		if (Themes[i].Id == SelectedThemeId)
		{
			CurrentIdx = i;
			break;
		}
	}
	const int32 NewIdx = (CurrentIdx + StepDir + Themes.Num()) % Themes.Num();
	PreviewThemeOnStage(Themes[NewIdx].Id);
	PlayTowerSFX(TEXT("Click"), 1.12f);
}

void AVTTowerGameMode::ApplyThemeVisuals(const FVTGameThemeInfo& ThemeInfo)
{
	UMaterialInterface* CustomNeonMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VelocityTower/Materials/M_NeonEmissive_Master.M_NeonEmissive_Master"));
	UMaterialInterface* FallbackMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInterface* BaseMat = CustomNeonMat ? CustomNeonMat : FallbackMat;

	const bool bNeonWallTheme =
		(ThemeInfo.Id == EVTGameThemeId::CyberpunkDefault ||
		 ThemeInfo.Id == EVTGameThemeId::RetroSynthwave ||
		 ThemeInfo.Id == EVTGameThemeId::DeepSpaceOrbit ||
		 ThemeInfo.Id == EVTGameThemeId::BioLumenArk ||
		 ThemeInfo.Id == EVTGameThemeId::IndustrialForge);

	// 1. Wall Emissive Rails & Accents
	if (LeftWallMID)
	{
		const FLinearColor RailCol = bNeonWallTheme ? ThemeInfo.PrimaryColor : ThemeInfo.WallBaseColor;
		LeftWallMID->SetVectorParameterValue(TEXT("BaseColor"), RailCol);
		LeftWallMID->SetVectorParameterValue(TEXT("EmissiveColor"), RailCol * (bNeonWallTheme ? 6.0f : 0.28f));
	}
	if (RightWallMID)
	{
		const FLinearColor RailCol = bNeonWallTheme ? ThemeInfo.PrimaryColor : ThemeInfo.WallBaseColor;
		RightWallMID->SetVectorParameterValue(TEXT("BaseColor"), RailCol);
		RightWallMID->SetVectorParameterValue(TEXT("EmissiveColor"), RailCol * (bNeonWallTheme ? 6.0f : 0.28f));
	}

	// 2. Structural Wall Pillars & Backplate (Warm Cardboard / Stone / Reef / Candy / Voxel / Cyber)
	const float BackEmissBoost = (ThemeInfo.Id == EVTGameThemeId::PapercraftToybox || ThemeInfo.Id == EVTGameThemeId::CandyLand || ThemeInfo.Id == EVTGameThemeId::WhimsicalToybox)
		? 0.34f
		: 0.16f;
	for (UStaticMeshComponent* Pillar : LeftWallPillars)
	{
		if (Pillar && BaseMat)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("BaseColor"), ThemeInfo.WallBaseColor);
				MID->SetVectorParameterValue(TEXT("Color"), ThemeInfo.WallBaseColor);
				MID->SetVectorParameterValue(TEXT("EmissiveColor"), ThemeInfo.WallBaseColor * BackEmissBoost);
				Pillar->SetMaterial(0, MID);
			}
		}
	}
	for (UStaticMeshComponent* Pillar : RightWallPillars)
	{
		if (Pillar && BaseMat)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("BaseColor"), ThemeInfo.WallBaseColor);
				MID->SetVectorParameterValue(TEXT("Color"), ThemeInfo.WallBaseColor);
				MID->SetVectorParameterValue(TEXT("EmissiveColor"), ThemeInfo.WallBaseColor * BackEmissBoost);
				Pillar->SetMaterial(0, MID);
			}
		}
	}
	for (UStaticMeshComponent* BackPanel : BackWallPanels)
	{
		if (BackPanel && BaseMat)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
			if (MID)
			{
				const FLinearColor BackCol = (ThemeInfo.Id == EVTGameThemeId::PapercraftToybox)
					? FLinearColor(0.68f, 0.49f, 0.30f)
					: ThemeInfo.WallBaseColor * 0.82f;
				MID->SetVectorParameterValue(TEXT("BaseColor"), BackCol);
				MID->SetVectorParameterValue(TEXT("Color"), BackCol);
				MID->SetVectorParameterValue(TEXT("EmissiveColor"), BackCol * BackEmissBoost);
				BackPanel->SetMaterial(0, MID);
			}
		}
	}

	// 3. Audio Equalizer Bars in background
	for (int32 i = 0; i < EqualizerBarMIDs.Num(); ++i)
	{
		if (EqualizerBarMIDs[i])
		{
			EqualizerBarMIDs[i]->SetVectorParameterValue(TEXT("BaseColor"), ThemeInfo.EqualizerColor);
			EqualizerBarMIDs[i]->SetVectorParameterValue(TEXT("EmissiveColor"), ThemeInfo.EqualizerColor * 5.0f);
		}
	}

	// 4. Dynamic Tower Main Light
	if (TowerMainLight)
	{
		TowerMainLight->SetLightColor(ThemeInfo.LightColor);
		TowerMainLight->SetIntensity(ThemeInfo.LightIntensity);
	}

	// 5. Active In-Game Platforms
	for (AVTPlatformActor* P : Platforms)
	{
		if (P && !P->bBroken)
		{
			P->ApplyTheme(ThemeInfo);
		}
	}

	// 6. Apply 1:1 Theme Character Costume (Knitted Ragdoll, Wizard, Scuba Diver, Pink Bear, Voxel Hero, Cyber Runner)
	if (ClimberPawn)
	{
		ClimberPawn->ApplyThemeCharacterCostume(ThemeInfo.Id);
		if (ClimberPawn->ShowroomPedestalMesh && ClimberPawn->ShowroomHoloRingMesh)
		{
			UMaterialInstanceDynamic* PedMID = Cast<UMaterialInstanceDynamic>(ClimberPawn->ShowroomPedestalMesh->GetMaterial(0));
			if (PedMID)
			{
				PedMID->SetVectorParameterValue(TEXT("BaseColor"), ThemeInfo.WallBaseColor);
				PedMID->SetVectorParameterValue(TEXT("EmissiveColor"), ThemeInfo.PrimaryColor * 2.5f);
			}
			UMaterialInstanceDynamic* RingMID = Cast<UMaterialInstanceDynamic>(ClimberPawn->ShowroomHoloRingMesh->GetMaterial(0));
			if (RingMID)
			{
				RingMID->SetVectorParameterValue(TEXT("BaseColor"), ThemeInfo.PrimaryColor);
				RingMID->SetVectorParameterValue(TEXT("EmissiveColor"), ThemeInfo.PrimaryColor * 10.0f);
			}
		}
	}

	// 7. Build Rich Distinct 3D World Scenery (Matching All 10 Reference Images 1:1!)
	BuildThemeScenery(ThemeInfo);
}

void AVTTowerGameMode::BuildThemeScenery(const FVTGameThemeInfo& ThemeInfo)
{
	if (!EnvironmentHostActor || !GetWorld()) return;

	USceneComponent* Root = EnvironmentHostActor->GetRootComponent();
	if (!Root) return;

	// 1. Destroy and clear previous theme scenery components
	for (UStaticMeshComponent* Comp : ThemeSceneryMeshes)
	{
		if (Comp) Comp->DestroyComponent();
	}
	ThemeSceneryMeshes.Empty();
	ThemeSceneryMIDs.Empty();
	ThemeSceneryBasePos.Empty();
	ThemeSceneryBaseRot.Empty();

	for (UPointLightComponent* Light : ThemeSceneryLights)
	{
		if (Light) Light->DestroyComponent();
	}
	ThemeSceneryLights.Empty();

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* ConeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));

	UMaterialInterface* CustomNeonMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VelocityTower/Materials/M_NeonEmissive_Master.M_NeonEmissive_Master"));
	UMaterialInterface* CustomWallMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VelocityTower/Materials/M_TowerMegastructure_Master.M_TowerMegastructure_Master"));
	UMaterialInterface* FallbackMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInterface* BaseMat = CustomNeonMat ? CustomNeonMat : (CustomWallMat ? CustomWallMat : FallbackMat);

	// Note: RelLoc.Z is stored as an offset relative to CameraZ so UpdateThemeScenery scrolls all props cleanly!
	const float RefCamZ = FMath::Max(160.0f, CameraZ);
	auto AddSceneryMesh = [this, Root, BaseMat, RefCamZ](UStaticMesh* Mesh, const FVector& RelLoc, const FRotator& RelRot, const FVector& RelScale, const FLinearColor& BaseCol, const FLinearColor& EmissCol) -> UStaticMeshComponent*
	{
		if (!Mesh || !EnvironmentHostActor) return nullptr;
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(EnvironmentHostActor);
		Comp->SetupAttachment(Root);
		Comp->SetStaticMesh(Mesh);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		Comp->SetRelativeLocation(RelLoc);
		Comp->SetRelativeRotation(RelRot);
		Comp->SetRelativeScale3D(RelScale);
		if (BaseMat)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("BaseColor"), BaseCol);
				MID->SetVectorParameterValue(TEXT("Color"), BaseCol);
				MID->SetVectorParameterValue(TEXT("EmissiveColor"), EmissCol);
				Comp->SetMaterial(0, MID);
				ThemeSceneryMIDs.Add(MID);
			}
		}
		Comp->RegisterComponent();
		ThemeSceneryMeshes.Add(Comp);
		ThemeSceneryBasePos.Add(FVector(RelLoc.X, RelLoc.Y, RelLoc.Z - RefCamZ));
		ThemeSceneryBaseRot.Add(RelRot);
		return Comp;
	};

	auto AddSceneryLight = [this, Root](const FVector& RelLoc, const FLinearColor& LightCol, float Intensity, float AttenRadius) -> UPointLightComponent*
	{
		if (!EnvironmentHostActor) return nullptr;
		UPointLightComponent* Light = NewObject<UPointLightComponent>(EnvironmentHostActor);
		Light->SetupAttachment(Root);
		Light->SetRelativeLocation(RelLoc);
		Light->SetLightColor(LightCol);
		Light->SetIntensity(Intensity);
		Light->SetAttenuationRadius(AttenRadius);
		Light->SetCastShadows(false);
		Light->RegisterComponent();
		ThemeSceneryLights.Add(Light);
		return Light;
	};

	// -------------------------------------------------------------------------
	// 1. HAND-CRAFTED PAPERCRAFT 1:1 (Mobile_game_papercraft_UI_theme_20261027193244.jpg)
	//    Corrugated cardboard shoebox, Yellow Star Sticky Note, Pink "tape." patch + Blue Crayon,
	//    diagonal side-wall popsicle sticks with masking tape, and 3D sewing buttons!
	// -------------------------------------------------------------------------
	if (ThemeInfo.Id == EVTGameThemeId::PapercraftToybox)
	{
		const FLinearColor KraftDark(0.55f, 0.37f, 0.20f);
		const FLinearColor TapeWhite(0.95f, 0.93f, 0.86f);

		// A. Corrugated Cardboard Vertical Flute Ribs on the Shoebox Back Wall
		for (int32 f = -6; f <= 6; ++f)
		{
			AddSceneryMesh(CubeMesh, FVector(float(f) * 52.0f, 250.0f, RefCamZ), FRotator::ZeroRotator,
				FVector(0.08f, 0.04f, 16.0f), KraftDark, KraftDark * 0.25f);
		}
		// Horizontal cardboard box crease seams
		for (int32 h = -2; h <= 2; ++h)
		{
			AddSceneryMesh(CubeMesh, FVector(0.0f, 248.0f, RefCamZ + float(h) * 260.0f), FRotator::ZeroRotator,
				FVector(6.8f, 0.05f, 0.04f), KraftDark * 0.85f, KraftDark * 0.18f);
		}

		// B. 1:1 YELLOW STAR STICKY NOTE on Upper-Left Back Wall (Screen Left = +X)
		const FVector StickyCenter(150.0f, 238.0f, RefCamZ + 150.0f);
		const FLinearColor StickyYellow(0.98f, 0.84f, 0.22f);
		const FLinearColor StarBrown(0.68f, 0.38f, 0.12f);
		AddSceneryMesh(CubeMesh, StickyCenter, FRotator(0.0f, 0.0f, -6.0f),
			FVector(1.05f, 0.04f, 1.05f), StickyYellow, StickyYellow * 0.35f);
		// Translucent Scotch Tape strip at top of sticky note
		AddSceneryMesh(CubeMesh, StickyCenter + FVector(-4.0f, -3.0f, 52.0f), FRotator(0.0f, 0.0f, 4.0f),
			FVector(0.48f, 0.05f, 0.16f), TapeWhite, TapeWhite * 0.40f);
		// 5-Point Star drawn on the sticky note
		for (int32 s = 0; s < 5; ++s)
		{
			AddSceneryMesh(CubeMesh, StickyCenter + FVector(0.0f, -4.0f, -4.0f), FRotator(0.0f, 0.0f, float(s) * 36.0f),
				FVector(0.52f, 0.05f, 0.11f), StarBrown, StarBrown * 0.25f);
		}

		// C. 1:1 PINK "tape." PAPER PATCH + BLUE CRAYON PENCIL on Lower-Right Back Wall (Screen Right = -X)
		const FVector PinkNoteCenter(-125.0f, 238.0f, RefCamZ - 95.0f);
		const FLinearColor PinkPaper(0.95f, 0.56f, 0.62f);
		const FLinearColor InkDark(0.24f, 0.18f, 0.16f);
		AddSceneryMesh(CubeMesh, PinkNoteCenter, FRotator(0.0f, 0.0f, 2.0f),
			FVector(1.55f, 0.04f, 1.15f), PinkPaper, PinkPaper * 0.34f);
		// White masking tape strips across top-left and bottom-right corners of pink patch
		AddSceneryMesh(CubeMesh, PinkNoteCenter + FVector(68.0f, -3.0f, 48.0f), FRotator(0.0f, 0.0f, -38.0f),
			FVector(0.55f, 0.05f, 0.16f), TapeWhite, TapeWhite * 0.40f);
		AddSceneryMesh(CubeMesh, PinkNoteCenter + FVector(-68.0f, -3.0f, -48.0f), FRotator(0.0f, 0.0f, -38.0f),
			FVector(0.55f, 0.05f, 0.16f), TapeWhite, TapeWhite * 0.40f);
		// Handwritten "tape." & ruled notebook lines on pink patch
		for (int32 ln = 0; ln < 3; ++ln)
		{
			AddSceneryMesh(CubeMesh, PinkNoteCenter + FVector(10.0f, -4.0f, 22.0f - float(ln) * 22.0f), FRotator(0.0f, 0.0f, 2.0f),
				FVector(0.95f - float(ln) * 0.18f, 0.05f, 0.045f), InkDark, FLinearColor::Black);
		}
		// Blue Crayon / Colored Pencil resting diagonally across bottom-left of pink patch
		const FVector CrayonPos = PinkNoteCenter + FVector(52.0f, -8.0f, -36.0f);
		const FLinearColor CrayonBlue(0.18f, 0.45f, 0.85f);
		const FLinearColor WoodConeCol(0.88f, 0.72f, 0.50f);
		AddSceneryMesh(CylinderMesh, CrayonPos, FRotator(0.0f, 0.0f, 54.0f),
			FVector(0.16f, 0.16f, 0.95f), CrayonBlue, CrayonBlue * 0.28f);
		AddSceneryMesh(ConeMesh, CrayonPos + FVector(38.0f, 0.0f, 28.0f), FRotator(0.0f, 0.0f, -36.0f),
			FVector(0.16f, 0.16f, 0.28f), WoodConeCol, WoodConeCol * 0.25f);

		// D. Diagonal Wooden Popsicle Sticks Taped to the Left & Right Cardboard Shoebox Side Walls
		const FLinearColor SideStickCols[6] = {
			FLinearColor(0.90f, 0.22f, 0.18f), // Red
			FLinearColor(0.20f, 0.68f, 0.30f), // Green
			FLinearColor(0.96f, 0.78f, 0.12f), // Yellow
			FLinearColor(0.16f, 0.52f, 0.86f), // Blue
			FLinearColor(0.62f, 0.28f, 0.76f), // Purple
			FLinearColor(0.95f, 0.48f, 0.14f)  // Orange
		};
		for (int32 s = 0; s < 6; ++s)
		{
			const float SideSign = (s % 2 == 0) ? 1.0f : -1.0f; // +X = Left wall, -X = Right wall
			const float WallX = SideSign * (PlayfieldHalfWidthCm - 6.0f);
			const float StickZ = RefCamZ - 380.0f + float(s) * 155.0f;
			const float TiltPitch = (s % 2 == 0) ? 28.0f : -26.0f;
			AddSceneryMesh(CubeMesh, FVector(WallX, 40.0f, StickZ), FRotator(TiltPitch, 0.0f, 0.0f),
				FVector(0.08f, 1.25f, 0.20f), SideStickCols[s], SideStickCols[s] * 0.28f);
			// White masking tape holding each side stick to the cardboard wall
			AddSceneryMesh(CubeMesh, FVector(WallX - SideSign * 2.0f, 40.0f, StickZ), FRotator(0.0f, 0.0f, 12.0f),
				FVector(0.09f, 0.36f, 0.42f), TapeWhite, TapeWhite * 0.38f);
		}
	}
	// -------------------------------------------------------------------------
	// 2. RETRO SYNTHWAVE 80s (Outrun Sun, Palm Trees, Laser Wall Grids)
	// -------------------------------------------------------------------------
	else if (ThemeInfo.Id == EVTGameThemeId::RetroSynthwave)
	{
		// Iconic 5-tier horizontal sliced Outrun Sun
		const FVector SunCenter(0.0f, 680.0f, RefCamZ - 80.0f);
		const float SunRadii[5] = { 400.0f, 430.0f, 410.0f, 370.0f, 310.0f };
		const float SunZOffsets[5] = { 90.0f, 25.0f, -35.0f, -85.0f, -125.0f };
		const float SunThickness[5] = { 0.40f, 0.26f, 0.20f, 0.16f, 0.12f };
		const FLinearColor SunColors[5] = {
			FLinearColor(1.0f, 0.95f, 0.20f),
			FLinearColor(1.0f, 0.65f, 0.05f),
			FLinearColor(1.0f, 0.25f, 0.15f),
			FLinearColor(0.95f, 0.10f, 0.55f),
			FLinearColor(0.75f, 0.05f, 0.70f)
		};
		for (int32 s = 0; s < 5; ++s)
		{
			AddSceneryMesh(CubeMesh, SunCenter + FVector(0.0f, 0.0f, SunZOffsets[s]), FRotator::ZeroRotator,
				FVector(SunRadii[s] / 50.0f, 0.08f, SunThickness[s]), SunColors[s], SunColors[s] * 12.0f);
		}

		// Palm Tree Silhouettes (Left & Right)
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float PalmX = float(Side) * 260.0f;
			AddSceneryMesh(CylinderMesh, FVector(PalmX, 580.0f, RefCamZ - 180.0f), FRotator(0.0f, 0.0f, float(Side) * -8.0f),
				FVector(0.12f, 0.12f, 3.2f), FLinearColor(0.02f, 0.01f, 0.04f), FLinearColor(0.95f, 0.10f, 0.55f) * 1.5f);
			for (int32 f = 0; f < 4; ++f)
			{
				const float Angle = float(f) * 45.0f - 45.0f;
				AddSceneryMesh(CubeMesh, FVector(PalmX + float(Side) * 35.0f, 570.0f, RefCamZ - 20.0f), FRotator(0.0f, 0.0f, Angle),
					FVector(0.70f, 0.05f, 0.08f), FLinearColor(0.02f, 0.01f, 0.04f), FLinearColor(0.95f, 0.10f, 0.55f) * 2.0f);
			}
		}

		// Wireframe Neon Laser Corner Rails & Horizon Grid Lines
		AddSceneryMesh(CylinderMesh, FVector(-PlayfieldHalfWidthCm - 10.0f, -20.0f, RefCamZ), FRotator::ZeroRotator,
			FVector(0.08f, 0.08f, 18.0f), ThemeInfo.WallEmissiveColor, ThemeInfo.WallEmissiveColor * 16.0f);
		AddSceneryMesh(CylinderMesh, FVector(PlayfieldHalfWidthCm + 10.0f, -20.0f, RefCamZ), FRotator::ZeroRotator,
			FVector(0.08f, 0.08f, 18.0f), FLinearColor(0.0f, 0.95f, 1.0f), FLinearColor(0.0f, 0.95f, 1.0f) * 16.0f);
	}
	// -------------------------------------------------------------------------
	// 3. ANCIENT MYSTICAL RUINS (Hanging Ivy, Wall Torches, Glowing Runes, Crystals)
	// -------------------------------------------------------------------------
	else if (ThemeInfo.Id == EVTGameThemeId::AncientRuins)
	{
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float PillarX = float(Side) * (PlayfieldHalfWidthCm + 14.0f);
			for (int32 v = 0; v < 6; ++v)
			{
				const float VineZ = RefCamZ - 400.0f + float(v) * 160.0f;
				AddSceneryMesh(SphereMesh, FVector(PillarX, -10.0f, VineZ), FRotator(15.0f, float(v * 40), 0.0f),
					FVector(0.32f, 0.32f, 0.62f), FLinearColor(0.10f, 0.46f, 0.16f), FLinearColor(0.10f, 0.46f, 0.16f) * 0.5f);
			}
		}
		const float TorchOffsetsZ[2] = { -220.0f, 220.0f };
		for (int32 t = 0; t < 2; ++t)
		{
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				const float TorchX = float(Side) * (PlayfieldHalfWidthCm - 12.0f);
				const float TorchZ = RefCamZ + TorchOffsetsZ[t];
				AddSceneryMesh(ConeMesh, FVector(TorchX, -15.0f, TorchZ), FRotator(180.0f, 0.0f, 0.0f),
					FVector(0.14f, 0.14f, 0.26f), FLinearColor(0.24f, 0.18f, 0.12f), FLinearColor::Black);
				AddSceneryMesh(SphereMesh, FVector(TorchX, -15.0f, TorchZ + 14.0f), FRotator::ZeroRotator,
					FVector(0.14f, 0.14f, 0.20f), FLinearColor(1.0f, 0.50f, 0.08f), FLinearColor(1.0f, 0.50f, 0.08f) * 16.0f);
				AddSceneryLight(FVector(TorchX, -30.0f, TorchZ + 15.0f), FLinearColor(1.0f, 0.52f, 0.10f), 5500.0f, 480.0f);
			}
		}
		for (int32 r = 0; r < 5; ++r)
		{
			const float RuneZ = RefCamZ - 340.0f + float(r) * 180.0f;
			AddSceneryMesh(CubeMesh, FVector(-PlayfieldHalfWidthCm - 18.0f, -8.0f, RuneZ), FRotator::ZeroRotator,
				FVector(0.06f, 0.16f, 0.36f), ThemeInfo.WallEmissiveColor, ThemeInfo.WallEmissiveColor * 12.0f);
			AddSceneryMesh(CubeMesh, FVector(PlayfieldHalfWidthCm + 18.0f, -8.0f, RuneZ + 80.0f), FRotator::ZeroRotator,
				FVector(0.06f, 0.16f, 0.36f), FLinearColor(0.25f, 0.95f, 0.55f), FLinearColor(0.25f, 0.95f, 0.55f) * 12.0f);
		}
	}
	// -------------------------------------------------------------------------
	// 4. BIOLUMINESCENT DEEP SEA (Sunbeams, Floating Jellyfish, Coral Reefs, Bubbles)
	// -------------------------------------------------------------------------
	else if (ThemeInfo.Id == EVTGameThemeId::DeepSea)
	{
		// Diagonal cyan underwater light rays piercing from above
		for (int32 ray = -1; ray <= 1; ++ray)
		{
			AddSceneryMesh(CubeMesh, FVector(float(ray) * 140.0f, 240.0f, RefCamZ + 80.0f), FRotator(0.0f, 0.0f, -14.0f),
				FVector(0.45f, 0.02f, 12.0f), FLinearColor(0.05f, 0.55f, 0.72f), FLinearColor(0.05f, 0.65f, 0.82f) * 1.6f);
		}
		// 3 Floating translucent Jellyfish in background trench
		const FVector JellyPos[3] = {
			FVector(-140.0f, 340.0f, RefCamZ - 120.0f),
			FVector(160.0f, 420.0f, RefCamZ + 160.0f),
			FVector(-30.0f, 480.0f, RefCamZ + 360.0f)
		};
		for (int32 j = 0; j < 3; ++j)
		{
			AddSceneryMesh(SphereMesh, JellyPos[j], FRotator::ZeroRotator,
				FVector(0.65f, 0.65f, 0.42f), FLinearColor(0.10f, 0.85f, 0.95f), FLinearColor(0.10f, 0.85f, 0.95f) * 8.0f);
			for (int32 ten = 0; ten < 4; ++ten)
			{
				const float TAngle = float(ten) * 90.0f * (PI / 180.0f);
				const FVector TOffset(FMath::Cos(TAngle) * 16.0f, FMath::Sin(TAngle) * 16.0f, -40.0f);
				AddSceneryMesh(CylinderMesh, JellyPos[j] + TOffset, FRotator(0.0f, 0.0f, float(ten * 5 - 10)),
					FVector(0.035f, 0.035f, 0.75f), FLinearColor(0.85f, 0.30f, 0.90f), FLinearColor(0.85f, 0.30f, 0.90f) * 6.0f);
			}
		}
		// Bioluminescent Coral Horns & Bubbles along cavern walls
		for (int32 c = 0; c < 6; ++c)
		{
			const float CoralZ = RefCamZ - 380.0f + float(c) * 160.0f;
			AddSceneryMesh(ConeMesh, FVector(-PlayfieldHalfWidthCm - 12.0f, 10.0f, CoralZ), FRotator(0.0f, 0.0f, -55.0f),
				FVector(0.24f, 0.24f, 0.52f), FLinearColor(0.0f, 0.92f, 0.82f), FLinearColor(0.0f, 0.92f, 0.82f) * 6.0f);
			AddSceneryMesh(ConeMesh, FVector(PlayfieldHalfWidthCm + 12.0f, 10.0f, CoralZ + 80.0f), FRotator(0.0f, 0.0f, 55.0f),
				FVector(0.24f, 0.24f, 0.52f), FLinearColor(0.95f, 0.28f, 0.78f), FLinearColor(0.95f, 0.28f, 0.78f) * 6.0f);
			AddSceneryMesh(SphereMesh, FVector(FMath::Sin(float(c) * 2.1f) * 220.0f, 120.0f, CoralZ + 40.0f), FRotator::ZeroRotator,
				FVector(0.12f, 0.12f, 0.12f), FLinearColor(0.65f, 0.98f, 1.0f), FLinearColor(0.65f, 0.98f, 1.0f) * 4.0f);
		}
	}
	// -------------------------------------------------------------------------
	// 5. INDUSTRIAL FORGE & LAVA (Rotating Cogs, Steam Pipes, Molten Lava Lake)
	// -------------------------------------------------------------------------
	else if (ThemeInfo.Id == EVTGameThemeId::IndustrialForge)
	{
		for (int32 g = 0; g < 4; ++g)
		{
			const float SideSign = (g % 2 == 0) ? -1.0f : 1.0f;
			const float GearX = SideSign * (PlayfieldHalfWidthCm + 35.0f);
			const float GearZ = RefCamZ - 300.0f + float(g) * 220.0f;
			AddSceneryMesh(CylinderMesh, FVector(GearX, 15.0f, GearZ), FRotator(90.0f, 0.0f, 0.0f),
				FVector(1.4f, 1.4f, 0.22f), FLinearColor(0.18f, 0.16f, 0.15f), FLinearColor::Black);
			AddSceneryMesh(CubeMesh, FVector(GearX, 15.0f, GearZ), FRotator(0.0f, 0.0f, 0.0f),
				FVector(1.6f, 0.22f, 0.26f), FLinearColor(0.14f, 0.12f, 0.11f), FLinearColor::Black);
			AddSceneryMesh(CubeMesh, FVector(GearX, 15.0f, GearZ), FRotator(0.0f, 0.0f, 90.0f),
				FVector(1.6f, 0.22f, 0.26f), FLinearColor(0.14f, 0.12f, 0.11f), FLinearColor::Black);
			AddSceneryMesh(CylinderMesh, FVector(GearX, 5.0f, GearZ), FRotator(90.0f, 0.0f, 0.0f),
				FVector(0.35f, 0.35f, 0.26f), FLinearColor(1.0f, 0.40f, 0.02f), FLinearColor(1.0f, 0.40f, 0.02f) * 8.0f);
		}
		AddSceneryMesh(CylinderMesh, FVector(-PlayfieldHalfWidthCm - 45.0f, 25.0f, RefCamZ), FRotator::ZeroRotator,
			FVector(0.24f, 0.24f, 18.0f), FLinearColor(0.15f, 0.13f, 0.12f), FLinearColor::Black);
		AddSceneryMesh(CylinderMesh, FVector(PlayfieldHalfWidthCm + 45.0f, 25.0f, RefCamZ), FRotator::ZeroRotator,
			FVector(0.24f, 0.24f, 18.0f), FLinearColor(0.15f, 0.13f, 0.12f), FLinearColor::Black);
		AddSceneryMesh(CubeMesh, FVector(0.0f, 80.0f, RefCamZ - 520.0f), FRotator::ZeroRotator,
			FVector(8.5f, 4.0f, 0.8f), FLinearColor(1.0f, 0.25f, 0.01f), FLinearColor(1.0f, 0.25f, 0.01f) * 16.0f);
		AddSceneryLight(FVector(0.0f, 0.0f, RefCamZ - 460.0f), FLinearColor(1.0f, 0.35f, 0.05f), 12000.0f, 900.0f);
	}
	// -------------------------------------------------------------------------
	// 6. DEEP SPACE ORBIT (Ringed Planet, Nebula, Elevator Thrusters)
	// -------------------------------------------------------------------------
	else if (ThemeInfo.Id == EVTGameThemeId::DeepSpaceOrbit)
	{
		const FVector PlanetPos(80.0f, 820.0f, RefCamZ + 140.0f);
		AddSceneryMesh(SphereMesh, PlanetPos, FRotator(25.0f, 35.0f, 0.0f),
			FVector(4.8f, 4.8f, 4.8f), FLinearColor(0.12f, 0.25f, 0.58f), FLinearColor(0.05f, 0.18f, 0.42f) * 1.5f);
		AddSceneryMesh(CylinderMesh, PlanetPos, FRotator(65.0f, 25.0f, 0.0f),
			FVector(7.5f, 7.5f, 0.04f), FLinearColor(0.40f, 0.65f, 0.95f), FLinearColor(0.40f, 0.65f, 0.95f) * 3.0f);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float PodX = float(Side) * (PlayfieldHalfWidthCm + 25.0f);
			AddSceneryMesh(ConeMesh, FVector(PodX, -10.0f, RefCamZ + 180.0f), FRotator(180.0f, 0.0f, 0.0f),
				FVector(0.28f, 0.28f, 0.55f), ThemeInfo.PrimaryColor, ThemeInfo.PrimaryColor * 18.0f);
			AddSceneryMesh(ConeMesh, FVector(PodX, -10.0f, RefCamZ - 180.0f), FRotator(180.0f, 0.0f, 0.0f),
				FVector(0.28f, 0.28f, 0.55f), ThemeInfo.PrimaryColor, ThemeInfo.PrimaryColor * 18.0f);
		}
	}
	// -------------------------------------------------------------------------
	// 7. CANDY LAND DESSERT (Spiral Candy Canes, Dripping Frosting, Pastel Rainbow)
	// -------------------------------------------------------------------------
	else if (ThemeInfo.Id == EVTGameThemeId::CandyLand)
	{
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float CaneX = float(Side) * (PlayfieldHalfWidthCm + 8.0f);
			AddSceneryMesh(CylinderMesh, FVector(CaneX, -12.0f, RefCamZ), FRotator::ZeroRotator,
				FVector(0.26f, 0.26f, 18.0f), FLinearColor(0.98f, 0.98f, 0.96f), FLinearColor(0.98f, 0.98f, 0.96f) * 0.45f);
			for (int32 st = 0; st < 8; ++st)
			{
				const float StripeZ = RefCamZ - 420.0f + float(st) * 120.0f;
				AddSceneryMesh(CylinderMesh, FVector(CaneX, -12.0f, StripeZ), FRotator(25.0f, float(Side) * 30.0f, 0.0f),
					FVector(0.28f, 0.28f, 0.45f), FLinearColor(0.98f, 0.16f, 0.32f), FLinearColor(0.98f, 0.16f, 0.32f) * 0.65f);
			}
		}
		for (int32 f = 0; f < 6; ++f)
		{
			const float FrostZ = RefCamZ - 360.0f + float(f) * 150.0f;
			AddSceneryMesh(SphereMesh, FVector(-PlayfieldHalfWidthCm - 12.0f, -8.0f, FrostZ), FRotator::ZeroRotator,
				FVector(0.26f, 0.34f, 0.42f), FLinearColor(1.0f, 0.62f, 0.78f), FLinearColor(1.0f, 0.62f, 0.78f) * 0.5f);
			AddSceneryMesh(SphereMesh, FVector(PlayfieldHalfWidthCm + 12.0f, -8.0f, FrostZ + 75.0f), FRotator::ZeroRotator,
				FVector(0.26f, 0.34f, 0.42f), FLinearColor(0.55f, 0.92f, 0.85f), FLinearColor(0.55f, 0.92f, 0.85f) * 0.5f);
		}
	}
	// -------------------------------------------------------------------------
	// 8. 8-BIT PIXEL DUNGEON (Stepped Voxel Brick Walls, Pixel Torches, Moss & Chains)
	// -------------------------------------------------------------------------
	else if (ThemeInfo.Id == EVTGameThemeId::PixelDungeon)
	{
		const FLinearColor VoxelStone(0.24f, 0.24f, 0.34f);
		const FLinearColor VoxelMoss(0.22f, 0.76f, 0.25f);
		const FLinearColor VoxelFlame(1.0f, 0.62f, 0.10f);
		for (int32 b = 0; b < 8; ++b)
		{
			const float SideSign = (b % 2 == 0) ? 1.0f : -1.0f;
			const float BrickX = SideSign * (PlayfieldHalfWidthCm - 8.0f);
			const float BrickZ = RefCamZ - 420.0f + float(b) * 120.0f;
			AddSceneryMesh(CubeMesh, FVector(BrickX, 10.0f, BrickZ), FRotator::ZeroRotator,
				FVector(0.32f, 0.45f, 0.45f), VoxelStone, VoxelStone * 0.22f);
			AddSceneryMesh(CubeMesh, FVector(BrickX, 8.0f, BrickZ + 24.0f), FRotator::ZeroRotator,
				FVector(0.34f, 0.46f, 0.12f), VoxelMoss, VoxelMoss * 0.35f);
		}
		// 4 Blocky 8-Bit Wall Torches
		for (int32 t = 0; t < 4; ++t)
		{
			const float SideSign = (t % 2 == 0) ? 1.0f : -1.0f;
			const float TorchX = SideSign * 190.0f;
			const float TorchZ = RefCamZ - 240.0f + float(t) * 180.0f;
			AddSceneryMesh(CubeMesh, FVector(TorchX, 235.0f, TorchZ), FRotator::ZeroRotator,
				FVector(0.12f, 0.12f, 0.38f), FLinearColor(0.45f, 0.26f, 0.12f), FLinearColor::Black);
			AddSceneryMesh(CubeMesh, FVector(TorchX, 232.0f, TorchZ + 26.0f), FRotator::ZeroRotator,
				FVector(0.20f, 0.20f, 0.20f), VoxelFlame, VoxelFlame * 10.0f);
		}
	}
	// -------------------------------------------------------------------------
	// 9. WHIMSICAL TOYBOX (Taped Cardboard Box Walls, Giant LEGO Bricks, Toy Blocks)
	// -------------------------------------------------------------------------
	else if (ThemeInfo.Id == EVTGameThemeId::WhimsicalToybox)
	{
		const FLinearColor LegoCols[4] = {
			FLinearColor(0.90f, 0.16f, 0.15f), // Red
			FLinearColor(0.14f, 0.42f, 0.86f), // Blue
			FLinearColor(0.96f, 0.78f, 0.10f), // Yellow
			FLinearColor(0.18f, 0.72f, 0.28f)  // Green
		};
		for (int32 b = 0; b < 8; ++b)
		{
			const float SideSign = (b % 2 == 0) ? 1.0f : -1.0f;
			const float WallX = SideSign * (PlayfieldHalfWidthCm - 4.0f);
			const float BoxZ = RefCamZ - 400.0f + float(b) * 125.0f;
			const FLinearColor Col = LegoCols[b % 4];
			AddSceneryMesh(CubeMesh, FVector(WallX, 20.0f, BoxZ), FRotator::ZeroRotator,
				FVector(0.28f, 0.55f, 0.42f), Col, Col * 0.28f);
			// Cylindrical LEGO Studs protruding inward from wall bricks
			AddSceneryMesh(CylinderMesh, FVector(WallX - SideSign * 16.0f, 20.0f, BoxZ), FRotator(0.0f, 0.0f, 90.0f),
				FVector(0.16f, 0.16f, 0.10f), Col, Col * 0.32f);
		}
	}
	// -------------------------------------------------------------------------
	// 10. CYBERPUNK METROPOLIS (Holographic Billboards, Flying Traffic)
	// -------------------------------------------------------------------------
	else if (ThemeInfo.Id == EVTGameThemeId::CyberpunkDefault)
	{
		const FVector BoardOffsets[4] = {
			FVector(-PlayfieldHalfWidthCm - 35.0f, 120.0f, -180.0f),
			FVector(PlayfieldHalfWidthCm + 35.0f, 140.0f, -60.0f),
			FVector(-PlayfieldHalfWidthCm - 35.0f, 130.0f, 120.0f),
			FVector(PlayfieldHalfWidthCm + 35.0f, 110.0f, 260.0f)
		};
		for (int32 b = 0; b < 4; ++b)
		{
			const FLinearColor BoardCol = (b % 2 == 0) ? ThemeInfo.PrimaryColor : ThemeInfo.WallEmissiveColor;
			AddSceneryMesh(CubeMesh, FVector(BoardOffsets[b].X, BoardOffsets[b].Y, RefCamZ + BoardOffsets[b].Z), FRotator(0.0f, 15.0f, 0.0f),
				FVector(0.06f, 1.2f, 0.85f), BoardCol, BoardCol * 8.0f);
		}
		for (int32 car = 0; car < 2; ++car)
		{
			const float CarY = 560.0f + float(car) * 120.0f;
			const float CarZ = RefCamZ - 100.0f + float(car) * 240.0f;
			AddSceneryMesh(CubeMesh, FVector(0.0f, CarY, CarZ), FRotator::ZeroRotator,
				FVector(0.65f, 0.28f, 0.16f), FLinearColor(0.05f, 0.15f, 0.35f), ThemeInfo.PrimaryColor * 12.0f);
		}
	}
	// -------------------------------------------------------------------------
	// 11. BIO-LUMEN XENOVERSE ARK 1:1 (ref_08_bio_lumen_xenoverse_ark_1793214803988.jpg)
	//     Colossal Teal Ringplanet, Purple Moon, 4 Wall Terrarium Flora Cylinders & Bio-Domes
	// -------------------------------------------------------------------------
	else
	{
		const FLinearColor CyanLumen(0.0f, 0.95f, 1.0f);
		const FLinearColor VioletLumen(0.78f, 0.35f, 1.0f);
		const FLinearColor EmeraldFlora(0.12f, 0.95f, 0.52f);
		const FLinearColor CoralPink(1.0f, 0.38f, 0.72f);

		// A. Colossal Turquoise-and-White Ocean Exoplanet in deep space (Y = +840cm)
		const FVector PlanetPos(60.0f, 840.0f, RefCamZ + 120.0f);
		AddSceneryMesh(SphereMesh, PlanetPos, FRotator(20.0f, 40.0f, 0.0f),
			FVector(5.2f, 5.2f, 5.2f), FLinearColor(0.08f, 0.62f, 0.74f), FLinearColor(0.05f, 0.45f, 0.55f) * 1.8f);
		// Diagonal sweeping glowing cyan planetary rings
		AddSceneryMesh(CylinderMesh, PlanetPos, FRotator(68.0f, 28.0f, 0.0f),
			FVector(8.8f, 8.8f, 0.035f), CyanLumen, CyanLumen * 3.5f);

		// B. Purple/Magenta Companion Moon in the lower right background
		const FVector MoonPos(220.0f, 760.0f, RefCamZ - 120.0f);
		AddSceneryMesh(SphereMesh, MoonPos, FRotator::ZeroRotator,
			FVector(1.8f, 1.8f, 1.8f), VioletLumen * 0.7f, VioletLumen * 2.2f);

		// C. 4 Tall Vertical Cylindrical Wall-Terrariums embedded in the Left & Right Shaft Bulkheads
		// Left Upper & Lower Terrariums (Screen Left = +X)
		const float LeftWallX = PlayfieldHalfWidthCm - 12.0f;
		const float RightWallX = -PlayfieldHalfWidthCm + 12.0f;
		const float TerrariumZ[2] = { RefCamZ + 190.0f, RefCamZ - 140.0f };

		for (int32 t = 0; t < 2; ++t)
		{
			// Left Wall Terrarium Cylinder & Alien Flora
			AddSceneryMesh(CylinderMesh, FVector(LeftWallX, 15.0f, TerrariumZ[t]), FRotator::ZeroRotator,
				FVector(0.36f, 0.36f, 1.55f), FLinearColor(0.06f, 0.12f, 0.18f), CyanLumen * 1.5f);
			AddSceneryMesh(CylinderMesh, FVector(LeftWallX, 12.0f, TerrariumZ[t]), FRotator::ZeroRotator,
				FVector(0.28f, 0.28f, 1.35f), FLinearColor(0.02f, 0.85f, 0.95f), CyanLumen * 8.0f);
			AddSceneryMesh(ConeMesh, FVector(LeftWallX - 8.0f, 10.0f, TerrariumZ[t] - 20.0f), FRotator(0.0f, 0.0f, 18.0f),
				FVector(0.18f, 0.18f, 0.65f), EmeraldFlora, EmeraldFlora * 10.0f);
			AddSceneryMesh(SphereMesh, FVector(LeftWallX - 8.0f, 10.0f, TerrariumZ[t] + 28.0f), FRotator::ZeroRotator,
				FVector(0.16f, 0.16f, 0.24f), CoralPink, CoralPink * 14.0f);

			// Right Wall Terrarium Cylinder & Alien Flora (Screen Right = -X)
			AddSceneryMesh(CylinderMesh, FVector(RightWallX, 15.0f, TerrariumZ[t]), FRotator::ZeroRotator,
				FVector(0.36f, 0.36f, 1.55f), FLinearColor(0.06f, 0.12f, 0.18f), CyanLumen * 1.5f);
			AddSceneryMesh(CylinderMesh, FVector(RightWallX, 12.0f, TerrariumZ[t]), FRotator::ZeroRotator,
				FVector(0.28f, 0.28f, 1.35f), FLinearColor(0.02f, 0.85f, 0.95f), CyanLumen * 8.0f);
			AddSceneryMesh(ConeMesh, FVector(RightWallX + 8.0f, 10.0f, TerrariumZ[t] - 20.0f), FRotator(0.0f, 0.0f, -18.0f),
				FVector(0.18f, 0.18f, 0.65f), EmeraldFlora, EmeraldFlora * 10.0f);
			AddSceneryMesh(SphereMesh, FVector(RightWallX + 8.0f, 10.0f, TerrariumZ[t] + 28.0f), FRotator::ZeroRotator,
				FVector(0.16f, 0.16f, 0.24f), CoralPink, CoralPink * 14.0f);

			// Terrarium Point Lights
			AddSceneryLight(FVector(LeftWallX - 15.0f, -10.0f, TerrariumZ[t]), CyanLumen, 7500.0f, 450.0f);
			AddSceneryLight(FVector(RightWallX + 15.0f, -10.0f, TerrariumZ[t]), CyanLumen, 7500.0f, 450.0f);
		}

		// D. Curved Panoramic Glass Dome Window Ribs in the background
		for (int32 r = -2; r <= 2; ++r)
		{
			AddSceneryMesh(CylinderMesh, FVector(float(r) * 110.0f, 320.0f, RefCamZ), FRotator(0.0f, 0.0f, float(r * 4)),
				FVector(0.04f, 0.04f, 16.0f), FLinearColor(0.15f, 0.22f, 0.32f), CyanLumen * 2.5f);
		}
	}
}

void AVTTowerGameMode::UpdateThemeScenery(float DeltaTime, float TimeSec)
{
	const EVTGameThemeId ActiveThemeId = bInThemeSelectStage ? PreviewThemeId : SelectedThemeId;

	// Scroll all ThemeSceneryMeshes smoothly with CameraZ so props stay framed in the camera view as you climb!
	for (int32 i = 0; i < ThemeSceneryMeshes.Num(); ++i)
	{
		UStaticMeshComponent* Mesh = ThemeSceneryMeshes[i];
		if (!Mesh || !ThemeSceneryBasePos.IsValidIndex(i)) continue;

		FVector TargetLoc = ThemeSceneryBasePos[i];
		TargetLoc.Z += CameraZ;

		if (ActiveThemeId == EVTGameThemeId::DeepSea && Mesh->GetRelativeScale3D().X > 0.5f)
		{
			TargetLoc.Z += FMath::Sin(TimeSec * 2.2f + float(i)) * 14.0f;
		}
		Mesh->SetRelativeLocation(TargetLoc);

		if (ActiveThemeId == EVTGameThemeId::IndustrialForge && Mesh->GetRelativeScale3D().X > 1.2f)
		{
			Mesh->AddLocalRotation(FRotator(0.0f, 0.0f, 25.0f * DeltaTime));
		}
	}

	if (ActiveThemeId == EVTGameThemeId::AncientRuins)
	{
		for (UPointLightComponent* Light : ThemeSceneryLights)
		{
			if (Light)
			{
				const float Flicker = FMath::Sin(TimeSec * 16.0f + Light->GetRelativeLocation().Z) * 600.0f;
				Light->SetIntensity(5500.0f + Flicker);
			}
		}
	}
}

void AVTTowerGameMode::SaveGameToDisk()
{
	if (!ActiveSaveGame) return;
	ActiveSaveGame->BestScore = BestScore;
	ActiveSaveGame->MaxFloorRecord = FMath::Max(ActiveSaveGame->MaxFloorRecord, MaxFloor);
	ActiveSaveGame->NeonCores = NeonCores;
	ActiveSaveGame->PlayerLevel = PlayerLevel;
	ActiveSaveGame->PlayerXP = CurrentXP;
	ActiveSaveGame->ControlMode = CurrentControlMode;
	ActiveSaveGame->GameMode = CurrentGameMode;
	ActiveSaveGame->BaseSpeedRatio = BaseSpeedRatio;
	ActiveSaveGame->SelectedClimber = SelectedClimberId;
	ActiveSaveGame->SelectedTrail = SelectedTrailId;
	ActiveSaveGame->SelectedTheme = SelectedThemeId;
	ActiveSaveGame->StartFloor = StartFloor;

	UGameplayStatics::SaveGameToSlot(ActiveSaveGame, TEXT("VelocityTowerApexSaveV6"), 0);
}

// ============================================================================
// REAL-TIME PROCEDURAL + WAV MIXER AUDIO ENGINE (100% AUDIBLE IN PIE & MOBILE)
// ============================================================================

bool AVTTowerGameMode::LoadWavClipFromDisk(const FString& Key, const FString& RelativeWavPath)
{
	const FString CleanFileName = FPaths::GetCleanFilename(RelativeWavPath);

	TArray<FString> CandidatePaths;
	// 1. Primary UFS pak mount on Android (Content/RawAudio staged into pak via DirectoriesToAlwaysStageAsUFS)
	CandidatePaths.Add(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("RawAudio"), CleanFileName));
	CandidatePaths.Add(FPaths::Combine(FPaths::ProjectContentDir(), RelativeWavPath));
	CandidatePaths.Add(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Audio"), CleanFileName));
	CandidatePaths.Add(FPaths::Combine(FPaths::ProjectContentDir(), CleanFileName));

	// 2. Loose project directories (PC Editor / PIE / local dev)
	CandidatePaths.Add(FPaths::Combine(FPaths::ProjectDir(), RelativeWavPath));
	CandidatePaths.Add(FPaths::Combine(FPaths::ProjectDir(), TEXT("Content/RawAudio"), CleanFileName));
	CandidatePaths.Add(FPaths::Combine(FPaths::ProjectDir(), TEXT("RawAssets/Audio"), CleanFileName));
	CandidatePaths.Add(FPaths::Combine(FPaths::LaunchDir(), RelativeWavPath));
	CandidatePaths.Add(FPaths::Combine(FPaths::LaunchDir(), TEXT("VelocityTower"), RelativeWavPath));
	CandidatePaths.Add(FPaths::Combine(FPaths::RootDir(), RelativeWavPath));
	CandidatePaths.Add(FPaths::Combine(TEXT("E:/Projekte/Velocitytower"), RelativeWavPath));
	CandidatePaths.Add(RelativeWavPath);

	TArray<uint8> RawBytes;
	FString FoundPath;
	for (const FString& Candidate : CandidatePaths)
	{
		if (FFileHelper::LoadFileToArray(RawBytes, *Candidate) && RawBytes.Num() > 44)
		{
			FoundPath = Candidate;
			break;
		}
	}

	if (RawBytes.Num() <= 44)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioEngine] Could NOT load '%s' from '%s'"), *Key, *RelativeWavPath);
		return false;
	}

	// Standard 16-bit 44.1kHz stereo WAV header is 44 bytes; find "data" chunk safely
	int32 DataOffset = 44;
	for (int32 i = 12; i < FMath::Min(RawBytes.Num() - 8, 128); ++i)
	{
		if (RawBytes[i] == 'd' && RawBytes[i + 1] == 'a' && RawBytes[i + 2] == 't' && RawBytes[i + 3] == 'a')
		{
			DataOffset = i + 8;
			break;
		}
	}

	const int32 NumPcmBytes = RawBytes.Num() - DataOffset;
	const int32 NumInt16Samples = NumPcmBytes / 2;
	if (NumInt16Samples <= 2) return false;

	FVTPcmClip Clip;
	Clip.StereoSamples.SetNumUninitialized(NumInt16Samples);
	FMemory::Memcpy(Clip.StereoSamples.GetData(), RawBytes.GetData() + DataOffset, NumInt16Samples * sizeof(int16));
	Clip.NumFrames = NumInt16Samples / 2;
	LoadedAudioClips.Add(Key, MoveTemp(Clip));
	UE_LOG(LogTemp, Log, TEXT("[AudioEngine] Successfully loaded '%s' from '%s' (%d frames)"), *Key, *FoundPath, Clip.NumFrames);
	return true;
}

void AVTTowerGameMode::InitializeAudioEngine()
{
	LoadedAudioClips.Empty();
	ActiveSfxVoices.Empty();
	MusicReadFramePos = 0.0;

	LoadWavClipFromDisk(TEXT("Music"), TEXT("RawAssets/Audio/MUS_CyberTower_Loop.wav"));
	LoadWavClipFromDisk(TEXT("Jump"), TEXT("RawAssets/Audio/SFX_Jump.wav"));
	LoadWavClipFromDisk(TEXT("SuperLaunch"), TEXT("RawAssets/Audio/SFX_SuperLaunch.wav"));
	LoadWavClipFromDisk(TEXT("WallRicochet"), TEXT("RawAssets/Audio/SFX_WallRicochet.wav"));
	LoadWavClipFromDisk(TEXT("CorePickup"), TEXT("RawAssets/Audio/SFX_CorePickup.wav"));
	LoadWavClipFromDisk(TEXT("PowerUp_Magnet"), TEXT("RawAssets/Audio/SFX_PowerUp_Magnet.wav"));
	LoadWavClipFromDisk(TEXT("PowerUp_Spring"), TEXT("RawAssets/Audio/SFX_PowerUp_Spring.wav"));
	LoadWavClipFromDisk(TEXT("PowerUp_Freeze"), TEXT("RawAssets/Audio/SFX_PowerUp_Freeze.wav"));
	LoadWavClipFromDisk(TEXT("PowerUp_Shield"), TEXT("RawAssets/Audio/SFX_PowerUp_Shield.wav"));
	LoadWavClipFromDisk(TEXT("ComboUp"), TEXT("RawAssets/Audio/SFX_ComboUp.wav"));
	LoadWavClipFromDisk(TEXT("LevelUp"), TEXT("RawAssets/Audio/SFX_LevelUp_Fanfare.wav"));
	LoadWavClipFromDisk(TEXT("IceShatter"), TEXT("RawAssets/Audio/SFX_IceShatter.wav"));
	LoadWavClipFromDisk(TEXT("Click"), TEXT("RawAssets/Audio/SFX_ButtonClick.wav"));
	LoadWavClipFromDisk(TEXT("FallScream"), TEXT("RawAssets/Audio/SFX_Character_Scream.wav"));
	LoadWavClipFromDisk(TEXT("DeadImpact"), TEXT("RawAssets/Audio/SFX_Dead_Impact.wav"));

	// Load all 24 Quake / Unreal Tournament Style Deep Male English Announcer Voice Lines!
	for (int32 i = 1; i <= 19; ++i)
	{
		const FString Key = FString::Printf(TEXT("VO_Combo_%02d"), i);
		LoadWavClipFromDisk(Key, FString::Printf(TEXT("RawAssets/Audio/%s.wav"), *Key));
	}
	LoadWavClipFromDisk(TEXT("VO_Combo_HolyShit"), TEXT("RawAssets/Audio/VO_Combo_HolyShit.wav"));
	LoadWavClipFromDisk(TEXT("VO_Clutch_Close"), TEXT("RawAssets/Audio/VO_Clutch_Close.wav"));
	LoadWavClipFromDisk(TEXT("VO_Clutch_Barely"), TEXT("RawAssets/Audio/VO_Clutch_Barely.wav"));
	LoadWavClipFromDisk(TEXT("VO_Wall_Ricochet"), TEXT("RawAssets/Audio/VO_Wall_Ricochet.wav"));
	LoadWavClipFromDisk(TEXT("VO_HighScore"), TEXT("RawAssets/Audio/VO_HighScore.wav"));
	LoadWavClipFromDisk(TEXT("VO_GameOver"), TEXT("RawAssets/Audio/VO_GameOver.wav"));

	// Procedural SFX Synthesis Fallback (Guarantees all SFX are audible even if a WAV is missing)
	auto SynthesizeSfxFallback = [this](const FString& Key, float StartFreq, float EndFreq, float DurationSec, bool bHarmonics)
	{
		if (LoadedAudioClips.Contains(Key)) return;
		const int32 SampleRate = 44100;
		const int32 NumFrames = FMath::Clamp(FMath::RoundToInt(DurationSec * float(SampleRate)), 256, 44100);
		FVTPcmClip Clip;
		Clip.NumFrames = NumFrames;
		Clip.StereoSamples.SetNumUninitialized(NumFrames * 2);
		double Phase = 0.0;
		for (int32 f = 0; f < NumFrames; ++f)
		{
			const float T = float(f) / float(NumFrames);
			const float Freq = FMath::Lerp(StartFreq, EndFreq, T);
			Phase += (double(Freq) * 2.0 * PI) / double(SampleRate);
			const float Env = FMath::Pow(1.0f - T, 1.8f);
			float S = FMath::Sin(float(Phase));
			if (bHarmonics) S = (S + 0.35f * FMath::Sin(float(Phase * 2.0))) / 1.35f;
			const int16 SampleVal = static_cast<int16>(FMath::Clamp(FMath::RoundToInt(S * Env * 28000.0f), -32767, 32767));
			Clip.StereoSamples[f * 2]     = SampleVal;
			Clip.StereoSamples[f * 2 + 1] = SampleVal;
		}
		LoadedAudioClips.Add(Key, MoveTemp(Clip));
	};

	SynthesizeSfxFallback(TEXT("Jump"), 280.0f, 780.0f, 0.08f, true);
	SynthesizeSfxFallback(TEXT("SuperLaunch"), 180.0f, 1100.0f, 0.22f, true);
	SynthesizeSfxFallback(TEXT("WallRicochet"), 980.0f, 320.0f, 0.06f, false);
	SynthesizeSfxFallback(TEXT("CorePickup"), 580.0f, 880.0f, 0.10f, false);
	SynthesizeSfxFallback(TEXT("ComboUp"), 440.0f, 660.0f, 0.12f, false);
	SynthesizeSfxFallback(TEXT("Click"), 850.0f, 600.0f, 0.03f, false);
	SynthesizeSfxFallback(TEXT("DeadImpact"), 140.0f, 32.0f, 0.28f, true);
	SynthesizeSfxFallback(TEXT("FallScream"), 650.0f, 160.0f, 0.35f, false);
	SynthesizeSfxFallback(TEXT("IceShatter"), 1200.0f, 220.0f, 0.14f, true);
	SynthesizeSfxFallback(TEXT("LevelUp"), 523.0f, 784.0f, 0.25f, true);

	ProceduralAudioStream = NewObject<USoundWaveProcedural>(this, TEXT("VTProceduralAudioStream"));
	if (ProceduralAudioStream)
	{
		ProceduralAudioStream->SetSampleRate(44100);
		ProceduralAudioStream->NumChannels = 2;
		ProceduralAudioStream->Duration = INDEFINITELY_LOOPING_DURATION;
		ProceduralAudioStream->SoundGroup = SOUNDGROUP_Default;
		ProceduralAudioStream->bLooping = false;
		ProceduralAudioStream->bCanProcessAsync = true;
	}

	if (GetWorld())
	{
		ProceduralAudioComp = UGameplayStatics::CreateSound2D(GetWorld(), ProceduralAudioStream, 1.0f, 1.0f, 0.0f, nullptr, false, false);
		if (ProceduralAudioComp)
		{
			ProceduralAudioComp->bAutoDestroy = false;
			ProceduralAudioComp->bIsUISound = true;
			ProceduralAudioComp->bAllowSpatialization = false;
			ProceduralAudioComp->SetVolumeMultiplier(1.0f);
			ProceduralAudioComp->Play();
		}
	}

	// Pre-fill 120ms of audio buffer immediately so playback starts without underrun
	PumpProceduralAudioStream(0.08f);
}

void AVTTowerGameMode::PumpProceduralAudioStream(float DeltaTime)
{
	if (!ProceduralAudioStream) return;

	if (!ProceduralAudioComp || !ProceduralAudioComp->IsPlaying())
	{
		if (!ProceduralAudioComp && GetWorld())
		{
			ProceduralAudioComp = UGameplayStatics::CreateSound2D(GetWorld(), ProceduralAudioStream, 1.0f, 1.0f, 0.0f, nullptr, false, false);
			if (ProceduralAudioComp)
			{
				ProceduralAudioComp->bAutoDestroy = false;
				ProceduralAudioComp->bIsUISound = true;
				ProceduralAudioComp->bAllowSpatialization = false;
			}
		}
		if (ProceduralAudioComp)
		{
			ProceduralAudioComp->Play();
		}
	}

	const bool bMuted = ActiveSaveGame ? ActiveSaveGame->bAudioMuted : false;
	const int32 QueuedBytes = ProceduralAudioStream->GetAvailableAudioByteCount();
	const int32 TargetQueuedBytes = 44100 * 4 / 8; // Keep ~125ms stereo 16-bit buffered for low-latency SFX
	const int32 BytesNeeded = TargetQueuedBytes - QueuedBytes;
	if (BytesNeeded <= 512) return;

	const int32 FramesToGenerate = FMath::Clamp(BytesNeeded / 4, 256, 8192);
	TArray<int16> OutStereoPcm;
	OutStereoPcm.SetNumZeroed(FramesToGenerate * 2);

	if (!bMuted)
	{
		const FVTPcmClip* MusicClip = LoadedAudioClips.Find(TEXT("Music"));
		const double MusicPitch = (TowerRunState == EVTGameState::Play)
			? FMath::Clamp(double(CurrentSmoothedMusicPitch), 0.70, 1.55)
			: 0.85;
		const float MusicVol = (TowerRunState == EVTGameState::Play) ? 0.58f : 0.42f;

		for (int32 f = 0; f < FramesToGenerate; ++f)
		{
			float MixL = 0.0f;
			float MixR = 0.0f;

			// 1. Dynamic BPM-Synced Synthwave Loop
			if (MusicClip && MusicClip->NumFrames > 0)
			{
				const int32 Idx = FMath::Clamp(int32(MusicReadFramePos), 0, MusicClip->NumFrames - 1);
				MixL += (float(MusicClip->StereoSamples[Idx * 2]) / 32767.0f) * MusicVol;
				MixR += (float(MusicClip->StereoSamples[Idx * 2 + 1]) / 32767.0f) * MusicVol;

				MusicReadFramePos += MusicPitch;
				if (MusicReadFramePos >= double(MusicClip->NumFrames))
				{
					MusicReadFramePos = FMath::Fmod(MusicReadFramePos, double(MusicClip->NumFrames));
				}
			}
			else
			{
				// Procedural Synthwave Fallback (dynamic cyber bass + synth arp)
				static double SynthPhase = 0.0;
				static double BassPhase = 0.0;
				static int32 StepIndex = 0;
				static double StepTimer = 0.0;

				StepTimer += (1.0 / 44100.0) * MusicPitch;
				if (StepTimer >= 0.125)
				{
					StepTimer -= 0.125;
					StepIndex = (StepIndex + 1) % 16;
				}

				const float BassNotes[16] = { 82.4f, 82.4f, 98.0f, 82.4f, 110.0f, 82.4f, 123.5f, 98.0f, 82.4f, 82.4f, 98.0f, 82.4f, 146.8f, 130.8f, 123.5f, 98.0f };
				const float LeadNotes[16] = { 329.6f, 392.0f, 440.0f, 493.9f, 587.3f, 659.3f, 493.9f, 392.0f, 440.0f, 392.0f, 329.6f, 293.7f, 329.6f, 440.0f, 493.9f, 659.3f };

				BassPhase += (double(BassNotes[StepIndex]) * 2.0 * PI) / 44100.0;
				SynthPhase += (double(LeadNotes[(StepIndex + 3) % 16]) * 2.0 * PI) / 44100.0;

				const float BassSaw = float((FMath::Fmod(BassPhase, 2.0 * PI) / PI) - 1.0);
				const float LeadSin = FMath::Sin(float(SynthPhase));
				const float Env = FMath::Clamp(1.0f - float(StepTimer / 0.125), 0.0f, 1.0f);

				MixL += (BassSaw * 0.26f + LeadSin * Env * 0.18f) * MusicVol;
				MixR += (BassSaw * 0.26f + LeadSin * (1.0f - Env) * 0.18f) * MusicVol;
			}

			// 2. Active Pitch-Shifted Dopamine SFX Voices
			for (int32 v = ActiveSfxVoices.Num() - 1; v >= 0; --v)
			{
				FVTActiveSfxVoice& Voice = ActiveSfxVoices[v];
				const FVTPcmClip* SfxClip = LoadedAudioClips.Find(Voice.Key);
				if (!SfxClip || SfxClip->NumFrames <= 0 || Voice.ReadFramePos >= double(SfxClip->NumFrames))
				{
					ActiveSfxVoices.RemoveAtSwap(v);
					continue;
				}
				const int32 SIdx = FMath::Clamp(int32(Voice.ReadFramePos), 0, SfxClip->NumFrames - 1);
				MixL += (float(SfxClip->StereoSamples[SIdx * 2]) / 32767.0f) * Voice.Volume;
				MixR += (float(SfxClip->StereoSamples[SIdx * 2 + 1]) / 32767.0f) * Voice.Volume;
				Voice.ReadFramePos += double(Voice.Pitch);
			}

			OutStereoPcm[f * 2]     = static_cast<int16>(FMath::Clamp(FMath::RoundToInt(MixL * 32000.0f), -32767, 32767));
			OutStereoPcm[f * 2 + 1] = static_cast<int16>(FMath::Clamp(FMath::RoundToInt(MixR * 32000.0f), -32767, 32767));
		}
	}

	ProceduralAudioStream->QueueAudio(reinterpret_cast<const uint8*>(OutStereoPcm.GetData()), OutStereoPcm.Num() * sizeof(int16));
}

void AVTTowerGameMode::PlayTowerSFX(const FString& SfxKey, float PitchMult, float VolumeMult)
{
	if (ActiveSaveGame && ActiveSaveGame->bAudioMuted) return;
	if (!LoadedAudioClips.Contains(SfxKey)) return;

	if (ActiveSfxVoices.Num() >= 12)
	{
		ActiveSfxVoices.RemoveAt(0);
	}

	FVTActiveSfxVoice NewVoice;
	NewVoice.Key = SfxKey;
	NewVoice.ReadFramePos = 0.0;
	NewVoice.Pitch = FMath::Clamp(PitchMult, 0.5f, 2.5f);
	NewVoice.Volume = FMath::Clamp(VolumeMult, 0.05f, 1.5f);
	NewVoice.Priority = 0;
	ActiveSfxVoices.Add(NewVoice);
}

int32 AVTTowerGameMode::GetVoicePriority(const FString& VoiceKey) const
{
	// 6 = ULTRA / MEGA KRASS (interrupts lower playing voices immediately!)
	if (VoiceKey == TEXT("VO_HighScore") || VoiceKey == TEXT("VO_GameOver"))
	{
		return 6;
	}
	if (VoiceKey == TEXT("VO_Combo_19") || VoiceKey == TEXT("VO_Combo_HolyShit") || VoiceKey == TEXT("VO_Combo_18"))
	{
		return 6; // Godlike, Holy Shit, Ludicrous Speed (Highest priority, interrupts anything!)
	}

	// 5 = VERY HIGH (Major combos 40x-145x, Level Up)
	if (VoiceKey == TEXT("VO_Combo_12") || VoiceKey == TEXT("VO_Combo_13") ||
	    VoiceKey == TEXT("VO_Combo_14") || VoiceKey == TEXT("VO_Combo_15") ||
	    VoiceKey == TEXT("VO_Combo_16") || VoiceKey == TEXT("VO_Combo_17"))
	{
		return 5; // Unstoppable, Dominating, Rampage, Multi, Ultra, Monster
	}
	if (VoiceKey == TEXT("LevelUp"))
	{
		return 5;
	}

	// 4 = HIGH (Intermediate combo milestones 12x-20x)
	if (VoiceKey == TEXT("VO_Combo_08") || VoiceKey == TEXT("VO_Combo_09") ||
	    VoiceKey == TEXT("VO_Combo_10") || VoiceKey == TEXT("VO_Combo_12"))
	{
		return 4; // Amazing, Incredible, Spectacular, Unstoppable
	}

	// 3 = NORMAL (Early combo milestones 2x, 5x, 8x)
	if (VoiceKey.StartsWith(TEXT("VO_Combo_")))
	{
		return 3; // Double Jump, Super Combo, Excellent
	}

	// 2 = MEDIUM-LOW ("Oh that was close", "Barely made it" clutch saves)
	if (VoiceKey.StartsWith(TEXT("VO_Clutch_")))
	{
		return 2;
	}

	// 1 = LOW (Wall ricochet callout)
	if (VoiceKey == TEXT("VO_Wall_Ricochet"))
	{
		return 1;
	}

	return 0; // Default SFX
}

void AVTTowerGameMode::TriggerAnnouncerVoice(const FString& VoiceKey, const FString& DisplayCallout)
{
	LatestAnnouncerCallout = DisplayCallout;
	AnnouncerTimerSec = 2.0f;

	bool bNewlyUnlocked = false;
	if (ActiveSaveGame && !VoiceKey.IsEmpty())
	{
		if (!ActiveSaveGame->UnlockedVoiceLines.Contains(VoiceKey))
		{
			ActiveSaveGame->UnlockedVoiceLines.AddUnique(VoiceKey);
			bNewlyUnlocked = true;
			AwardNeonCores(10, FString::Printf(TEXT("NEUE STIMME (%d/24)"), ActiveSaveGame->UnlockedVoiceLines.Num()));
			AwardPlayerXP(120, TEXT("Announcer Voice Unlock"));
			SaveGameToDisk();
		}
	}

	if (!ActiveSaveGame || ActiveSaveGame->bVoiceEnabled)
	{
		if (ActiveSaveGame && ActiveSaveGame->bAudioMuted) return;
		if (!LoadedAudioClips.Contains(VoiceKey)) return;

		const float CurrentGameTime = GetWorld() ? float(GetWorld()->GetTimeSeconds()) : 0.0f;
		const int32 IncomingPrio = GetVoicePriority(VoiceKey);

		// Find highest priority voice currently playing in ActiveSfxVoices
		int32 ActiveMaxPrio = -1;
		for (int32 i = 0; i < ActiveSfxVoices.Num(); ++i)
		{
			if (ActiveSfxVoices[i].Key.StartsWith(TEXT("VO_")))
			{
				if (ActiveSfxVoices[i].Priority > ActiveMaxPrio)
				{
					ActiveMaxPrio = ActiveSfxVoices[i].Priority;
				}
			}
		}

		// RULE 1: If a voice is already talking:
		if (ActiveMaxPrio >= 0)
		{
			// Only interrupt if incoming voice has STRICTLY HIGHER priority ("mega krass" / Highscore / Ultra Combo)
			if (IncomingPrio > ActiveMaxPrio)
			{
				// Interrupt active lower-priority voice(s)
				for (int32 i = ActiveSfxVoices.Num() - 1; i >= 0; --i)
				{
					if (ActiveSfxVoices[i].Key.StartsWith(TEXT("VO_")))
					{
						ActiveSfxVoices.RemoveAtSwap(i);
					}
				}

				FVTActiveSfxVoice NewVoice;
				NewVoice.Key = VoiceKey;
				NewVoice.ReadFramePos = 0.0;
				NewVoice.Pitch = 1.0f;
				NewVoice.Volume = 1.35f;
				NewVoice.Priority = IncomingPrio;
				ActiveSfxVoices.Add(NewVoice);
				LastAnnouncerVoiceTime = CurrentGameTime;
			}
			else
			{
				// Do NOT cut off the currently playing sentence! Lower/equal priority voice sound is ignored.
				return;
			}
		}
		else
		{
			// RULE 2: No voice currently playing -> check anti-spam cooldowns!
			if (VoiceKey.StartsWith(TEXT("VO_Clutch_")))
			{
				if (CurrentGameTime - LastClutchVoiceTime < 6.0f)
				{
					return; // Prevent repeated "Oh that was close" spam
				}
				LastClutchVoiceTime = CurrentGameTime;
			}
			else if (VoiceKey == TEXT("VO_Wall_Ricochet"))
			{
				if (CurrentGameTime - LastWallVoiceTime < 4.5f)
				{
					return; // Prevent repeated wall announcer spam
				}
				LastWallVoiceTime = CurrentGameTime;
			}
			else if (VoiceKey.StartsWith(TEXT("VO_Combo_")))
			{
				if (IncomingPrio < 6 && (CurrentGameTime - LastAnnouncerVoiceTime < 1.9f))
				{
					return; // Allow sentences to finish and prevent rapid machine-gun callouts
				}
				LastAnnouncerVoiceTime = CurrentGameTime;
			}

			FVTActiveSfxVoice NewVoice;
			NewVoice.Key = VoiceKey;
			NewVoice.ReadFramePos = 0.0;
			NewVoice.Pitch = 1.0f;
			NewVoice.Volume = 1.35f;
			NewVoice.Priority = IncomingPrio;
			ActiveSfxVoices.Add(NewVoice);
		}
	}

	if (bNewlyUnlocked && ActiveSaveGame)
	{
		LatestAnnouncerCallout = FString::Printf(TEXT("%s [NEUE STIMME %d/24! +10c]"), *DisplayCallout, ActiveSaveGame->UnlockedVoiceLines.Num());
	}
}

int32 AVTTowerGameMode::GetUnlockedVoiceCount() const
{
	return ActiveSaveGame ? ActiveSaveGame->UnlockedVoiceLines.Num() : 0;
}

void AVTTowerGameMode::ToggleAudioMute()
{
	if (!ActiveSaveGame) return;
	ActiveSaveGame->bAudioMuted = !ActiveSaveGame->bAudioMuted;
	SaveGameToDisk();
	if (!ActiveSaveGame->bAudioMuted)
	{
		PlayTowerSFX(TEXT("ComboUp"), 1.15f, 1.0f);
	}
	TriggerToast(ActiveSaveGame->bAudioMuted ? TEXT("AUDIO & MUSIK: STUMMGESCHALTET") : TEXT("AUDIO & SYNTH-MUSIK: AKTIV!"));
}

void AVTTowerGameMode::ToggleVoiceEnabled()
{
	if (!ActiveSaveGame) return;
	ActiveSaveGame->bVoiceEnabled = !ActiveSaveGame->bVoiceEnabled;
	SaveGameToDisk();
	PlayTowerSFX(TEXT("Click"), 1.1f, 0.9f);
	TriggerToast(ActiveSaveGame->bVoiceEnabled ? TEXT("ANNOUNCER CALLOUTS: AKTIV") : TEXT("ANNOUNCER CALLOUTS: AUS"));
}

// ============================================================================
// DOPAMINE PLAYER XP & LEVEL PROGRESSION SYSTEM
// ============================================================================

void AVTTowerGameMode::AwardPlayerXP(int32 Amount, const FString& SourceLabel)
{
	if (Amount <= 0) return;
	const int32 Mult = (CurrentGameMode == EVTGameModeType::Overdrive2x) ? 2 : 1;
	CurrentXP += Amount * Mult;
	XpGainPulse = 1.0f;
	XPToNextLevel = 400 + (PlayerLevel - 1) * 200;

	bool bLeveledUp = false;
	while (CurrentXP >= XPToNextLevel)
	{
		CurrentXP -= XPToNextLevel;
		PlayerLevel++;
		XPToNextLevel = 400 + (PlayerLevel - 1) * 200;
		bLeveledUp = true;
	}

	if (bLeveledUp)
	{
		const int32 CoreBonus = 25;
		AwardNeonCores(CoreBonus, FString::Printf(TEXT("LEVEL %d AUFSTIEG!"), PlayerLevel));
		LatestLevelUpText = FString::Printf(TEXT("LEVEL UP! LVL %d (%s) // +%d CORES!"), PlayerLevel, *GetPlayerRankTitle(), CoreBonus);
		LevelUpBannerTimerSec = 3.5f;
		PlayTowerSFX(TEXT("LevelUp"), 1.0f, 1.15f);
		if (ClimberPawn)
		{
			SpawnShockwave3D(ClimberPawn->PosX, ClimberPawn->PosZ, 240.0f, FLinearColor(1.0f, 0.82f, 0.15f));
			SpawnShockwave3D(ClimberPawn->PosX, ClimberPawn->PosZ, 160.0f, FLinearColor(0.0f, 0.961f, 0.831f));
		}
	}
	SaveGameToDisk();
}

FString AVTTowerGameMode::GetPlayerRankTitle() const
{
	if (PlayerLevel >= 30) return TEXT("APEX EMPEROR");
	if (PlayerLevel >= 20) return TEXT("GRANDMASTER");
	if (PlayerLevel >= 15) return TEXT("CYBER LEGEND");
	if (PlayerLevel >= 10) return TEXT("NEON VANGUARD");
	if (PlayerLevel >= 6)  return TEXT("TOWER PRO");
	if (PlayerLevel >= 3)  return TEXT("STREET RUNNER");
	return TEXT("ROOKIE CLIMBER");
}

// ============================================================================
// DEVELOPER ADMIN / GODMODE TESTING CONTROLS (HIDDEN FROM NORMAL PLAYERS)
// ============================================================================

void AVTTowerGameMode::AdminWarpFloors(int32 DeltaFloors)
{
	if (!ClimberPawn) return;
	const int32 TargetFloor = FMath::Max(0, MaxFloor + DeltaFloors);
	StartFloor = TargetFloor;
	StartNewRun();
	PlayTowerSFX(TEXT("SuperLaunch"), 1.2f, 1.0f);
	TriggerToast(FString::Printf(TEXT("ADMIN WARP: ETAGE %d (%s)"), TargetFloor, *GetBiomeForFloor(TargetFloor).Name));
}

void AVTTowerGameMode::AdminUnlockAllClimbers()
{
	if (!ActiveSaveGame) return;
	for (const FVTClimberInfo& C : Climbers)
	{
		ActiveSaveGame->UnlockedClimbers.AddUnique(C.Id);
	}
	SaveGameToDisk();
	PlayTowerSFX(TEXT("LevelUp"), 1.0f, 1.0f);
	TriggerToast(TEXT("ADMIN: ALLE 20 CHARAKTERE FREIGESCHALTET!"));
}

void AVTTowerGameMode::AdminResetRosterToStarter()
{
	if (!ActiveSaveGame) return;
	ActiveSaveGame->UnlockedClimbers.Empty();
	ActiveSaveGame->UnlockedClimbers.AddUnique(EVTClimberId::KidFlex);
	ActiveSaveGame->bIsVip = false;
	SelectedClimberId = EVTClimberId::KidFlex;
	PreviewClimberId = EVTClimberId::KidFlex;
	ActiveSaveGame->SelectedClimber = EVTClimberId::KidFlex;
	if (ClimberPawn)
	{
		ClimberPawn->ApplyClimberVisuals(GetClimberInfo(EVTClimberId::KidFlex), GetTrailInfo(SelectedTrailId).Color);
	}
	SaveGameToDisk();
	PlayTowerSFX(TEXT("Click"), 0.85f, 1.0f);
	TriggerToast(TEXT("ADMIN: ROSTER RESET AUF #01 KID FLEX (1/20)!"));
}

void AVTTowerGameMode::AdminTriggerMegaCombo(int32 TargetCombo)
{
	CurrentCombo = FMath::Max(2, TargetCombo);
	if (CurrentCombo > MaxCombo) MaxCombo = CurrentCombo;
	ComboTimerFrames = 240;
	ComboHitPulse = 1.0f;
	LatestAnnouncerCallout = FString::Printf(TEXT("%dx GODLIKE APEX!!"), CurrentCombo);
	AnnouncerTimerSec = 2.2f;
	PlayTowerSFX(TEXT("ComboUp"), 1.65f, 1.0f);
	TriggerToast(FString::Printf(TEXT("ADMIN: %dx MEGA-COMBO AKTIVIERT!"), CurrentCombo));
}

void AVTTowerGameMode::AdminResetPhysicsDefaults()
{
	bAdminGodMode = false;
	bAdminInfiniteJump = false;
	bAdminInfinitePowerups = false;
	AdminGravityScale = 1.0f;
	AdminJumpForceScale = 1.0f;
	AdminMoveSpeedScale = 1.0f;
	AdminCameraSpeedScale = 1.0f;
	AdminPlatformWidthScale = 1.0f;
	PlayTowerSFX(TEXT("Click"), 1.0f, 1.0f);
	TriggerToast(TEXT("ADMIN: ALLE PHYSIK-VARIABLEN AUF STANDARD (1.0x) ZURUECKGESETZT"));
}

#include "VTSaveGame.h"

UVTSaveGame::UVTSaveGame()
{
	// Only 1 Climber (KID FLEX) is unlocked initially out of the 20-Climber Roster!
	UnlockedClimbers.Empty();
	UnlockedClimbers.AddUnique(EVTClimberId::KidFlex);
	bMigratedTo20Roster = true;

	UnlockedTrails.AddUnique(TEXT("cyan"));
	UnlockedThemes.AddUnique(EVTGameThemeId::CyberpunkDefault);

	// Initialize 3 Daily Quests
	{
		FVTQuest Q1;
		Q1.Id = TEXT("combo");
		Q1.Title = TEXT("Erziele eine 8x Combo");
		Q1.Target = 8;
		Q1.Progress = 0;
		Q1.RewardCores = 12;
		Q1.Icon = TEXT("ZAP");
		DailyQuests.Add(Q1);

		FVTQuest Q2;
		Q2.Id = TEXT("wall");
		Q2.Title = TEXT("Mache 4 Apex-Wandsprünge");
		Q2.Target = 4;
		Q2.Progress = 0;
		Q2.RewardCores = 15;
		Q2.Icon = TEXT("WALL");
		DailyQuests.Add(Q2);

		FVTQuest Q3;
		Q3.Id = TEXT("boost");
		Q3.Title = TEXT("Nutze 2x Super Launch-Pads");
		Q3.Target = 2;
		Q3.Progress = 0;
		Q3.RewardCores = 10;
		Q3.Icon = TEXT("BOOST");
		DailyQuests.Add(Q3);
	}

	// Default Rivals / Friends
	{
		FVTFriendEntry F1;
		F1.Name = TEXT("Tobi");
		F1.Tag = TEXT("TOBI-2026");
		F1.Avatar = TEXT("ZAP");
		F1.MaxFloor = 125;
		F1.BestScore = 68500;
		F1.Status = TEXT("Online");
		FriendsList.Add(F1);

		FVTFriendEntry F2;
		F2.Name = TEXT("CyberNinja");
		F2.Tag = TEXT("NINJA-99");
		F2.Avatar = TEXT("NINJA");
		F2.MaxFloor = 98;
		F2.BestScore = 49200;
		F2.Status = TEXT("Offline");
		FriendsList.Add(F2);
	}
}

#pragma once

namespace CoDAssets
{
	enum class ESupportedGames : uint8
	{
		None,
		WorldAtWar, // COD 5, IW 3.0
		BlackOps, // COD 7, IW 3.0
		BlackOps2, // COD 9, IW 3.0
		BlackOps3, // COD 12, IW 3.0
		BlackOps4, // COD 15, IW 3.0
		BlackOpsCW, // COD 17, IW 8.0
		BlackOps5, // COD 21, IW 9.0
		ModernWarfare, // COD 4, IW 3.0
		ModernWarfare2, // COD 6, IW 4.0 
		ModernWarfare3, // COD 8, IW 5.0
		ModernWarfare4, // COD 16, IW 8.0
		ModernWarfare5, // COD 19, IW 9.0
		ModernWarfare6, // COD 20, IW 9.0
		QuantumSolace, // IW 3.0
		ModernWarfareRemastered, // COD 4 2016重制版, IW 6.0
		ModernWarfare2Remastered, // COD 6 2020重制版, IW 6.0
		Ghosts, // COD 10, IW 6.0
		InfiniteWarfare, // COD 13, IW 7.0
		AdvancedWarfare, // COD 11, IW 6.0
		WorldWar2, // COD 14, IW 7.0
		Vanguard, // COD 18, IW 8.0
		Parasyte, // DUMP工具
	};

	enum class ESupportedGameFlags : uint8
	{
		None,
		Files,
		SP,
		MP,
		ZM
	};

	struct FCoDGameProcess
	{
		FString ProcessName;
		ESupportedGames GameID;
		ESupportedGameFlags GameFlags;
	};

	inline TArray<FCoDGameProcess> GameProcessInfos =
	{
		// World at War
		{"codwaw.exe", ESupportedGames::WorldAtWar, ESupportedGameFlags::SP},
		{"codwawmp.exe", ESupportedGames::WorldAtWar, ESupportedGameFlags::MP},
		// Black Ops
		{"blackops.exe", ESupportedGames::BlackOps, ESupportedGameFlags::SP},
		{"blackopsmp.exe", ESupportedGames::BlackOps, ESupportedGameFlags::MP},
		// Black Ops 2
		{"t6zm.exe", ESupportedGames::BlackOps2, ESupportedGameFlags::ZM},
		{"t6mp.exe", ESupportedGames::BlackOps2, ESupportedGameFlags::MP},
		{"t6sp.exe", ESupportedGames::BlackOps2, ESupportedGameFlags::SP},
		// Black Ops 3
		{"blackops3.exe", ESupportedGames::BlackOps3, ESupportedGameFlags::SP},
		// Black Ops 4
		{"blackops4.exe", ESupportedGames::BlackOps4, ESupportedGameFlags::SP},
		// Black Ops CW
		{"blackopscoldwar.exe", ESupportedGames::BlackOpsCW, ESupportedGameFlags::SP},
		// Modern Warfare
		{"iw3sp.exe", ESupportedGames::ModernWarfare, ESupportedGameFlags::SP},
		{"iw3mp.exe", ESupportedGames::ModernWarfare, ESupportedGameFlags::MP},
		// Modern Warfare 2
		{"iw4sp.exe", ESupportedGames::ModernWarfare2, ESupportedGameFlags::SP},
		{"iw4mp.exe", ESupportedGames::ModernWarfare2, ESupportedGameFlags::MP},
		// Modern Warfare 3
		{"iw5sp.exe", ESupportedGames::ModernWarfare3, ESupportedGameFlags::SP},
		{"iw5mp.exe", ESupportedGames::ModernWarfare3, ESupportedGameFlags::MP},
		// Ghosts
		{"iw6sp64_ship.exe", ESupportedGames::Ghosts, ESupportedGameFlags::SP},
		{"iw6mp64_ship.exe", ESupportedGames::Ghosts, ESupportedGameFlags::MP},
		// Advanced Warfare
		{"s1_sp64_ship.exe", ESupportedGames::AdvancedWarfare, ESupportedGameFlags::SP},
		{"s1_mp64_ship.exe", ESupportedGames::AdvancedWarfare, ESupportedGameFlags::MP},
		// Modern Warfare Remastered
		{"h1_sp64_ship.exe", ESupportedGames::ModernWarfareRemastered, ESupportedGameFlags::SP},
		{"h1_mp64_ship.exe", ESupportedGames::ModernWarfareRemastered, ESupportedGameFlags::MP},
		// Infinite Warfare
		{"iw7_ship.exe", ESupportedGames::InfiniteWarfare, ESupportedGameFlags::SP},
		// World War II
		{"s2_sp64_ship.exe", ESupportedGames::WorldWar2, ESupportedGameFlags::SP},
		{"s2_mp64_ship.exe", ESupportedGames::WorldWar2, ESupportedGameFlags::MP},
		// Cordycep
		{"Cordycep.CLI.exe", ESupportedGames::Parasyte, ESupportedGameFlags::SP},
		// Modern Warfare 2 Remastered
		{"mw2cr.exe", ESupportedGames::ModernWarfare2Remastered, ESupportedGameFlags::SP},
		// 007 Quantum Solace
		{"jb_liveengine_s.exe", ESupportedGames::QuantumSolace, ESupportedGameFlags::SP},
	};
};

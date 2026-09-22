// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "MoraleBreak.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "MoraleBreakLog.h"
#include "MoraleBreakSubsystem.h"

DEFINE_LOG_CATEGORY(LogMoraleBreak);

#define LOCTEXT_NAMESPACE "FMoraleBreakModule"

namespace
{
	UWorld* MoraleBreakConsoleWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game))
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	void MoraleBreakDebugCommand(const TArray<FString>& Args)
	{
		UWorld* World = MoraleBreakConsoleWorld();
		UMoraleBreakSubsystem* Morale = World ? World->GetSubsystem<UMoraleBreakSubsystem>() : nullptr;
		if (!Morale)
		{
			UE_LOG(LogMoraleBreak, Warning, TEXT("MoraleBreak.Debug: no running world."));
			return;
		}

		Morale->bDebugDraw = Args.Num() > 0 ? (FCString::Atoi(*Args[0]) != 0) : !Morale->bDebugDraw;
		UE_LOG(LogMoraleBreak, Display, TEXT("MoraleBreak.Debug: %s"),
			Morale->bDebugDraw ? TEXT("on") : TEXT("off"));
	}

	void MoraleBreakDumpCommand()
	{
		UWorld* World = MoraleBreakConsoleWorld();
		const UMoraleBreakSubsystem* Morale = World ? World->GetSubsystem<UMoraleBreakSubsystem>() : nullptr;
		if (!Morale)
		{
			UE_LOG(LogMoraleBreak, Warning, TEXT("MoraleBreak.Dump: no running world."));
			return;
		}

		TArray<int32> Squads;
		Morale->GetSquadIds(Squads);
		if (Squads.Num() == 0)
		{
			UE_LOG(LogMoraleBreak, Display, TEXT("MoraleBreak: nobody registered."));
			return;
		}

		for (const int32 SquadId : Squads)
		{
			const FMoraleSquadStats Stats = Morale->GetSquadStats(SquadId);
			UE_LOG(LogMoraleBreak, Display,
				TEXT("Squad %d: %d members, average %.2f, lowest %.2f, %d panicked, %d routed"),
				Stats.SquadId, Stats.Members, Stats.AverageMorale, Stats.LowestMorale,
				Stats.Panicked, Stats.Routed);
		}
	}

	void MoraleBreakRallyCommand(const TArray<FString>& Args)
	{
		UWorld* World = MoraleBreakConsoleWorld();
		UMoraleBreakSubsystem* Morale = World ? World->GetSubsystem<UMoraleBreakSubsystem>() : nullptr;
		if (!Morale)
		{
			UE_LOG(LogMoraleBreak, Warning, TEXT("MoraleBreak.Rally: no running world."));
			return;
		}

		const int32 SquadId = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		Morale->RallySquad(SquadId);
		UE_LOG(LogMoraleBreak, Display, TEXT("MoraleBreak.Rally: squad %d back to steady."), SquadId);
	}

	FAutoConsoleCommand GMoraleBreakDebug(
		TEXT("MoraleBreak.Debug"),
		TEXT("Draw every agent's morale state over its head. MoraleBreak.Debug [0|1]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&MoraleBreakDebugCommand));

	FAutoConsoleCommand GMoraleBreakDump(
		TEXT("MoraleBreak.Dump"),
		TEXT("Log every squad's average and lowest morale and how many have broken."),
		FConsoleCommandDelegate::CreateStatic(&MoraleBreakDumpCommand));

	FAutoConsoleCommand GMoraleBreakRally(
		TEXT("MoraleBreak.Rally"),
		TEXT("Bring a squad straight back to steady. MoraleBreak.Rally [SquadId]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&MoraleBreakRallyCommand));
}

void FMoraleBreakModule::StartupModule()
{
}

void FMoraleBreakModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMoraleBreakModule, MoraleBreak)

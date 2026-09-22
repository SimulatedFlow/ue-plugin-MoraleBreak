// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "MoraleBreakSubsystem.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "MoraleBreakLog.h"
#include "MoraleBreakSettings.h"
#include "MoraleBreakStatics.h"
#include "MoraleComponent.h"

void UMoraleBreakSubsystem::RegisterComponent(UMoraleComponent* Component)
{
	if (!IsValid(Component))
	{
		return;
	}
	Components.AddUnique(Component);
}

void UMoraleBreakSubsystem::UnregisterComponent(UMoraleComponent* Component)
{
	Components.RemoveAll([Component](const TWeakObjectPtr<UMoraleComponent>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Component;
	});
}

void UMoraleBreakSubsystem::DropDeadComponents()
{
	Components.RemoveAll([](const TWeakObjectPtr<UMoraleComponent>& Entry)
	{
		return !Entry.IsValid();
	});
}

void UMoraleBreakSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UMoraleBreakSettings* Settings = UMoraleBreakSettings::Get();

	// Morale is not a per-frame quantity. Accumulating and stepping on an interval keeps the cost flat
	// and - because the whole step is expressed per second - produces the same result either way.
	Accumulated += DeltaTime;
	const float Interval = FMath::Max(Settings->SquadUpdateInterval, 0.02f);
	if (Accumulated < Interval)
	{
		return;
	}

	const float Step = Accumulated;
	Accumulated = 0.0f;

	DropDeadComponents();
	if (Components.Num() == 0)
	{
		return;
	}

	// One pass to total each squad, one to advance everybody. The obvious alternative - every component
	// asking the subsystem for its squad's average - is the same work repeated once per member, and it is
	// quadratic in squad size at exactly the moment a squad is most interesting.
	TMap<int32, float> Totals;
	TMap<int32, int32> Counts;
	for (const TWeakObjectPtr<UMoraleComponent>& Entry : Components)
	{
		const UMoraleComponent* Component = Entry.Get();
		if (!Component)
		{
			continue;
		}
		Totals.FindOrAdd(Component->GetSquadId()) += Component->GetMorale();
		Counts.FindOrAdd(Component->GetSquadId()) += 1;
	}

	for (const TWeakObjectPtr<UMoraleComponent>& Entry : Components)
	{
		UMoraleComponent* Component = Entry.Get();
		if (!Component)
		{
			continue;
		}

		const int32 Squad = Component->GetSquadId();
		const int32 Count = Counts.FindRef(Squad);
		const float Average = Count > 0 ? Totals.FindRef(Squad) / Count : Component->GetMorale();

		Component->AdvanceMorale(Step, Average);
	}

	if (bDebugDraw || Settings->bDrawDebugMorale)
	{
		const UWorld* World = GetWorld();
		for (const TWeakObjectPtr<UMoraleComponent>& Entry : Components)
		{
			const UMoraleComponent* Component = Entry.Get();
			const AActor* Owner = Component ? Component->GetOwner() : nullptr;
			if (!World || !Owner)
			{
				continue;
			}

			FColor Colour = FColor::Green;
			switch (Component->GetState())
			{
			case EMoraleState::Shaken:   Colour = FColor::Yellow; break;
			case EMoraleState::Panicked: Colour = FColor::Orange; break;
			case EMoraleState::Routed:   Colour = FColor::Red; break;
			default: break;
			}

			DrawDebugString(World, Owner->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f),
				FString::Printf(TEXT("%s %.2f"),
					*UEnum::GetDisplayValueAsText(Component->GetState()).ToString(),
					Component->GetMorale()),
				nullptr, Colour, Step, true);
		}
	}
}

void UMoraleBreakSubsystem::ReportEventAtLocation(EMoraleEvent Event, const FVector& Location,
	float Radius, int32 SquadFilter, float Scale)
{
	const UMoraleBreakSettings* Settings = UMoraleBreakSettings::Get();

	// A radius of zero means "use the project's falloff", which is what nearly every call site wants;
	// passing an explicit one is for the grenade that was louder than usual.
	const float Full = Settings->FullEffectRadius;
	const float Outer = Radius > 0.0f ? Radius : Settings->NoEffectRadius;

	DropDeadComponents();

	for (const TWeakObjectPtr<UMoraleComponent>& Entry : Components)
	{
		UMoraleComponent* Component = Entry.Get();
		AActor* Owner = Component ? Component->GetOwner() : nullptr;
		if (!Owner)
		{
			continue;
		}
		if (SquadFilter >= 0 && Component->GetSquadId() != SquadFilter)
		{
			continue;
		}

		const float Distance = FVector::Dist(Owner->GetActorLocation(), Location);
		const float Falloff = UMoraleBreakStatics::DistanceFalloff(Distance, Full, Outer);
		if (Falloff <= 0.0f)
		{
			continue;
		}

		Component->ReportEvent(Event, Scale * Falloff);
	}
}

void UMoraleBreakSubsystem::RallySquad(int32 SquadId)
{
	DropDeadComponents();
	for (const TWeakObjectPtr<UMoraleComponent>& Entry : Components)
	{
		UMoraleComponent* Component = Entry.Get();
		if (Component && Component->GetSquadId() == SquadId)
		{
			Component->Rally();
		}
	}
}

FMoraleSquadStats UMoraleBreakSubsystem::GetSquadStats(int32 SquadId) const
{
	FMoraleSquadStats Stats;
	Stats.SquadId = SquadId;
	Stats.LowestMorale = 1.0f;

	float Total = 0.0f;
	for (const TWeakObjectPtr<UMoraleComponent>& Entry : Components)
	{
		const UMoraleComponent* Component = Entry.Get();
		if (!Component || Component->GetSquadId() != SquadId)
		{
			continue;
		}

		++Stats.Members;
		Total += Component->GetMorale();
		Stats.LowestMorale = FMath::Min(Stats.LowestMorale, Component->GetMorale());

		if (Component->GetState() == EMoraleState::Panicked)
		{
			++Stats.Panicked;
		}
		else if (Component->GetState() == EMoraleState::Routed)
		{
			++Stats.Routed;
		}
	}

	Stats.AverageMorale = Stats.Members > 0 ? Total / Stats.Members : 1.0f;
	if (Stats.Members == 0)
	{
		Stats.LowestMorale = 1.0f;
	}
	return Stats;
}

void UMoraleBreakSubsystem::GetSquadIds(TArray<int32>& Out) const
{
	Out.Reset();
	for (const TWeakObjectPtr<UMoraleComponent>& Entry : Components)
	{
		if (const UMoraleComponent* Component = Entry.Get())
		{
			Out.AddUnique(Component->GetSquadId());
		}
	}
	Out.Sort();
}

TStatId UMoraleBreakSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMoraleBreakSubsystem, STATGROUP_Tickables);
}

void UMoraleBreakSubsystem::Deinitialize()
{
	Components.Reset();
	Super::Deinitialize();
}

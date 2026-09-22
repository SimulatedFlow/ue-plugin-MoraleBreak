// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "MoraleComponent.h"

#include "Engine/World.h"
#include "MoraleBreakLog.h"
#include "MoraleBreakSettings.h"
#include "MoraleBreakStatics.h"
#include "MoraleBreakSubsystem.h"

UMoraleComponent::UMoraleComponent()
{
	// Driven by the subsystem, which computes the squad average once and hands it to everybody. A tick
	// here would make every agent recompute the same number.
	PrimaryComponentTick.bCanEverTick = false;
}

void UMoraleComponent::BeginPlay()
{
	Super::BeginPlay();

	const UMoraleBreakSettings* Settings = UMoraleBreakSettings::Get();

	// Nerve is rolled once and then never again. Re-rolling it - which is what happens if you compute it
	// on demand from a hash of something mutable - means an agent's character changes mid-fight.
	Nerve = (NerveOverride >= 0.0f)
		? FMath::Clamp(NerveOverride, 0.0f, 1.0f)
		: FMath::FRandRange(FMath::Min(Settings->NerveMin, Settings->NerveMax),
			FMath::Max(Settings->NerveMin, Settings->NerveMax));

	if (UMoraleBreakSubsystem* Subsystem = UMoraleBreakStatics::GetMoraleBreak(this))
	{
		Subsystem->RegisterComponent(this);
	}
}

void UMoraleComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UMoraleBreakSubsystem* Subsystem = UMoraleBreakStatics::GetMoraleBreak(this))
	{
		Subsystem->UnregisterComponent(this);
	}
	Super::EndPlay(Reason);
}

void UMoraleComponent::ReportEvent(EMoraleEvent Event, float Scale)
{
	const float Weight = UMoraleBreakSettings::Get()->WeightFor(Event) * Scale;
	if (FMath::IsNearlyZero(Weight))
	{
		return;
	}

	Morale = UMoraleBreakStatics::ApplyMoraleEvent(Morale, Weight, Nerve);

	// Only bad news resets the recovery delay. Good news that also postponed recovery would mean a squad
	// that keeps winning never calms down, which is backwards.
	if (Weight < 0.0f)
	{
		SecondsSinceBadNews = 0.0f;
	}

	ResolveAndBroadcast(Event);
}

void UMoraleComponent::ReportCustomEvent(float Weight)
{
	if (FMath::IsNearlyZero(Weight))
	{
		return;
	}

	Morale = UMoraleBreakStatics::ApplyMoraleEvent(Morale, Weight, Nerve);
	if (Weight < 0.0f)
	{
		SecondsSinceBadNews = 0.0f;
	}
	ResolveAndBroadcast(EMoraleEvent::Custom);
}

void UMoraleComponent::SetMorale(float NewMorale, EMoraleEvent Reason)
{
	Morale = FMath::Clamp(NewMorale, 0.0f, 1.0f);
	ResolveAndBroadcast(Reason);
}

void UMoraleComponent::SetNerve(float NewNerve)
{
	Nerve = FMath::Clamp(NewNerve, 0.0f, 1.0f);
}

void UMoraleComponent::Rally()
{
	const EMoraleState Old = State;

	Morale = 1.0f;
	State = EMoraleState::Steady;
	SecondsInState = 0.0f;
	LastReason = EMoraleEvent::ReinforcementsArrived;

	// Rally is an override, so it ignores the minimum dwell time and the rout minimum by design - it is
	// the scripted moment where the officer arrives, and a scripted moment that silently does not happen
	// is worse than no feature.
	if (Old != State)
	{
		OnStateChanged.Broadcast(Old, State, LastReason);
	}
	OnMoraleChanged.Broadcast(Morale, LastReason);
}

void UMoraleComponent::ForceRout()
{
	const EMoraleState Old = State;

	Morale = 0.0f;
	State = EMoraleState::Routed;
	SecondsInState = 0.0f;
	LastReason = EMoraleEvent::Custom;

	if (Old != State)
	{
		OnStateChanged.Broadcast(Old, State, LastReason);
	}
	OnMoraleChanged.Broadcast(Morale, LastReason);
}

void UMoraleComponent::SetSquadId(int32 NewSquadId)
{
	SquadId = NewSquadId;
}

FMoraleSnapshot UMoraleComponent::GetSnapshot() const
{
	FMoraleSnapshot Snapshot;
	Snapshot.Morale = Morale;
	Snapshot.Nerve = Nerve;
	Snapshot.State = State;
	Snapshot.SecondsInState = SecondsInState;
	Snapshot.LastReason = LastReason;
	Snapshot.SquadId = SquadId;
	return Snapshot;
}

void UMoraleComponent::AdvanceMorale(float DeltaSeconds, float SquadAverage)
{
	const UMoraleBreakSettings* Settings = UMoraleBreakSettings::Get();
	const float Dt = FMath::Max(DeltaSeconds, 0.0f);

	SecondsInState += Dt;
	SecondsSinceBadNews += Dt;

	const float Before = Morale;

	// Calm returns, but not immediately. Without the delay an agent recovers between two bursts of the
	// same magazine, and nothing ever accumulates.
	if (SecondsSinceBadNews >= Settings->RecoveryDelaySeconds)
	{
		Morale = UMoraleBreakStatics::RecoverMorale(Morale, Settings->RecoveryPerSecond, Dt);
	}

	if (bAffectedByContagion)
	{
		Morale = UMoraleBreakStatics::ContagionPull(Morale, SquadAverage, Settings->ContagionPerSecond, Dt);
	}

	// A routed agent may come back, if it has been long enough and if morale has genuinely recovered past
	// the hysteresis band. Both conditions, not either.
	if (State == EMoraleState::Routed)
	{
		if (UMoraleBreakStatics::CanReturnFromRout(Morale, SecondsInState, Settings->MinRoutSeconds,
			Settings->PanicBelow, Settings->Hysteresis))
		{
			const EMoraleState Old = State;
			State = EMoraleState::Shaken;
			SecondsInState = 0.0f;
			LastReason = EMoraleEvent::Rest;
			OnStateChanged.Broadcast(Old, State, LastReason);
		}
	}
	else if (UMoraleBreakStatics::ShouldRout(State, SecondsInState, Settings->RoutAfterSeconds))
	{
		const EMoraleState Old = State;
		State = EMoraleState::Routed;
		SecondsInState = 0.0f;
		OnStateChanged.Broadcast(Old, State, LastReason);
	}
	else
	{
		ResolveAndBroadcast(LastReason);
	}

	if (!FMath::IsNearlyEqual(Before, Morale, 0.0005f))
	{
		OnMoraleChanged.Broadcast(Morale, LastReason);
	}
}

void UMoraleComponent::ResolveAndBroadcast(EMoraleEvent Reason)
{
	const UMoraleBreakSettings* Settings = UMoraleBreakSettings::Get();

	LastReason = Reason;

	const EMoraleState Next = UMoraleBreakStatics::ResolveState(Morale, State,
		Settings->ShakenBelow, Settings->PanicBelow, Settings->Hysteresis);

	if (Next == State)
	{
		return;
	}

	// The second flicker guard, and it is worth having on top of the hysteresis band: a burst of four
	// events in one frame can jump clean across the band, and an agent that changed state 30 ms ago has
	// not finished the animation that says so.
	if (SecondsInState < Settings->MinimumStateSeconds)
	{
		return;
	}

	const EMoraleState Old = State;
	State = Next;
	SecondsInState = 0.0f;

	UE_LOG(LogMoraleBreak, Verbose, TEXT("%s: %s -> %s (morale %.2f, nerve %.2f)"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("<none>"),
		*UEnum::GetValueAsString(Old), *UEnum::GetValueAsString(State), Morale, Nerve);

	OnStateChanged.Broadcast(Old, State, Reason);
}

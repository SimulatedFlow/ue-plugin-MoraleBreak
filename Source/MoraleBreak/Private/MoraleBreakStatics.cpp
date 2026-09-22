// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "MoraleBreakStatics.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "MoraleBreakSubsystem.h"

UMoraleBreakSubsystem* UMoraleBreakStatics::GetMoraleBreak(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	return World ? World->GetSubsystem<UMoraleBreakSubsystem>() : nullptr;
}

void UMoraleBreakStatics::ReportEventAtLocation(const UObject* WorldContextObject, EMoraleEvent Event,
	const FVector& Location, float Radius, int32 SquadFilter, float Scale)
{
	if (UMoraleBreakSubsystem* Morale = GetMoraleBreak(WorldContextObject))
	{
		Morale->ReportEventAtLocation(Event, Location, Radius, SquadFilter, Scale);
	}
}

FMoraleSquadStats UMoraleBreakStatics::GetSquadStats(const UObject* WorldContextObject, int32 SquadId)
{
	if (const UMoraleBreakSubsystem* Morale = GetMoraleBreak(WorldContextObject))
	{
		return Morale->GetSquadStats(SquadId);
	}
	return FMoraleSquadStats();
}

// --------------------------------------------------------------------------------------------- rules

float UMoraleBreakStatics::ApplyMoraleEvent(float CurrentMorale, float EventWeight, float Nerve)
{
	// Nerve resists in proportion, and is clamped first so that a project which stores nerve in some other
	// range cannot turn resistance into amplification - or, at Nerve > 1, into a sign flip that would have
	// good news frightening people.
	const float Resistance = 1.0f - FMath::Clamp(Nerve, 0.0f, 1.0f);
	return FMath::Clamp(CurrentMorale + EventWeight * Resistance, 0.0f, 1.0f);
}

float UMoraleBreakStatics::RecoverMorale(float CurrentMorale, float RecoveryPerSecond, float DeltaSeconds)
{
	return FMath::Clamp(CurrentMorale + RecoveryPerSecond * FMath::Max(DeltaSeconds, 0.0f), 0.0f, 1.0f);
}

float UMoraleBreakStatics::ContagionPull(float OwnMorale, float SquadAverage, float StrengthPerSecond,
	float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.0f);
	const float Strength = FMath::Max(StrengthPerSecond, 0.0f);
	if (Dt <= 0.0f || Strength <= 0.0f)
	{
		return OwnMorale;
	}

	// Exponential approach. The alternative everyone writes first - Own += (Avg - Own) * Strength * Dt -
	// is only equal to this for tiny steps, overshoots for large ones, and composes differently depending
	// on how many times it ran. This form composes exactly: two half-seconds are one second, which is
	// what the test asserts and what keeps the game the same on every machine.
	const float Alpha = 1.0f - FMath::Exp(-Strength * Dt);
	return FMath::Clamp(OwnMorale + (SquadAverage - OwnMorale) * Alpha, 0.0f, 1.0f);
}

EMoraleState UMoraleBreakStatics::ResolveState(float Morale, EMoraleState CurrentState,
	float ShakenBelow, float PanicBelow, float Hysteresis)
{
	const float Band = FMath::Max(Hysteresis, 0.0f);

	// A routed agent is not talked out of it by arithmetic. Coming back is a decision about time, and it
	// belongs to CanReturnFromRout(); answering it here would let an agent rally in the same frame it
	// broke, which is the exact flicker this function exists to prevent.
	if (CurrentState == EMoraleState::Routed)
	{
		return EMoraleState::Routed;
	}

	// Going down: the plain thresholds.
	if (Morale < PanicBelow)
	{
		return EMoraleState::Panicked;
	}
	if (Morale < ShakenBelow)
	{
		// Climbing out of panic costs the extra band. Without it, an agent parked on PanicBelow changes
		// state every frame, and every frame fires a delegate, a bark and an animation.
		if (CurrentState == EMoraleState::Panicked && Morale < PanicBelow + Band)
		{
			return EMoraleState::Panicked;
		}
		return EMoraleState::Shaken;
	}

	// Above ShakenBelow. Same argument one level up: recovering to Steady costs the band on top.
	if (CurrentState != EMoraleState::Steady && Morale < ShakenBelow + Band)
	{
		return (CurrentState == EMoraleState::Panicked && Morale < PanicBelow + Band)
			? EMoraleState::Panicked
			: EMoraleState::Shaken;
	}

	return EMoraleState::Steady;
}

bool UMoraleBreakStatics::ShouldRout(EMoraleState State, float SecondsInState, float RoutAfterSeconds)
{
	// A rout time of zero means this project does not want routing at all, rather than wanting it
	// instantly - instant routing is indistinguishable from deleting the agent.
	return State == EMoraleState::Panicked
		&& RoutAfterSeconds > 0.0f
		&& SecondsInState >= RoutAfterSeconds;
}

bool UMoraleBreakStatics::CanReturnFromRout(float Morale, float SecondsRouted, float MinRoutSeconds,
	float PanicBelow, float Hysteresis)
{
	if (SecondsRouted < FMath::Max(MinRoutSeconds, 0.0f))
	{
		return false;
	}
	return Morale >= PanicBelow + FMath::Max(Hysteresis, 0.0f);
}

float UMoraleBreakStatics::OutnumberedFactor(int32 Friends, int32 Foes)
{
	// Both sides floored at one. Zero enemies is the moment the last one dies - which is exactly when
	// this gets called - and a ratio with a zero in it is either a division by zero or an infinity that
	// propagates into morale and stays there.
	const float F = static_cast<float>(FMath::Max(Friends, 1));
	const float E = static_cast<float>(FMath::Max(Foes, 1));

	// Deliberately gentle: a square root, not a ratio. Three-to-one odds are frightening, they are not
	// three times as frightening, and the linear version makes a horde game unplayable the moment the
	// horde arrives.
	return FMath::Clamp(FMath::Sqrt(E / F), 0.25f, 4.0f);
}

float UMoraleBreakStatics::DistanceFalloff(float Distance, float FullEffectRadius, float NoEffectRadius)
{
	const float Full = FMath::Max(FullEffectRadius, 0.0f);
	const float None = FMath::Max(NoEffectRadius, Full);

	if (Distance <= Full)
	{
		return 1.0f;
	}
	if (Distance >= None || None <= Full)
	{
		return 0.0f;
	}
	return 1.0f - (Distance - Full) / (None - Full);
}

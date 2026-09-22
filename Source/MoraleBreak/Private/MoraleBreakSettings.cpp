// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "MoraleBreakSettings.h"

UMoraleBreakSettings::UMoraleBreakSettings()
	: ShakenBelow(0.60f)
	, PanicBelow(0.25f)
	// 0.15 is roughly two ordinary events wide. Narrow enough that recovery still feels responsive, wide
	// enough that nothing sits on a threshold and vibrates.
	, Hysteresis(0.15f)
	, MinimumStateSeconds(0.75f)
	, NerveMin(0.15f)
	, NerveMax(0.85f)
	, RecoveryPerSecond(0.06f)
	, RecoveryDelaySeconds(4.0f)
	// 0.5 closes about 40% of the gap to the squad in a second. High enough that a squad reads as one
	// body, low enough that individual nerve still shows: at 0.8 the members converge inside two
	// seconds and every man in the squad has the same morale, which throws away the reason nerve exists.
	, ContagionPerSecond(0.5f)
	, SquadUpdateInterval(0.25f)
	, RoutAfterSeconds(3.5f)
	, MinRoutSeconds(6.0f)
	, FullEffectRadius(600.0f)
	, NoEffectRadius(3000.0f)
	, bDrawDebugMorale(false)
{
	// A starting table, not a claim to be right for any particular game. The relative sizes are the part
	// worth keeping: a leader is worth several ordinary men, being shot at matters nearly as much as
	// being hit, and nothing good is ever as strong as something bad - which is why a fight that turns
	// takes a while to turn back.
	EventWeights.Add(EMoraleEvent::AllyDown, -0.18f);
	EventWeights.Add(EMoraleEvent::LeaderDown, -0.45f);
	EventWeights.Add(EMoraleEvent::TookDamage, -0.10f);
	EventWeights.Add(EMoraleEvent::Suppressed, -0.07f);
	EventWeights.Add(EMoraleEvent::Outnumbered, -0.05f);
	EventWeights.Add(EMoraleEvent::LowHealth, -0.15f);
	EventWeights.Add(EMoraleEvent::EnemyDown, 0.08f);
	EventWeights.Add(EMoraleEvent::ReinforcementsArrived, 0.25f);
	EventWeights.Add(EMoraleEvent::LeaderNearby, 0.04f);
	EventWeights.Add(EMoraleEvent::Rest, 0.03f);
	EventWeights.Add(EMoraleEvent::Custom, 0.0f);
}

const UMoraleBreakSettings* UMoraleBreakSettings::Get()
{
	const UMoraleBreakSettings* Settings = GetDefault<UMoraleBreakSettings>();
	check(Settings);
	return Settings;
}

float UMoraleBreakSettings::WeightFor(EMoraleEvent Event) const
{
	// Absent means zero, on purpose: deleting a row is how a project switches an event off, and that
	// should not be a crash or a silent fallback to some hard-coded number.
	if (const float* Found = EventWeights.Find(Event))
	{
		return *Found;
	}
	return 0.0f;
}

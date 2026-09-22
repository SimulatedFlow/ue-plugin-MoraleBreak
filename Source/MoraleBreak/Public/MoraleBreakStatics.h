// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MoraleBreakTypes.h"
#include "MoraleBreakStatics.generated.h"

class UMoraleBreakSubsystem;
class UMoraleComponent;

/**
 * MoraleBreak from Blueprint, and the rules on their own.
 *
 * Everything below the divider is pure. The component calls exactly these functions and so do the
 * tests, which is the only arrangement in which a green test suite is evidence about the game rather
 * than about a second implementation that happens to live in the test file.
 */
UCLASS(meta = (ScriptName = "MoraleBreakStatics"))
class MORALEBREAK_API UMoraleBreakStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** This world's morale bookkeeping. May be null outside a world. */
	UFUNCTION(BlueprintPure, Category = "MoraleBreak", meta = (WorldContext = "WorldContextObject"))
	static UMoraleBreakSubsystem* GetMoraleBreak(const UObject* WorldContextObject);

	/**
	 * Tell everyone near a place that something happened.
	 *
	 * The usual way to report a death: one call at the spot, and the agents who were close enough to
	 * notice take it personally in proportion to how close they were.
	 */
	UFUNCTION(BlueprintCallable, Category = "MoraleBreak", meta = (WorldContext = "WorldContextObject"))
	static void ReportEventAtLocation(const UObject* WorldContextObject, EMoraleEvent Event,
		const FVector& Location, float Radius, int32 SquadFilter = -1, float Scale = 1.0f);

	/** What a whole squad looks like. */
	UFUNCTION(BlueprintPure, Category = "MoraleBreak", meta = (WorldContext = "WorldContextObject"))
	static FMoraleSquadStats GetSquadStats(const UObject* WorldContextObject, int32 SquadId);

	// ----------------------------------------------------------------------------------------------------
	// The rules, on their own
	// ----------------------------------------------------------------------------------------------------

	/**
	 * What one event does to one agent.
	 *
	 * Nerve resists: the same bad news moves a steady man less than a nervous one. It resists, it never
	 * reverses - a high-nerve agent shrugs off a death, he is not cheered up by it - which is why the sign
	 * of the weight survives untouched.
	 *
	 * @param EventWeight Negative for bad news, positive for good. Comes from the settings table.
	 * @param Nerve       0 (falls apart at anything) to 1 (very hard to move).
	 */
	UFUNCTION(BlueprintPure, Category = "MoraleBreak|Rules")
	static float ApplyMoraleEvent(float CurrentMorale, float EventWeight, float Nerve);

	/**
	 * Calm returning over time.
	 *
	 * Per second, not per call. A recovery that runs per tick heals twice as fast at 120 fps as at 60,
	 * which turns a difficulty setting into a hardware setting.
	 */
	UFUNCTION(BlueprintPure, Category = "MoraleBreak|Rules")
	static float RecoverMorale(float CurrentMorale, float RecoveryPerSecond, float DeltaSeconds);

	/**
	 * The pull towards the rest of the squad. This is what makes a rout look like a rout.
	 *
	 * Exponential approach, so that the result depends on how much TIME passed and not on how many times
	 * it was called: two steps of half a second land exactly where one step of a whole second does. That
	 * is not a nicety. A linear version makes panic spread faster on a fast machine, and the bug reads as
	 * "the AI is more aggressive on my PC than on the build machine".
	 *
	 * @param StrengthPerSecond 0 leaves everyone to their own thoughts; 1 closes about 63% of the gap in
	 *                          a second; large values make the squad a single organism, which is wrong
	 *                          for infantry and right for a hive.
	 */
	UFUNCTION(BlueprintPure, Category = "MoraleBreak|Rules")
	static float ContagionPull(float OwnMorale, float SquadAverage, float StrengthPerSecond,
		float DeltaSeconds);

	/**
	 * Morale in, state out - with the hysteresis that stops the flicker.
	 *
	 * Falling, the thresholds are exactly ShakenBelow and PanicBelow. Rising, both are lifted by the
	 * hysteresis band, so an agent sitting at the line does not change its mind sixty times a second.
	 * Without this the whole system is unusable and the symptom is not "morale is wrong", it is
	 * "the animations stutter".
	 *
	 * Routed is not decided here; it is a question about time, and it lives in ShouldRout().
	 */
	UFUNCTION(BlueprintPure, Category = "MoraleBreak|Rules")
	static EMoraleState ResolveState(float Morale, EMoraleState CurrentState,
		float ShakenBelow, float PanicBelow, float Hysteresis);

	/** Panicked for long enough that the agent gives up entirely. */
	UFUNCTION(BlueprintPure, Category = "MoraleBreak|Rules")
	static bool ShouldRout(EMoraleState State, float SecondsInState, float RoutAfterSeconds);

	/**
	 * May a routed agent turn round and come back?
	 *
	 * Two conditions, and the time one is not optional: an agent who breaks and rallies inside half a
	 * second has not done anything the player can read. The minimum is what makes a rout an event rather
	 * than a wobble.
	 */
	UFUNCTION(BlueprintPure, Category = "MoraleBreak|Rules")
	static bool CanReturnFromRout(float Morale, float SecondsRouted, float MinRoutSeconds,
		float PanicBelow, float Hysteresis);

	/**
	 * How much worse things are because of the numbers.
	 *
	 * 1.0 at even odds, above 1 when outnumbered, below 1 when we have the upper hand - a multiplier for
	 * the Outnumbered event weight. Defined for zero on either side, because a squad that has just wiped
	 * out the last enemy is exactly when this gets called with a zero.
	 */
	UFUNCTION(BlueprintPure, Category = "MoraleBreak|Rules")
	static float OutnumberedFactor(int32 Friends, int32 Foes);

	/**
	 * How much of an event reaches somebody standing this far away.
	 *
	 * 1 inside the full radius, fading to 0 at the outer one. A death two rooms away is news; a death at
	 * your elbow is not the same news.
	 */
	UFUNCTION(BlueprintPure, Category = "MoraleBreak|Rules")
	static float DistanceFalloff(float Distance, float FullEffectRadius, float NoEffectRadius);
};

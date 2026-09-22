// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MoraleBreakTypes.h"
#include "MoraleBreakSettings.generated.h"

/**
 * Project Settings > Plugins > MoraleBreak.
 *
 * The event weights live here rather than in code so that tuning how brittle an army is does not need a
 * programmer, and so that a horde shooter and a squad tactics game can both use this plugin without one
 * of them forking it.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "MoraleBreak"))
class MORALEBREAK_API UMoraleBreakSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMoraleBreakSettings();

	virtual FName GetContainerName() const override { return TEXT("Project"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	static const UMoraleBreakSettings* Get();

	/** The weight for one event, or zero if the project has removed it from the table. */
	float WeightFor(EMoraleEvent Event) const;

	// --------------------------------------------------------------------------------------- thresholds

	/** Below this an agent is Shaken. */
	UPROPERTY(config, EditAnywhere, Category = "Thresholds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShakenBelow;

	/** Below this an agent is Panicked. */
	UPROPERTY(config, EditAnywhere, Category = "Thresholds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PanicBelow;

	/**
	 * How much further morale must recover before an agent climbs back out of a state.
	 *
	 * The single most important number in the plugin. At zero, an agent whose morale is sitting on a
	 * threshold changes state every frame - and every change fires a delegate, a bark and an animation.
	 * The symptom is never reported as "morale is wrong"; it is reported as stuttering.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Thresholds", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float Hysteresis;

	/** No state may be left before this many seconds have passed in it. The second flicker guard. */
	UPROPERTY(config, EditAnywhere, Category = "Thresholds", meta = (ClampMin = "0.0", UIMax = "5.0"))
	float MinimumStateSeconds;

	// ------------------------------------------------------------------------------------------- nerve

	/** An agent's nerve is rolled once at spawn, uniformly between these. */
	UPROPERTY(config, EditAnywhere, Category = "Nerve", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NerveMin;

	UPROPERTY(config, EditAnywhere, Category = "Nerve", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NerveMax;

	// ---------------------------------------------------------------------------------------- recovery

	/** Morale returned per second when nothing is happening. */
	UPROPERTY(config, EditAnywhere, Category = "Recovery", meta = (ClampMin = "0.0", UIMax = "1.0"))
	float RecoveryPerSecond;

	/** Seconds after the last bad news before recovery starts. */
	UPROPERTY(config, EditAnywhere, Category = "Recovery", meta = (ClampMin = "0.0", UIMax = "30.0"))
	float RecoveryDelaySeconds;

	// --------------------------------------------------------------------------------------- contagion

	/**
	 * How hard the squad pulls an individual towards its average, per second.
	 *
	 * Zero gives a squad of strangers who happen to stand near each other. One is about right for
	 * infantry who know each other. Very high values make the squad a single organism - correct for a
	 * hive, wrong for people.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Contagion", meta = (ClampMin = "0.0", UIMax = "5.0"))
	float ContagionPerSecond;

	/** How often the squad averages are recomputed. Not every frame; morale is not that urgent. */
	UPROPERTY(config, EditAnywhere, Category = "Contagion", meta = (ClampMin = "0.02", UIMax = "2.0"))
	float SquadUpdateInterval;

	// -------------------------------------------------------------------------------------------- rout

	/** Seconds of unbroken panic before an agent gives up entirely. Zero switches routing off. */
	UPROPERTY(config, EditAnywhere, Category = "Rout", meta = (ClampMin = "0.0", UIMax = "60.0"))
	float RoutAfterSeconds;

	/** The shortest a rout may last, whatever morale does. Below a couple of seconds it reads as a twitch. */
	UPROPERTY(config, EditAnywhere, Category = "Rout", meta = (ClampMin = "0.0", UIMax = "60.0"))
	float MinRoutSeconds;

	// ------------------------------------------------------------------------------------------ events

	/**
	 * What each event is worth. Negative is bad news.
	 *
	 * Anything missing from the table is worth nothing, which is a supported way to switch an event off.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Events")
	TMap<EMoraleEvent, float> EventWeights;

	/** Inside this radius an event lands at full weight. */
	UPROPERTY(config, EditAnywhere, Category = "Events", meta = (ClampMin = "0.0", UIMax = "5000.0"))
	float FullEffectRadius;

	/** Beyond this it is not news at all. */
	UPROPERTY(config, EditAnywhere, Category = "Events", meta = (ClampMin = "0.0", UIMax = "20000.0"))
	float NoEffectRadius;

	// ------------------------------------------------------------------------------------------- debug

	/** Draw every agent's state over its head. Also switchable live with MoraleBreak.Debug. */
	UPROPERTY(config, EditAnywhere, Category = "Debug")
	bool bDrawDebugMorale;
};

// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MoraleBreakTypes.generated.h"

/**
 * How an agent is holding up.
 *
 * Four states, not a bool, because the interesting behaviour lives in the middle. An agent that is
 * Shaken still fights - it just fights from cover, at longer range, with worse aim. That is where most
 * of the character is, and a two-state system throws it away.
 */
UENUM(BlueprintType)
enum class EMoraleState : uint8
{
	/** Fighting normally. */
	Steady,

	/** Still fighting, but wants cover and distance. */
	Shaken,

	/** Not fighting well. Suppressed, hesitant, looking for a way out. */
	Panicked,

	/** Gone. Running for a rally point and not coming back for a while. */
	Routed
};

/**
 * Why morale moved.
 *
 * The reason travels all the way to the state change, which is the point: a squad that breaks because
 * its sergeant died should say something different from one that breaks because it is outnumbered, and
 * a plugin that only reports the new state makes that impossible without a second bookkeeping system.
 */
UENUM(BlueprintType)
enum class EMoraleEvent : uint8
{
	/** Somebody on our side went down nearby. */
	AllyDown,

	/** The squad leader went down. Worth several ordinary losses. */
	LeaderDown,

	/** We were hit. */
	TookDamage,

	/** Rounds are landing near us, whether or not they connect. */
	Suppressed,

	/** There are more of them than of us. Reported continuously, not once. */
	Outnumbered,

	/** We are badly hurt. */
	LowHealth,

	/** One of theirs went down. */
	EnemyDown,

	/** Help arrived. */
	ReinforcementsArrived,

	/** The leader is here and alive. Quietly the strongest steadying influence there is. */
	LeaderNearby,

	/** Nothing is happening. Used by projects that want an explicit calm-down pulse. */
	Rest,

	/** For a project's own reasons; the weight is passed in at the call site. */
	Custom
};

/** Everything about one agent's state of mind, for a HUD, a debugger or a save file. */
USTRUCT(BlueprintType)
struct MORALEBREAK_API FMoraleSnapshot
{
	GENERATED_BODY()

	/** 0 is broken, 1 is untroubled. */
	UPROPERTY(BlueprintReadOnly, Category = "MoraleBreak")
	float Morale = 1.0f;

	/**
	 * How hard this particular agent is to shake, 0 to 1.
	 *
	 * Rolled once per agent. It is what stops a squad from breaking in unison like a single organism -
	 * one man runs first, and the others watch him do it.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "MoraleBreak")
	float Nerve = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "MoraleBreak")
	EMoraleState State = EMoraleState::Steady;

	/** How long they have been in it. The minimum dwell time is measured against this. */
	UPROPERTY(BlueprintReadOnly, Category = "MoraleBreak")
	float SecondsInState = 0.0f;

	/** What last moved them. */
	UPROPERTY(BlueprintReadOnly, Category = "MoraleBreak")
	EMoraleEvent LastReason = EMoraleEvent::Rest;

	UPROPERTY(BlueprintReadOnly, Category = "MoraleBreak")
	int32 SquadId = 0;
};

/** What a squad looks like as a whole. */
USTRUCT(BlueprintType)
struct MORALEBREAK_API FMoraleSquadStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "MoraleBreak")
	int32 SquadId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "MoraleBreak")
	int32 Members = 0;

	/** The average the contagion pulls everybody towards. */
	UPROPERTY(BlueprintReadOnly, Category = "MoraleBreak")
	float AverageMorale = 1.0f;

	/** The worst man in the squad. Usually the one who breaks first and takes the others with him. */
	UPROPERTY(BlueprintReadOnly, Category = "MoraleBreak")
	float LowestMorale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "MoraleBreak")
	int32 Panicked = 0;

	UPROPERTY(BlueprintReadOnly, Category = "MoraleBreak")
	int32 Routed = 0;
};

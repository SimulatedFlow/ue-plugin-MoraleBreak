// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MoraleBreakTypes.h"
#include "MoraleComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMoraleStateChanged,
	EMoraleState, OldState, EMoraleState, NewState, EMoraleEvent, Reason);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMoraleChanged, float, NewMorale, EMoraleEvent, Reason);

/**
 * Put this on anything that can lose its nerve.
 *
 * It holds a morale value, a private nerve, and the state machine over them. It does not move the
 * agent, pick a rally point or play a sound - it tells you when something changed and why, and the
 * project decides what that means. That separation is deliberate: every game already has its own idea
 * of what running away looks like.
 */
UCLASS(ClassGroup = (MoraleBreak), meta = (BlueprintSpawnableComponent, DisplayName = "Morale"))
class MORALEBREAK_API UMoraleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMoraleComponent();

	// ----------------------------------------------------------------------------------------- events

	/**
	 * Something happened to this agent.
	 *
	 * @param Scale Multiplies the configured weight. Used for distance falloff, for a hit that was worse
	 *              than usual, for odds. One event type, many intensities.
	 */
	UFUNCTION(BlueprintCallable, Category = "MoraleBreak")
	void ReportEvent(EMoraleEvent Event, float Scale = 1.0f);

	/** An event with a weight this call site decides, for anything the table does not cover. */
	UFUNCTION(BlueprintCallable, Category = "MoraleBreak")
	void ReportCustomEvent(float Weight);

	// ---------------------------------------------------------------------------------------- reading

	UFUNCTION(BlueprintPure, Category = "MoraleBreak")
	FMoraleSnapshot GetSnapshot() const;

	UFUNCTION(BlueprintPure, Category = "MoraleBreak")
	EMoraleState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "MoraleBreak")
	float GetMorale() const { return Morale; }

	UFUNCTION(BlueprintPure, Category = "MoraleBreak")
	float GetNerve() const { return Nerve; }

	/** Still willing to fight at all - Steady or Shaken. The one-pin version for a behaviour tree. */
	UFUNCTION(BlueprintPure, Category = "MoraleBreak")
	bool IsFighting() const { return State == EMoraleState::Steady || State == EMoraleState::Shaken; }

	// --------------------------------------------------------------------------------------- driving

	/** Put morale back where you want it. For scripted moments, and for loading a save. */
	UFUNCTION(BlueprintCallable, Category = "MoraleBreak")
	void SetMorale(float NewMorale, EMoraleEvent Reason = EMoraleEvent::Custom);

	/** Set nerve by hand instead of rolling it. For a named character who is supposed to be the brave one. */
	UFUNCTION(BlueprintCallable, Category = "MoraleBreak")
	void SetNerve(float NewNerve);

	/** Straight back to Steady at full morale. The officer arrives and the line holds. */
	UFUNCTION(BlueprintCallable, Category = "MoraleBreak")
	void Rally();

	/** Break on command, for a scripted retreat. */
	UFUNCTION(BlueprintCallable, Category = "MoraleBreak")
	void ForceRout();

	/** Which squad this agent belongs to. Panic spreads inside a squad, not across the map. */
	UFUNCTION(BlueprintCallable, Category = "MoraleBreak")
	void SetSquadId(int32 NewSquadId);

	UFUNCTION(BlueprintPure, Category = "MoraleBreak")
	int32 GetSquadId() const { return SquadId; }

	// -------------------------------------------------------------------------------------- delegates

	/** The one worth binding. Carries the reason, so the right line of dialogue can play. */
	UPROPERTY(BlueprintAssignable, Category = "MoraleBreak")
	FOnMoraleStateChanged OnStateChanged;

	/** Every movement of the value, for a bar on the HUD. */
	UPROPERTY(BlueprintAssignable, Category = "MoraleBreak")
	FOnMoraleChanged OnMoraleChanged;

	// ----------------------------------------------------------------------------------------- config

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MoraleBreak")
	int32 SquadId = 0;

	/** This agent is the squad leader. Losing them is worth several ordinary losses. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoraleBreak")
	bool bIsLeader = false;

	/**
	 * Take part in the squad's contagion.
	 *
	 * Off for the one character who is supposed to stand while everyone around them runs.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoraleBreak")
	bool bAffectedByContagion = true;

	/** Left at -1, nerve is rolled from the project's range at spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoraleBreak", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float NerveOverride = -1.0f;

	// ------------------------------------------------------------------------ called by the subsystem

	/** One step of recovery, contagion and state resolution. Driven by the subsystem, never by a tick. */
	void AdvanceMorale(float DeltaSeconds, float SquadAverage);

	// -------------------------------------------------------------------------------- UActorComponent

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	/** Runs the state machine and fires the delegates. Called after anything moves Morale. */
	void ResolveAndBroadcast(EMoraleEvent Reason);

	float Morale = 1.0f;
	float Nerve = 0.5f;
	EMoraleState State = EMoraleState::Steady;
	float SecondsInState = 0.0f;
	EMoraleEvent LastReason = EMoraleEvent::Rest;

	/** When bad news last arrived, so that recovery can wait a beat before undoing it. */
	float SecondsSinceBadNews = 1000.0f;
};

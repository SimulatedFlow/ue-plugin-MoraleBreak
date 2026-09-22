// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MoraleBreakTypes.h"
#include "MoraleBreakSubsystem.generated.h"

class UMoraleComponent;

/**
 * The squads, the averages, and the one clock everybody runs on.
 *
 * Every morale component is driven from here rather than from its own tick. That is not tidiness: the
 * squad average has to be computed once per squad per step, and a hundred components each asking "what
 * is my squad's average" is a hundred passes over the same list. Once here, handed out to everyone.
 */
UCLASS()
class MORALEBREAK_API UMoraleBreakSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterComponent(UMoraleComponent* Component);
	void UnregisterComponent(UMoraleComponent* Component);

	/**
	 * Tell everyone near a place that something happened.
	 *
	 * @param SquadFilter -1 for everybody in range, or a squad id to limit it to our own people.
	 */
	void ReportEventAtLocation(EMoraleEvent Event, const FVector& Location, float Radius,
		int32 SquadFilter = -1, float Scale = 1.0f);

	/** Bring a whole squad back to Steady. */
	void RallySquad(int32 SquadId);

	FMoraleSquadStats GetSquadStats(int32 SquadId) const;

	/** Every squad that currently has a member, for the debug dump. */
	void GetSquadIds(TArray<int32>& Out) const;

	/** MoraleBreak.Debug - draw each agent's state over its head. */
	bool bDebugDraw = false;

	// ------------------------------------------------------------------------- UTickableWorldSubsystem

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

	/** Nothing to do in an editor world that is not running - morale is a thing that happens in play. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
	{
		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
	}

private:
	void DropDeadComponents();

	UPROPERTY()
	TArray<TWeakObjectPtr<UMoraleComponent>> Components;

	/** Time owed to the next squad update, so the interval is honoured without drifting. */
	float Accumulated = 0.0f;
};

// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoraleBreakTypes.h"
#include "MoraleBreakDemoDirector.generated.h"

class UTextRenderComponent;

/**
 * Runs the shipped demo: a squad of five, two losses, and a line that breaks one man at a time.
 *
 * Morale is a number, and a number is not a demo. So the demo shows the thing the number produces: a
 * squad whose morale bars fall together because panic is contagious, one man with poor nerve who goes
 * first, and a rout that is visible because he leaves. The rally at the end puts the line back.
 *
 * It runs the same pure rules the component runs - the decay, the contagion, the hysteresis - rather
 * than a second implementation that could drift from them. It ticks in the editor viewport, because
 * that is where the store video is filmed.
 */
UCLASS(meta = (DisplayName = "MoraleBreak Demo Director"))
class MORALEBREAK_API AMoraleBreakDemoDirector : public AActor
{
	GENERATED_BODY()

public:
	AMoraleBreakDemoDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	/**
	 * The squad, in order. The first is treated as the leader.
	 *
	 * Leave it empty and the demo draws five markers instead, which is what makes the director worth
	 * dropping into an empty level.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoraleBreak Demo")
	TArray<TObjectPtr<AActor>> SquadActors;

	/** One run of the script. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoraleBreak Demo",
		meta = (ClampMin = "10.0", ClampMax = "180.0"))
	float CycleSeconds = 24.0f;

	/** How far a routed man runs before the cycle restarts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoraleBreak Demo",
		meta = (ClampMin = "0.0", ClampMax = "3000.0"))
	float RoutDistance = 600.0f;

	/**
	 * Which way he runs.
	 *
	 * A property rather than a constant because the demo has to be legible from wherever the level's
	 * camera happens to be: a man fleeing straight towards the lens does not look like he is fleeing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoraleBreak Demo")
	FVector RoutDirection = FVector(-1.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoraleBreak Demo")
	bool bDrawDemo = true;

	/** The headline. TextRender, because HighResShot does not capture DrawDebugString. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MoraleBreak Demo")
	TObjectPtr<UTextRenderComponent> BoardText;

	/** The squad average and how many have broken. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MoraleBreak Demo")
	TObjectPtr<UTextRenderComponent> SquadText;

private:
	/** One agent, run through the real rules. */
	struct FDemoAgent
	{
		float Morale = 1.0f;
		float Nerve = 0.5f;
		EMoraleState State = EMoraleState::Steady;
		float SecondsInState = 0.0f;
		float SecondsSinceBadNews = 1000.0f;
		FVector Home = FVector::ZeroVector;
		bool bAlive = true;
	};

	void StartCycle();
	void Step(float DeltaSeconds, float T);
	FVector SlotLocation(int32 Index) const;
	FString PhaseCaption(float T) const;
	void DrawScene() const;

	TArray<FDemoAgent> Agents;

	float CycleTime = 0.0f;
	int32 LossesFired = 0;
	bool bRallied = false;
	bool bHomesCaptured = false;
};

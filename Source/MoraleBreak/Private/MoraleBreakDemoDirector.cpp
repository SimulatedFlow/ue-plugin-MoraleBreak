// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "MoraleBreakDemoDirector.h"

#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "MoraleBreakSettings.h"
#include "MoraleBreakStatics.h"

namespace MoraleBreakDemoLocal
{
	static FColor StateColour(EMoraleState State)
	{
		switch (State)
		{
		case EMoraleState::Steady:   return FColor(120, 230, 130);
		case EMoraleState::Shaken:   return FColor(235, 215, 110);
		case EMoraleState::Panicked: return FColor(240, 150, 90);
		case EMoraleState::Routed:   return FColor(240, 100, 100);
		default:                     return FColor::White;
		}
	}

	static const TCHAR* StateText(EMoraleState State)
	{
		switch (State)
		{
		case EMoraleState::Steady:   return TEXT("STEADY");
		case EMoraleState::Shaken:   return TEXT("SHAKEN");
		case EMoraleState::Panicked: return TEXT("PANICKED");
		case EMoraleState::Routed:   return TEXT("ROUTED");
		default:                     return TEXT("?");
		}
	}

	/** Five fixed nerves rather than a roll, so every run of the video is the same run. */
	constexpr float Nerves[5] = { 0.75f, 0.20f, 0.55f, 0.35f, 0.65f };
	constexpr int32 SquadSize = 5;
	constexpr float Spacing = 220.0f;
}

AMoraleBreakDemoDirector::AMoraleBreakDemoDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BoardText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BoardText"));
	BoardText->SetupAttachment(Root);
	BoardText->SetHorizontalAlignment(EHTA_Center);
	BoardText->SetVerticalAlignment(EVRTA_TextBottom);
	BoardText->SetWorldSize(38.0f);
	BoardText->SetTextRenderColor(FColor::White);

	// Yaw 270; at 90 a TextRender is mirrored.
	BoardText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	BoardText->SetRelativeLocation(FVector(0.0f, 0.0f, 790.0f));

	SquadText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SquadText"));
	SquadText->SetupAttachment(Root);
	SquadText->SetHorizontalAlignment(EHTA_Center);
	SquadText->SetVerticalAlignment(EVRTA_TextBottom);
	SquadText->SetWorldSize(48.0f);
	SquadText->SetTextRenderColor(FColor(190, 205, 220));
	SquadText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	SquadText->SetRelativeLocation(FVector(0.0f, 0.0f, 690.0f));
}

void AMoraleBreakDemoDirector::BeginPlay()
{
	Super::BeginPlay();
	StartCycle();
}

void AMoraleBreakDemoDirector::StartCycle()
{
	CycleTime = 0.0f;
	LossesFired = 0;
	bRallied = false;

	Agents.Reset();
	Agents.SetNum(MoraleBreakDemoLocal::SquadSize);
	for (int32 i = 0; i < Agents.Num(); ++i)
	{
		Agents[i].Morale = 1.0f;
		Agents[i].Nerve = MoraleBreakDemoLocal::Nerves[i];
		Agents[i].State = EMoraleState::Steady;
		Agents[i].SecondsInState = 10.0f;  // past the minimum dwell, so the first break is not swallowed
		Agents[i].SecondsSinceBadNews = 1000.0f;
		Agents[i].bAlive = true;
		Agents[i].Home = SlotLocation(i);
	}

	// Put anybody the level gave us back where they started.
	for (int32 i = 0; i < SquadActors.Num() && i < Agents.Num(); ++i)
	{
		if (AActor* Actor = SquadActors[i].Get())
		{
			if (bHomesCaptured)
			{
				Actor->SetActorLocation(Agents[i].Home);
			}
			else
			{
				Agents[i].Home = Actor->GetActorLocation();
			}
		}
	}
	bHomesCaptured = true;
}

FVector AMoraleBreakDemoDirector::SlotLocation(int32 Index) const
{
	const float Offset = (Index - (MoraleBreakDemoLocal::SquadSize - 1) * 0.5f)
		* MoraleBreakDemoLocal::Spacing;
	return GetActorLocation() + FVector(0.0f, Offset, 90.0f);
}

void AMoraleBreakDemoDirector::Step(float DeltaSeconds, float T)
{
	const UMoraleBreakSettings* Settings = UMoraleBreakSettings::Get();

	// The script. Two losses and a spell of being shot at; then the officer arrives.
	//   4 s  a man on the right goes down
	//   9 s  the leader goes down - worth several ordinary losses
	//   6-17 s  rounds keep landing
	//   20 s  reinforcements, and the line re-forms
	if (LossesFired == 0 && T > 4.0f)
	{
		LossesFired = 1;
		Agents.Last().bAlive = false;
		for (FDemoAgent& Agent : Agents)
		{
			if (Agent.bAlive)
			{
				Agent.Morale = UMoraleBreakStatics::ApplyMoraleEvent(Agent.Morale,
					Settings->WeightFor(EMoraleEvent::AllyDown), Agent.Nerve);
				Agent.SecondsSinceBadNews = 0.0f;
			}
		}
	}
	if (LossesFired == 1 && T > 9.0f)
	{
		LossesFired = 2;
		Agents[0].bAlive = false;
		for (FDemoAgent& Agent : Agents)
		{
			if (Agent.bAlive)
			{
				Agent.Morale = UMoraleBreakStatics::ApplyMoraleEvent(Agent.Morale,
					Settings->WeightFor(EMoraleEvent::LeaderDown), Agent.Nerve);
				Agent.SecondsSinceBadNews = 0.0f;
			}
		}
	}

	// Suppression, once a second between six and sixteen.
	//
	// Tuned twice. Twice a second at full weight had the whole squad at zero by second twelve - the
	// plugin working, and a row of empty bars with nothing left to read. Once every second and a half
	// at 60% went the other way: the contagion levelled everybody at 0.44, nobody ever panicked, and
	// the demo promised a rout it never delivered. One a second at full weight lands the average just
	// under the panic threshold at about second fourteen, which leaves the middle states on screen for
	// eight seconds and still breaks the line.
	if (T > 6.0f && T < 16.0f && FMath::Fmod(T, 1.0f) < DeltaSeconds)
	{
		for (FDemoAgent& Agent : Agents)
		{
			if (Agent.bAlive)
			{
				Agent.Morale = UMoraleBreakStatics::ApplyMoraleEvent(Agent.Morale,
					Settings->WeightFor(EMoraleEvent::Suppressed), Agent.Nerve);
				Agent.SecondsSinceBadNews = 0.0f;
			}
		}
	}

	if (!bRallied && T > 20.0f)
	{
		bRallied = true;
		for (FDemoAgent& Agent : Agents)
		{
			if (Agent.bAlive)
			{
				Agent.Morale = 1.0f;
				Agent.State = EMoraleState::Steady;
				Agent.SecondsInState = 0.0f;
			}
		}
	}

	// The squad average, computed once for everybody - the same arrangement the subsystem uses, and for
	// the same reason.
	float Total = 0.0f;
	int32 Living = 0;
	for (const FDemoAgent& Agent : Agents)
	{
		if (Agent.bAlive)
		{
			Total += Agent.Morale;
			++Living;
		}
	}
	const float Average = Living > 0 ? Total / Living : 1.0f;

	for (FDemoAgent& Agent : Agents)
	{
		if (!Agent.bAlive)
		{
			continue;
		}

		Agent.SecondsInState += DeltaSeconds;
		Agent.SecondsSinceBadNews += DeltaSeconds;

		if (Agent.SecondsSinceBadNews >= Settings->RecoveryDelaySeconds)
		{
			Agent.Morale = UMoraleBreakStatics::RecoverMorale(Agent.Morale,
				Settings->RecoveryPerSecond, DeltaSeconds);
		}

		// The contagion. This is what makes the five bars fall together rather than independently, and
		// it is the reason a rout reads as a rout instead of as five separate decisions.
		Agent.Morale = UMoraleBreakStatics::ContagionPull(Agent.Morale, Average,
			Settings->ContagionPerSecond, DeltaSeconds);

		if (Agent.State == EMoraleState::Routed)
		{
			continue;  // in this demo a rout lasts to the end of the cycle
		}

		if (UMoraleBreakStatics::ShouldRout(Agent.State, Agent.SecondsInState, Settings->RoutAfterSeconds))
		{
			Agent.State = EMoraleState::Routed;
			Agent.SecondsInState = 0.0f;
			continue;
		}

		const EMoraleState Next = UMoraleBreakStatics::ResolveState(Agent.Morale, Agent.State,
			Settings->ShakenBelow, Settings->PanicBelow, Settings->Hysteresis);
		if (Next != Agent.State && Agent.SecondsInState >= Settings->MinimumStateSeconds)
		{
			Agent.State = Next;
			Agent.SecondsInState = 0.0f;
		}
	}
}

void AMoraleBreakDemoDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The editor's first delta after a recompile is enormous; unclamped it plays the whole script in one
	// frame.
	DeltaSeconds = FMath::Clamp(DeltaSeconds, 0.0f, 0.1f);

	CycleTime += DeltaSeconds;
	if (CycleTime > CycleSeconds)
	{
		StartCycle();
		return;
	}
	const float T = CycleTime;

	if (Agents.Num() != MoraleBreakDemoLocal::SquadSize)
	{
		StartCycle();
	}

	Step(DeltaSeconds, T);

	// Move the routed away and the dead out of the line, so the state is legible in a still frame.
	for (int32 i = 0; i < SquadActors.Num() && i < Agents.Num(); ++i)
	{
		AActor* Actor = SquadActors[i].Get();
		if (!Actor)
		{
			continue;
		}

		FVector Where = Agents[i].Home;
		if (!Agents[i].bAlive)
		{
			Where.Z -= 60.0f;  // down
		}
		else if (Agents[i].State == EMoraleState::Routed)
		{
			const FVector Away = RoutDirection.GetSafeNormal();
			Where += Away * RoutDistance * FMath::Min(Agents[i].SecondsInState / 3.0f, 1.0f);
		}
		Actor->SetActorLocation(Where);
	}

	float Total = 0.0f;
	int32 Living = 0;
	int32 Broken = 0;
	for (const FDemoAgent& Agent : Agents)
	{
		if (!Agent.bAlive)
		{
			continue;
		}
		Total += Agent.Morale;
		++Living;
		if (Agent.State == EMoraleState::Panicked || Agent.State == EMoraleState::Routed)
		{
			++Broken;
		}
	}

	if (BoardText)
	{
		BoardText->SetText(FText::FromString(PhaseCaption(T)));
	}
	if (SquadText)
	{
		SquadText->SetText(FText::FromString(FString::Printf(
			TEXT("SQUAD %.2f   -   %d of %d broken"),
			Living > 0 ? Total / Living : 1.0f, Broken, Living)));
		SquadText->SetTextRenderColor(Broken > 0 ? FColor(240, 150, 90) : FColor(190, 205, 220));
	}

	if (bDrawDemo)
	{
		DrawScene();
	}
}

FString AMoraleBreakDemoDirector::PhaseCaption(float T) const
{
	if (T < 4.0f)  return TEXT("five men, five different nerves - rolled once, never again");
	if (T < 9.0f)  return TEXT("one down: everybody loses morale, the nervous one loses most");
	if (T < 13.0f) return TEXT("the leader is down - worth several ordinary losses");
	if (T < 17.0f) return TEXT("contagion: the bars fall together, which is why a rout looks like one");
	if (T < 20.0f) return TEXT("panic held for long enough - he leaves, and does not come straight back");
	return TEXT("reinforcements: the line re-forms");
}

void AMoraleBreakDemoDirector::DrawScene() const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UWorld* Mutable = const_cast<UWorld*>(World);

	const UMoraleBreakSettings* Settings = UMoraleBreakSettings::Get();
		// 0.35 s, not one frame. A debug shape lives on the line batcher and is dropped when its time is
	// up - and during a long screenshot run the editor renders many frames between two world ticks,
	// so anything drawn for a twelfth of a second is gone by the time the picture is taken. Measured
	// on 11.09.2026: a whole 240-frame capture came out with no lane, no spheres and no bars.
	constexpr float Life = 0.35f;
	constexpr float BarHeight = 340.0f;

	for (int32 i = 0; i < Agents.Num(); ++i)
	{
		const FDemoAgent& Agent = Agents[i];

		FVector Base = Agent.Home;
		if (i < SquadActors.Num())
		{
			if (const AActor* Actor = SquadActors[i].Get())
			{
				Base = Actor->GetActorLocation();
			}
		}

		if (!Agent.bAlive)
		{
			DrawDebugSphere(Mutable, Base, 40.0f, 8, FColor(90, 90, 95), false, Life, 0, 1.0f);
			continue;
		}

		const FColor Colour = MoraleBreakDemoLocal::StateColour(Agent.State);
		DrawDebugSphere(Mutable, Base, 45.0f, 12, Colour, false, Life, 0, 3.0f);

		// A morale bar per man, drawn upwards. Five bars falling at slightly different rates is the
		// clearest possible picture of "nerve resists, contagion pulls".
		//
		// Built from six parallel lines rather than one. A single debug line is a hairline at the camera
		// distance a store screenshot is taken from, and a bar nobody can see is not a demo of anything.
		const FVector Bottom = Base + FVector(0.0f, 0.0f, 130.0f);
		for (int32 Ply = 0; Ply < 6; ++Ply)
		{
			const FVector Side((Ply - 2.5f) * 20.0f, 0.0f, 0.0f);
			DrawDebugLine(Mutable, Bottom + Side, Bottom + Side + FVector(0.0f, 0.0f, BarHeight),
				FColor(52, 54, 62), false, Life, 0, 7.0f);
			if (Agent.Morale > 0.004f)
			{
				DrawDebugLine(Mutable, Bottom + Side,
					Bottom + Side + FVector(0.0f, 0.0f, BarHeight * Agent.Morale),
					Colour, false, Life, 0, 11.0f);
			}
		}

		// The two thresholds, so the hysteresis is not an assertion in the manual.
		const FColor Line(190, 190, 205);
		DrawDebugLine(Mutable,
			Bottom + FVector(-95.0f, 0.0f, BarHeight * Settings->ShakenBelow),
			Bottom + FVector(95.0f, 0.0f, BarHeight * Settings->ShakenBelow), Line, false, Life, 0, 4.0f);
		DrawDebugLine(Mutable,
			Bottom + FVector(-95.0f, 0.0f, BarHeight * Settings->PanicBelow),
			Bottom + FVector(95.0f, 0.0f, BarHeight * Settings->PanicBelow),
			FColor(230, 120, 120), false, Life, 0, 4.0f);

		// And the band they have to climb back through - the reason nothing on screen flickers.
		DrawDebugLine(Mutable,
			Bottom + FVector(-60.0f, 0.0f, BarHeight * (Settings->PanicBelow + Settings->Hysteresis)),
			Bottom + FVector(60.0f, 0.0f, BarHeight * (Settings->PanicBelow + Settings->Hysteresis)),
			FColor(130, 190, 230), false, Life, 0, 3.0f);
	}
#endif
}

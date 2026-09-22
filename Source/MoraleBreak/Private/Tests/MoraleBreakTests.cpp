// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "MoraleBreakStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace MoraleBreakTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::CommandletContext
		| EAutomationTestFlags::EngineFilter;

	// The shipped defaults, so the tests are about the game as configured and not about a nicer
	// hypothetical configuration.
	constexpr float ShakenBelow = 0.60f;
	constexpr float PanicBelow = 0.25f;
	constexpr float Band = 0.15f;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoraleBreakHysteresisBothWays,
	"MoraleBreak.State.HysteresisWorksInBothDirections", MoraleBreakTests::TestFlags)

bool FMoraleBreakHysteresisBothWays::RunTest(const FString&)
{
	using namespace MoraleBreakTests;

	// Falling. The plain thresholds apply, because hesitating on the way down would mean an agent keeps
	// fighting normally while its squad is being wiped out.
	TestEqual(TEXT("0.55 from steady is shaken"),
		UMoraleBreakStatics::ResolveState(0.55f, EMoraleState::Steady, ShakenBelow, PanicBelow, Band),
		EMoraleState::Shaken);
	TestEqual(TEXT("0.20 from shaken is panicked"),
		UMoraleBreakStatics::ResolveState(0.20f, EMoraleState::Shaken, ShakenBelow, PanicBelow, Band),
		EMoraleState::Panicked);

	// Rising. THE WHOLE POINT. An agent parked just above the panic threshold must stay panicked, or it
	// changes its mind every frame - and every change of mind fires a delegate, a bark and an animation.
	TestEqual(TEXT("0.30 does not lift a panicked agent out"),
		UMoraleBreakStatics::ResolveState(0.30f, EMoraleState::Panicked, ShakenBelow, PanicBelow, Band),
		EMoraleState::Panicked);
	TestEqual(TEXT("0.45 finally does"),
		UMoraleBreakStatics::ResolveState(0.45f, EMoraleState::Panicked, ShakenBelow, PanicBelow, Band),
		EMoraleState::Shaken);

	TestEqual(TEXT("0.65 does not lift a shaken agent to steady"),
		UMoraleBreakStatics::ResolveState(0.65f, EMoraleState::Shaken, ShakenBelow, PanicBelow, Band),
		EMoraleState::Shaken);
	TestEqual(TEXT("0.80 does"),
		UMoraleBreakStatics::ResolveState(0.80f, EMoraleState::Shaken, ShakenBelow, PanicBelow, Band),
		EMoraleState::Steady);

	// A big recovery still walks up through the middle state rather than teleporting to Steady, so the
	// animation and the dialogue have somewhere to happen.
	TestEqual(TEXT("a panicked agent handed 0.70 becomes shaken, not steady"),
		UMoraleBreakStatics::ResolveState(0.70f, EMoraleState::Panicked, ShakenBelow, PanicBelow, Band),
		EMoraleState::Shaken);

	// Routed is not a matter of arithmetic. Letting this function talk an agent out of a rout would let
	// it rally in the same frame it broke.
	TestEqual(TEXT("routed stays routed whatever the number says"),
		UMoraleBreakStatics::ResolveState(1.0f, EMoraleState::Routed, ShakenBelow, PanicBelow, Band),
		EMoraleState::Routed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoraleBreakContagionIsPerSecond,
	"MoraleBreak.Contagion.TwoHalfStepsEqualOneWholeStep", MoraleBreakTests::TestFlags)

bool FMoraleBreakContagionIsPerSecond::RunTest(const FString&)
{
	constexpr float Strength = 0.8f;

	const float Whole = UMoraleBreakStatics::ContagionPull(1.0f, 0.0f, Strength, 1.0f);

	float Halves = 1.0f;
	Halves = UMoraleBreakStatics::ContagionPull(Halves, 0.0f, Strength, 0.5f);
	Halves = UMoraleBreakStatics::ContagionPull(Halves, 0.0f, Strength, 0.5f);

	// THE FRAME-RATE TEST. The version everyone writes first - Own += (Avg - Own) * Strength * Dt - fails
	// this, and the bug ships as "panic spreads faster on my machine than on the build server".
	TestNearlyEqual(TEXT("one second in one step equals one second in two"), Halves, Whole, 0.0005f);

	// Ten small steps as well, because that is what a real frame looks like.
	float Tenths = 1.0f;
	for (int32 i = 0; i < 10; ++i)
	{
		Tenths = UMoraleBreakStatics::ContagionPull(Tenths, 0.0f, Strength, 0.1f);
	}
	TestNearlyEqual(TEXT("and the same in ten"), Tenths, Whole, 0.0005f);

	TestNearlyEqual(TEXT("no contagion leaves an agent alone"),
		UMoraleBreakStatics::ContagionPull(0.9f, 0.1f, 0.0f, 1.0f), 0.9f, 0.0001f);

	// It pulls towards the squad, never past it - a fast machine must not overshoot into despair.
	const float Pulled = UMoraleBreakStatics::ContagionPull(1.0f, 0.4f, 50.0f, 1.0f);
	TestTrue(TEXT("contagion never overshoots the average"), Pulled >= 0.4f - 0.0001f);

	// Recovery is per second too, for the same reason.
	TestNearlyEqual(TEXT("recovery is per second"),
		UMoraleBreakStatics::RecoverMorale(0.5f, 0.1f, 2.0f), 0.7f, 0.0001f);
	TestNearlyEqual(TEXT("and it cannot exceed one"),
		UMoraleBreakStatics::RecoverMorale(0.95f, 1.0f, 2.0f), 1.0f, 0.0001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoraleBreakNerveResistsButNeverInverts,
	"MoraleBreak.Nerve.ResistsButNeverInverts", MoraleBreakTests::TestFlags)

bool FMoraleBreakNerveResistsButNeverInverts::RunTest(const FString&)
{
	const float Nervous = UMoraleBreakStatics::ApplyMoraleEvent(1.0f, -0.2f, 0.0f);
	const float Steady = UMoraleBreakStatics::ApplyMoraleEvent(1.0f, -0.2f, 0.8f);

	TestNearlyEqual(TEXT("no nerve at all takes the full weight"), Nervous, 0.8f, 0.0001f);
	TestTrue(TEXT("a steady man takes less of it"), Steady > Nervous);

	// Resisting is not being cheered up. A nerve value outside the sane range - which is what happens the
	// first time somebody stores nerve in 0..100 - must not turn a death into good news.
	TestTrue(TEXT("bad news is never good news, whatever nerve says"),
		UMoraleBreakStatics::ApplyMoraleEvent(1.0f, -0.2f, 5.0f) <= 1.0f);
	TestTrue(TEXT("and it never rises above the start on a negative event"),
		UMoraleBreakStatics::ApplyMoraleEvent(0.5f, -0.2f, 5.0f) <= 0.5f);

	TestNearlyEqual(TEXT("morale cannot go below zero"),
		UMoraleBreakStatics::ApplyMoraleEvent(0.05f, -0.9f, 0.0f), 0.0f, 0.0001f);
	TestNearlyEqual(TEXT("or above one"),
		UMoraleBreakStatics::ApplyMoraleEvent(0.95f, 0.9f, 0.0f), 1.0f, 0.0001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoraleBreakOddsSurviveZero,
	"MoraleBreak.Odds.DefinedWhenASideIsEmpty", MoraleBreakTests::TestFlags)

bool FMoraleBreakOddsSurviveZero::RunTest(const FString&)
{
	TestNearlyEqual(TEXT("even odds change nothing"),
		UMoraleBreakStatics::OutnumberedFactor(4, 4), 1.0f, 0.0001f);

	TestTrue(TEXT("outnumbered is worse"), UMoraleBreakStatics::OutnumberedFactor(1, 4) > 1.0f);
	TestTrue(TEXT("having the upper hand is better"), UMoraleBreakStatics::OutnumberedFactor(4, 1) < 1.0f);

	// The moment the last enemy dies is exactly when this gets called with a zero. A ratio with a zero in
	// it is a division by zero or an infinity, and an infinity that reaches morale stays there for the
	// rest of the match.
	const float NoEnemies = UMoraleBreakStatics::OutnumberedFactor(4, 0);
	TestTrue(TEXT("no enemies left is finite"), FMath::IsFinite(NoEnemies));
	TestTrue(TEXT("and reads as favourable"), NoEnemies < 1.0f);

	const float NoFriends = UMoraleBreakStatics::OutnumberedFactor(0, 4);
	TestTrue(TEXT("alone against four is finite"), FMath::IsFinite(NoFriends));
	TestTrue(TEXT("and reads as bad"), NoFriends > 1.0f);

	TestTrue(TEXT("both sides empty is still a number"),
		FMath::IsFinite(UMoraleBreakStatics::OutnumberedFactor(0, 0)));

	// Gentle on purpose: twenty to one is frightening, not twenty times as frightening, and the linear
	// version makes a horde game unplayable the moment the horde arrives.
	TestTrue(TEXT("overwhelming odds stay bounded"),
		UMoraleBreakStatics::OutnumberedFactor(1, 400) <= 4.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoraleBreakRoutIsAnEventNotAWobble,
	"MoraleBreak.Rout.MinimumDurationHolds", MoraleBreakTests::TestFlags)

bool FMoraleBreakRoutIsAnEventNotAWobble::RunTest(const FString&)
{
	using namespace MoraleBreakTests;

	TestFalse(TEXT("a moment of panic is not a rout"),
		UMoraleBreakStatics::ShouldRout(EMoraleState::Panicked, 1.0f, 3.5f));
	TestTrue(TEXT("sustained panic is"),
		UMoraleBreakStatics::ShouldRout(EMoraleState::Panicked, 4.0f, 3.5f));
	TestFalse(TEXT("a shaken agent never routs"),
		UMoraleBreakStatics::ShouldRout(EMoraleState::Shaken, 60.0f, 3.5f));

	// Zero means the project does not want routing, not that it wants it instantly - instant routing is
	// indistinguishable from deleting the agent.
	TestFalse(TEXT("zero switches routing off"),
		UMoraleBreakStatics::ShouldRout(EMoraleState::Panicked, 60.0f, 0.0f));

	// Coming back needs BOTH the time and the recovery. Full morale one second into a rout must not end
	// it, or the rout is a twitch the player never sees.
	TestFalse(TEXT("full morale too soon does not end a rout"),
		UMoraleBreakStatics::CanReturnFromRout(1.0f, 1.0f, 6.0f, PanicBelow, Band));
	TestFalse(TEXT("enough time but no recovery does not either"),
		UMoraleBreakStatics::CanReturnFromRout(0.30f, 10.0f, 6.0f, PanicBelow, Band));
	TestTrue(TEXT("both together do"),
		UMoraleBreakStatics::CanReturnFromRout(0.45f, 10.0f, 6.0f, PanicBelow, Band));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoraleBreakNewsTravelsWithDistance,
	"MoraleBreak.Falloff.NewsGetsWeakerFurtherAway", MoraleBreakTests::TestFlags)

bool FMoraleBreakNewsTravelsWithDistance::RunTest(const FString&)
{
	TestNearlyEqual(TEXT("at your elbow, full weight"),
		UMoraleBreakStatics::DistanceFalloff(100.0f, 600.0f, 3000.0f), 1.0f, 0.0001f);
	TestNearlyEqual(TEXT("at the inner radius, still full"),
		UMoraleBreakStatics::DistanceFalloff(600.0f, 600.0f, 3000.0f), 1.0f, 0.0001f);
	TestNearlyEqual(TEXT("halfway between, half"),
		UMoraleBreakStatics::DistanceFalloff(1800.0f, 600.0f, 3000.0f), 0.5f, 0.0001f);
	TestNearlyEqual(TEXT("beyond the outer radius, nothing"),
		UMoraleBreakStatics::DistanceFalloff(4000.0f, 600.0f, 3000.0f), 0.0f, 0.0001f);

	// Radii the wrong way round is a configuration mistake, not a licence to divide by a negative.
	TestTrue(TEXT("inverted radii stay in range"),
		UMoraleBreakStatics::DistanceFalloff(1000.0f, 3000.0f, 600.0f) >= 0.0f);

	return true;
}

#endif  // WITH_DEV_AUTOMATION_TESTS

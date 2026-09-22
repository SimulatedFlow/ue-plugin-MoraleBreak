# MoraleBreak — Fab Store Listing

## Headline

**Enemies that break, run, and come back — without a single frame of flicker.**

## Pitch

Enemies that fight to the last man read as machinery. Everybody knows it, and everybody's first fix
is the same: a float, a threshold, a bool. That fix reads as a bug, because an agent hovering at the
threshold flips between brave and terrified every frame — and every flip fires a delegate, a bark and
an animation. The symptom is never reported as "morale is wrong". It is reported as stuttering.

**MoraleBreak** is that float done properly. Four states rather than two, because a **Shaken** agent
still fights — from cover, at longer range, with worse aim — and that is where most of the character
lives. A **hysteresis band** so the level at which somebody breaks is not the level at which they
rally. A **minimum dwell time** on top of it, because a burst of four events in one frame can jump
clean across the band.

Every agent gets a private **nerve**, rolled once and never again, so the same bad news moves a
steady man less than a nervous one. Without it a squad breaks in unison like a single organism; with
it, one man runs first and the others watch him do it.

And panic is **contagious**: each agent is pulled towards its squad's average, which is what turns
"three enemies happened to flee" into "the line broke". The pull is an exponential approach, so one
second is one second whether your frame took 8 ms or 33 — there is an automation test that proves two
half-steps land exactly where one whole step does.

Events carry a **reason** all the way to the state change, so the right line of dialogue can play.
Weights live in a table in Project Settings, so tuning how brittle an army is does not need a
programmer.

## Feature bullets

- **Four states**: Steady, Shaken, Panicked, Routed — the middle two are where the character is.
- **Hysteresis band and minimum dwell time.** Two independent flicker guards, because one is not
  enough.
- **Per-agent nerve**, rolled at spawn, so a squad does not break in unison.
- **Contagion** as a pull towards the squad average. This is what makes a rout look like a rout.
- **Frame-rate independent by construction.** Decay, recovery and contagion are per second, and the
  tests assert it.
- **Reasons travel.** `OnStateChanged(Old, New, Reason)` — bind the bark straight to it.
- **A weight table in Project Settings.** Deleting a row switches an event off.
- **Odds that do not explode.** `OutnumberedFactor` is a square root and is defined when a side is
  empty — which is exactly when it gets called.
- **Distance falloff** built in: one call at the spot, and everybody close enough takes it personally.
- **No behaviour tree, no GAS, no navigation dependency.** Works with StateTree, behaviour trees or
  hand-written controllers, unchanged.
- **A demo level that runs without pressing play.**
- **Full C++ source. No third-party code. Blueprint-first.**

## Technical specs

Unreal Engine 5.8 · Win64 · one runtime module · no AIModule / GAS dependency · Blueprint-callable
throughout · 6 automation tests.

## Target audience

Squad shooters, tactics games, medieval and historical battles, horde games, stealth games, RTS —
anything where an enemy giving up is more interesting than an enemy dying.

## Suggested price

**17,18 €**

## Suggested tags

AI, Morale, Squad, Combat, Blueprint, C++

# ==================== TECHNICAL DETAILS (Fab form) ====================

**Features:**
- Four-state morale machine with a hysteresis band and a minimum dwell time
- Per-agent nerve rolled once at spawn, with a per-component override
- Squad contagion as an exponential approach to the group average (frame-rate independent)
- Rout with a configurable onset time and a minimum duration before recovery
- Event weight table in Project Settings; a missing row is a disabled event
- Distance falloff, outnumbered factor, recovery delay
- Console commands MoraleBreak.Debug, MoraleBreak.Dump, MoraleBreak.Rally
- Demo level that ticks in the editor viewport without PIE

**Code Modules:** MoraleBreak (Runtime, PreDefault)
**Number of Blueprints:** 0 (code plugin; the demo level contains no Blueprint logic)
**Number of C++ Classes:** 5 (actor component, tickable world subsystem, Blueprint function library,
developer settings, demo director) plus the module
**Network Replicated:** No (server-side state; replicate the state, not the float)
**Supported Development Platforms:** Windows
**Supported Target Build Platforms:** Windows
**Supported Engine Versions:** 5.8
**Third-party software:** none
**Documentation:** https://wiki.teufel-engineering.com/en/MoraleBreak/documentation
**Support:** teufelsilvan@gmail.com

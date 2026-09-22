# MoraleBreak — Squad Morale, Panic & Rout

**Unreal Engine 5.8 · Win64 · one runtime C++ module · Blueprint-first · full source**

Enemies that fight to the last man read as machinery. The usual fix — a float, a threshold and a
bool — reads as a bug, because an agent hovering at the line flips between brave and terrified every
frame.

MoraleBreak gives every agent a morale value and a private **nerve** that decides how much it takes
to shake them, spreads panic through a squad as a pull towards the group's average so a rout looks
like one, and separates the level at which somebody breaks from the level at which they rally so
nothing can flicker.

Events carry a **reason** all the way to the state change, which is what lets the right line of
dialogue play.

Frame-rate independent by construction: the decay and the contagion are per second, and the tests
prove two half-steps equal one whole one.

## Install in five minutes

1. Add a **Morale** component to every agent. Give a squad the same **Squad Id**.
2. Bind **On State Changed** and do whatever Shaken, Panicked and Routed mean in your game.
3. Call **Report Event At Location (AllyDown, …)** when somebody dies.

## What is in the box

* `UMoraleComponent` — morale, nerve, the state machine, the delegates
* `UMoraleBreakSubsystem` — squads, averages, one clock for everybody
* `UMoraleBreakStatics` — the rules as pure functions
* `UMoraleBreakSettings` — thresholds, hysteresis, nerve range, and the event weight table
* A demo level that runs **without pressing play**
* Six automation tests over the rules that fail quietly

## Documentation

https://wiki.teufel-engineering.com/en/MoraleBreak/documentation

## Support

teufelsilvan@gmail.com

Copyright 2026 Silvan Teufel. All Rights Reserved.

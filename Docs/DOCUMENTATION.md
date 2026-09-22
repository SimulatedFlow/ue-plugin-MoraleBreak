# MoraleBreak — Squad Morale, Panic & Rout

Enemies that fight to the last man read as machinery. The usual fix — a float, a threshold and a
bool — reads as a bug, because an agent sitting on the line flips between brave and terrified every
frame, and every flip fires a delegate, a bark and an animation.

This plugin is the float done properly.

---

## 0. Supported engine and platforms

* Unreal Engine **5.8**
* **Win64**. One runtime C++ module, no third-party code, full source included.
* No dependency on AIModule or GameplayAbilities. Morale decides how an agent **feels**; what that
  makes it do is your project's business.

---

## 1. The five-minute install

1. Add a **Morale** component to every agent that can lose its nerve.
2. Give agents that fight together the same **Squad Id**.
3. Bind **On State Changed**.

```
On State Changed (Old State, New State, Reason)
    New State == Shaken    ->  move to cover, longer engagement range
    New State == Panicked  ->  stop advancing, fire wildly, play the right bark
    New State == Routed    ->  run to the rally point
```

Then tell it what is happening:

```
On my death        ->  Report Event At Location (AllyDown, my location, 0)
On my leader dying ->  Report Event At Location (LeaderDown, my location, 0)
On taking damage   ->  Report Event (TookDamage)
```

`Report Event At Location` is the one you will use most: one call at the spot, and everybody close
enough to notice takes it personally in proportion to how close they were.

---

## 2. Morale, nerve, and why both

**Morale** is 0 to 1 and moves. **Nerve** is 0 to 1 and does not: it is rolled once at spawn from
`NerveMin`..`NerveMax` and then left alone.

Nerve **resists**. The same bad news moves a nerve-0.8 man by a fifth of what it moves a nerve-0.0
man. It never reverses: a steady soldier shrugs off a death, he is not cheered up by one.

Without nerve a squad breaks in unison, like a single organism. With it, one man runs first and the
others watch him do it — which is what a rout actually looks like.

`NerveOverride` on the component pins it, for the named character who is supposed to be the brave
one. `bAffectedByContagion = false` pins him further: he holds while everyone around him runs.

---

## 3. The four states, and the two things that stop them flickering

`Steady → Shaken → Panicked → Routed`

Four, not two, because the interesting behaviour is in the middle. A **Shaken** agent still fights —
from cover, at longer range, with worse aim. That is where most of the character lives.

### Hysteresis

Falling, the thresholds are exactly `ShakenBelow` (0.60) and `PanicBelow` (0.25).

Rising, **both are lifted by `Hysteresis`** (0.15). An agent has to reach 0.40 before he climbs out
of panic and 0.75 before he is steady again.

This is the single most important number in the plugin. At zero, an agent parked on a threshold
changes state every frame. The bug never gets reported as "morale is wrong"; it gets reported as
"the animations stutter".

### Minimum dwell time

`MinimumStateSeconds` (0.75) blocks any transition that arrives too soon after the last one. It is
worth having *on top of* the hysteresis band, because a burst of four events in a single frame can
jump clean across the band, and an agent that changed state 30 ms ago has not finished the animation
that says so.

### Routing

`ShouldRout` fires after `RoutAfterSeconds` (3.5) of unbroken panic. Coming back needs **both**
`MinRoutSeconds` (6.0) to have passed **and** morale to have recovered past the hysteresis band. Full
morale one second into a rout does not end it — otherwise the rout is a twitch the player never sees.

`RoutAfterSeconds = 0` switches routing off entirely. It does **not** mean "rout instantly"; instant
routing is indistinguishable from deleting the agent.

---

## 4. Contagion — why a rout looks like a rout

Every step, each agent is pulled towards its squad's average morale by `ContagionPerSecond` (0.5,
about 40% of the gap closed per second).

This is what makes five morale bars fall **together** rather than independently. One man breaking
drags the average down, which drags everybody down, which breaks the next one. That feedback loop is
the difference between "three enemies happened to flee" and "the line broke".

The pull is an **exponential approach**, so the result depends on how much *time* passed and not on
how many times it was called: two steps of half a second land exactly where one step of a whole
second does. The version everyone writes first —

```
Own += (Average - Own) * Strength * DeltaTime
```

— fails that, and the bug ships as *"panic spreads faster on my machine than on the build server"*.
There is an automation test that asserts one second in one step equals one second in ten.

Set `ContagionPerSecond` to 0 for a squad of strangers who happen to stand near each other. Very high
values make the squad a single organism: correct for a hive, wrong for people.

`SquadUpdateInterval` (0.25 s) is how often the whole thing steps. Morale is not a per-frame quantity,
and because every rule is expressed *per second* the result is the same either way.

---

## 5. Events and weights

Weights live in **Project Settings → Plugins → MoraleBreak** as a table, so tuning how brittle an army
is does not need a programmer.

| Event | Default | |
|---|---|---|
| `AllyDown` | −0.18 | Somebody on our side went down nearby. |
| `LeaderDown` | −0.45 | Worth several ordinary losses. |
| `TookDamage` | −0.10 | |
| `Suppressed` | −0.07 | Rounds landing near us, hit or not. |
| `Outnumbered` | −0.05 | Scale it with `OutnumberedFactor`. |
| `LowHealth` | −0.15 | |
| `EnemyDown` | +0.08 | |
| `ReinforcementsArrived` | +0.25 | |
| `LeaderNearby` | +0.04 | Quietly the strongest steadying influence there is. |
| `Rest` | +0.03 | |

An event **missing from the table is worth zero**. Deleting a row is a supported way to switch an
event off.

The relative sizes are the part worth keeping: nothing good is as strong as something bad, which is
why a fight that turns takes a while to turn back.

`Scale` on `ReportEvent` multiplies the weight — used for distance falloff, for a hit that was worse
than usual, for the odds. One event type, many intensities.

### Odds

`OutnumberedFactor(Friends, Foes)` returns 1.0 at even odds, above 1 when outnumbered, below 1 when
you have the upper hand. Use it as the `Scale` on an `Outnumbered` event.

It is a **square root**, not a ratio: three-to-one odds are frightening, they are not three times as
frightening, and the linear version makes a horde game unplayable the moment the horde arrives. Both
sides are floored at one, because the moment the last enemy dies is exactly when this gets called
with a zero, and a ratio with a zero in it is a division by zero or an infinity that reaches morale
and stays there for the rest of the match.

### Distance

`ReportEventAtLocation` weighs each agent by `DistanceFalloff`: full weight inside `FullEffectRadius`
(600 cm), fading to nothing at `NoEffectRadius` (3000 cm). A death two rooms away is news; a death at
your elbow is not the same news.

---

## 6. Recovery

Morale returns at `RecoveryPerSecond` (0.06) — but not immediately. `RecoveryDelaySeconds` (4.0) has
to pass since the last piece of bad news first, or an agent recovers between two bursts of the same
magazine and nothing ever accumulates.

Only bad news resets the delay. Good news that also postponed recovery would mean a squad that keeps
winning never calms down, which is backwards.

---

## 7. What MoraleBreak is not

* **It does not move anybody.** `Routed` is a state and a delegate. Where the agent runs, and how,
  is your navigation.
* **It is not a behaviour tree.** No task nodes, no decorators, no blackboard keys — on purpose, so
  that projects on StateTree, on GAS or on hand-written controllers all work with it unchanged.
* **It is not replicated.** Morale is server-side truth in every project that needs it; replicate the
  *state*, not the float, and you will send a byte instead of four.
* **It does not know what a leader is.** `bIsLeader` is a flag you set and a weight you report. The
  plugin never inspects your squad structure.

---

## 8. Console commands

| Command | What it does |
|---|---|
| `MoraleBreak.Debug [0\|1]` | Draw each agent's state and morale over its head. |
| `MoraleBreak.Dump` | Every squad's average and lowest morale, and how many have broken. |
| `MoraleBreak.Rally [SquadId]` | Straight back to steady. |

---

## 9. API reference

### `UMoraleComponent`

`ReportEvent(Event, Scale)`, `ReportCustomEvent(Weight)`, `GetSnapshot()`, `GetState()`,
`GetMorale()`, `GetNerve()`, `IsFighting()`, `SetMorale()`, `SetNerve()`, `Rally()`, `ForceRout()`,
`SetSquadId()`.
Delegates: `OnStateChanged(Old, New, Reason)`, `OnMoraleChanged(NewMorale, Reason)`.

`Rally()` is an override and deliberately ignores the dwell time and the rout minimum: it is the
scripted moment where the officer arrives, and a scripted moment that silently does not happen is
worse than no feature at all.

### `UMoraleBreakSubsystem`

`ReportEventAtLocation(Event, Location, Radius, SquadFilter, Scale)`, `RallySquad(SquadId)`,
`GetSquadStats(SquadId)`, `GetSquadIds()`.

The subsystem drives every component from **one** clock and computes each squad's average **once**.
A hundred components each asking "what is my squad's average" is a hundred passes over the same list,
and it is quadratic in squad size at exactly the moment a squad is most interesting.

### `UMoraleBreakStatics` — the rules, on their own

`ApplyMoraleEvent`, `RecoverMorale`, `ContagionPull`, `ResolveState`, `ShouldRout`,
`CanReturnFromRout`, `OutnumberedFactor`, `DistanceFalloff`.

Pure: no world, no actors, no state. The component calls exactly these and so do the automation
tests, which is the only arrangement in which a green test suite is evidence about the game rather
than about a second implementation living in the test file.

---

## 10. The demo level

`Content/MoraleBreak/Maps/L_MoraleBreakDemo` runs **without pressing play**. Five men, five nerves,
two losses and a rally. The three marks across each bar are `ShakenBelow`, `PanicBelow` and the
hysteresis band they have to climb back through.

---

## 11. Troubleshooting

**States flicker.** `Hysteresis` or `MinimumStateSeconds` is at zero. Put them back.

**The whole squad breaks at the same instant.** `NerveMin` and `NerveMax` are too close together, or
`ContagionPerSecond` is very high. Widen the nerve range first.

**Nobody ever breaks.** Add up your event weights against your fight length. `MoraleBreak.Dump` shows
the averages; if they sit at 0.9 the events are too small or they are not being reported at all.

**Everybody is at zero within ten seconds.** The opposite problem. `Suppressed` fired every frame is
the usual cause — report it on a timer, not on every hit registration.

**Panic spreads at a different speed on different machines.** It cannot, with this plugin. If you are
seeing it, something in your project is applying morale per tick instead of per second.

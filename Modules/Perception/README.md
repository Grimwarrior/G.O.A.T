# GOAT_Perception

Senses for GOAT agents. A profile is a data asset describing what an agent can see and hear and how
quickly it notices; the module senses for every agent that has one and publishes the result to the
agent's blackboard, where guards already watch it.

Depends on GOAT alone.

## What exists

| Piece | State |
|---|---|
| `.prx` asset (`PerceptionProfileAsset`), made in the Asset Editor under GOAT > Perception, with `Validate()` | Built |
| **GOAT Perception** component: gives an agent senses | Built |
| **GOAT Perceivable** component: makes an entity something to notice | Built |
| Sight (cone, then a physics raycast), proximity, hearing, the awareness meter, memory | Built |
| Calling allies for help, damage awareness | Built, through the bus |
| `perc_*` blackboard variables | Built |
| Darkness (`Darkness Scale`) | Not yet, the field is stored but nothing reads it |

Nothing here has been run in a level yet.

## Setting up

1. Enable `GOAT_Perception` in the project.
2. Make a profile: **File > New > GOAT > Perception** in the Asset Editor, saved as a `.prx`.
3. On the agent entity (next to **GOAT Agent**) add **GOAT Perception** and pick the profile. With
   none picked the built-in defaults are used.
4. On whatever agents should notice (the player), add **GOAT Perceivable** with a tag such as `player`.
   A profile with no target tags notices every perceivable; with tags it notices only those.

## What it publishes

Agent-scoped, declared by the module, so no `.bbx` has to mention them.

| Variable | Type | Meaning |
|---|---|---|
| `perc_state` | Int | 0 Unaware, 1 Suspicious, 2 Searching, 3 Engaged |
| `perc_target` | EntityId | What the agent is aware of. Set only once it is at least Suspicious |
| `perc_target_visible` | Bool | True while the target is in sight right now |
| `perc_last_known` | Vector3 | Where the target was last sensed |
| `perc_heard` | Bool | True from a sound until the profile's sound memory runs out |
| `perc_noise_pos` | Vector3 | Where the last heard sound came from |
| `perc_awareness` | Float | The meter, 0 to Engaged At |

Writes are throttled: `perc_last_known` moves in half-metre steps, `perc_awareness` in tenths of the
ceiling, and everything else only when it changes, because every write to a watched variable wakes
the agents guarding on it.

```lua
selector {
    sequence {
        condition "perc_target_visible" { abort = "lower_priority" },
        delegate "Combat" { goal = "EngageTarget" },
    },
    sequence {
        condition "perc_heard" { abort = "lower_priority" },
        move_to "perc_noise_pos",
    },
    script "Patrol",
}
```

## From code

`GOAT_PerceptionRequestBus` (see `GOAT_PerceptionBus.h`):

- `EmitNoise(position, loudness, source, tag)`: a sound. Loudness one is heard out to the profile's hearing range.
- `ReportDamage(victim, attacker)`: raises the victim's awareness and names the attacker as its target.
- `GetSnapshot(entity)`: the sensor's state, target and awareness for code that is not a tree.

## How awareness works

A visible target fills the meter at **Sight Fill Per Second**, scaled from full point blank down to a
quarter at the edge of sight range. A sound adds **Hearing Fill**, scaled by loudness and falling to
half at the edge of hearing range. Something inside **Proximity Range** counts as a point-blank
sighting regardless of facing, walls or light. A visible target inside **Engage Distance** skips
straight to Engaged.

The meter drains with nothing sensed, but a state is held at its own starting value until its
forget time is up (Suspicious Forget, Searching Forget, and Sight Memory for Engaged), then steps
down one state and starts the next timer. Awareness is only reported to trees once the agent is at
least Suspicious.

That staircase is the default. With **Continuous Drain** on, the forget time is only a hold: after
the last stimulus the meter waits for the forget time of the state it reached, then falls at **Drain
Per Second** with no stops, and the state follows the value down (Engaged below 100, Searching below
35, Suspicious below 5). With the default profile an engaged agent that loses you holds 24 seconds,
then takes about 10 seconds to reach zero.

## Layout

- `Code/Include/GOAT_Perception`: the asset, the bus and the shared types, public so other gems can use them.
- `Code/Source/Sensing`: the sensing rules (`SightGeometry`, `AwarenessMeter`, `HearingModel`, `AgentSenses`), which touch the world only through `IPerceptionWorld`, plus `PerceptionSystem`, the publisher and the physics world.
- `Code/Source/Components`: the two components.
- `Code/Source/Assets`, `Code/Source/Tools`: the asset's reflection and handler, the builder component, the editor icon.
- `Code/Tests`: `GOAT_Perception.Tests`, which covers everything except the physics world, the system and the components.

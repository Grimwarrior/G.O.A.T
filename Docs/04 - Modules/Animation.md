---
type: module
status: implemented
tags: [module, animation]
---

# Animation (GOAT_Animation)

> **Status:** Implemented. The signal hook was added 2026-10-03 and has not been run in a level
> **Gem:** `GOAT_Animation`
> **Folder:** `Modules/Animation/`

---

## What it is

Verbs for driving animation from an agent's program, so a behaviour can play a motion without the
core knowing anything about animation, and a component that brings animation events back so an
animation can open a decision window for the tree.

| Verb | Does |
| :--- | :--- |
| `play_motion` | plays a named motion |
| `animate` | drives animation state from the agent |
| `wait_signal` | pauses the branch until an animation signal variable is true, with an optional timeout |

---

## Signals: animation to AI

The **GOAT Animation Signals** component listens to EMotionFX motion events on its entity
(`ActorNotificationBus`) and writes agent Bool variables. A ranged event becomes a **window**, true
from its start to its end; a one-shot event becomes a **pulse**, true for a quarter of a second. The
design is in [[Animation Signal Hook]].

| Variable | Type | Written |
| :--- | :--- | :--- |
| each binding's variable | Bool | when its signal opens or closes |
| each binding's ready variable | Bool | while the window is open and every required variable holds |
| `anim_signal` | Name | with the parameter of the latest signal |
| `anim_signal_serial` | Int | counts up on every signal, so a repeat still wakes a guard |

Because a guard watches one variable, the common pattern is a branch that a window opens:

```lua
selector {
    sequence {
        condition "combo_ready" { abort = "lower_priority" },
        attack "slash_2",
    },
    attack "slash_1",
}
```

where `combo_ready` is the window combined with whatever else must hold. A binding's **Ready
variable** is exactly that: it is true only while the window is open and every variable under *Also
require* holds (a leading `!` means false). The component re-reads the required variables every
frame while the window is open, so the ready variable turns true even if a requirement only becomes
true halfway through the window. A required variable that is missing or not a Bool never holds.

- **Threads.** EMotionFX may raise the event on a job thread, so the component only copies it into a
  locked queue, and every blackboard write happens on the main thread tick.
- **Lost end events.** A window closes itself after its *Max seconds*, since a clip that was blended
  out or interrupted may never raise its end.
- **Fading clips.** A start event from a motion contributing less than *Minimum weight* of the pose is
  ignored. An end event always counts.
- **Declaring the variables.** The component declares each binding's variable in `Init`, before any
  component activates, so the agent's tree can name it when it compiles.

---

## Why it is a separate gem

Animation drags in EMotionFX. Keeping it out of the core means a project that renders its
characters some other way — or a headless server running the same AI — does not pay for it, or
even build it. A project with another animation system writes the same variables from its own
producer, and every tree keeps working.

This is the same reasoning as the paradigm gems: the core holds what every game needs and nothing
else.

---

## Related

- [[Animation Signal Hook]]
- [[Extensibility Model]]
- [[Adding New Actions]]
- [[Navigation]]

---

*Last updated: 2026-10-03*

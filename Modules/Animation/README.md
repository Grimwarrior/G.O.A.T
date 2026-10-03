# GOAT_Animation

Animation vocabulary for GOAT agents. Only this gem knows EMotionFX exists; a project that
animates some other way, or not at all, does not enable it and the words disappear with it.

## What it adds

| Word | Takes | What it does |
|---|---|---|
| `animate` | `parameter` (required), `key`, `amount` | Writes one named anim graph parameter, then succeeds |
| `play_motion` | `motion`, `seconds` | Plays a motion and runs for its duration |

### `animate` — the one to reach for

```lua
animate "Speed"   { key = "nav_remaining" }   -- value comes from the blackboard
animate "Alerted" { key = "target_seen" }
animate "Stance"  { amount = 2 }              -- or from the node itself
```

The tree says what the agent **is**; the anim graph decides what to play. No clip name ever
appears in a behaviour tree, so re-authoring the graph does not re-author the behaviour — the
same split Unreal has between a Behavior Tree and an Animation Blueprint.

Which parameter setter runs follows the blackboard variable's **declared type**: a `Bool` variable
goes to `SetNamedParameterBool`, `Float` and `Int` to the float setter, `Vector3` to the vector
setter. A tree therefore cannot silently push a bool into a float parameter and get a zero.
Needs an **Anim Graph** component on the agent.

### `play_motion` — for a project with no anim graph

```lua
play_motion "animations/wave.motion" { seconds = 2 }
play_motion {}                                  -- plays whatever the component already holds
```

Needs a **Simple Motion** component on the agent. Without a `seconds` the action runs for the
motion's own duration; with one, the tree can cut a long clip short.

## Setting up an agent

1. **Actor** component with the agent's actor asset.
2. **Anim Graph** component (for `animate`) or **Simple Motion** component (for `play_motion`).
3. **GOAT Agent**, pointed at the tree that uses these words.

## Animation signals: the other direction

`animate` sends the agent's state to the animation. **GOAT Animation Signals** brings events back, so
an animation can open a decision window for the tree, such as "you may cancel into the next attack
now". The animator decides *when* the agent may branch, and the tree decides *whether* it does.

It listens for EMotionFX motion events of one type (`GoatSignal` by default) and writes agent Bool
variables. In the Animation Editor, drag a **ranged** event over the frames of the window, set its
type to `GoatSignal` and its parameter to the signal name, such as `combo`. Then add the component
to the agent next to its **Actor**, and list a binding per signal:

| Binding field | Meaning |
|---|---|
| Signal | The event parameter to listen for, as in `combo` |
| Variable | The Bool it writes, as in `window_combo`. Declared automatically, before the agent's tree compiles |
| Window | On for a ranged event (true from its start to its end), off for a one-shot event (a pulse of a quarter of a second) |
| Max seconds | A window whose end event never arrives closes itself after this long. Zero never closes it |
| Wake agent | Also wake the agent when the variable changes, for `wait_signal` |
| Ready variable | Optional. A second Bool, true only while the window is open **and** every required variable holds. Declared automatically |
| Also require | Agent Bools the ready variable also needs, as in `target_in_reach`. A leading `!` means false, as in `!stunned` |

Two variables are declared for every project that has the module, for trees that react to any signal:
`anim_signal` (Name, the latest parameter) and `anim_signal_serial` (Int, counts up on every signal, so a
repeat of the same signal still changes it and wakes a guard).

```lua
selector {
    sequence {
        condition "combo_ready" { abort = "lower_priority" },   -- the window AND target_in_reach
        attack "slash_2",
    },
    sequence {
        attack "slash_1",                                       -- its animation opens the window
        wait { seconds = 0.5 },
    },
}
```

A guard decides on its own variable alone, so a window plus a second condition has to be one
variable. That is what the **Ready variable** is for. Give the binding for `combo` the variable
`window_combo`, the ready variable `combo_ready`, and `target_in_reach` under *Also require*; the
component then keeps `combo_ready` equal to *window open AND `target_in_reach` true*, re-checking
every frame while the window is open. A guard on `combo_ready` fires when both hold, even if
`target_in_reach` only becomes true halfway through the window, and never restarts the current attack
for a window it could not use.

The required variables can be anything else that writes an agent Bool, such as `perc_target_visible`
from [[Perception]] or one your own tree sets. One that does not exist or is not a Bool counts as
not holding, with a single warning, and is looked for again every frame. To simply pause a branch
until a window opens:

```lua
wait_signal "window_combo" { timeout = 1.5 }   -- succeeds when it is up, fails after 1.5 seconds
```

Events can arrive on a job thread, so the component copies them into a locked queue and does every
blackboard write on the main thread. Start events from a motion contributing less than **Minimum
weight** of the pose are ignored, so a fading clip opens nothing; end events always count.

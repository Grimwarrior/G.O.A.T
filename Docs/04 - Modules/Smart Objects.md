---
type: module
status: implemented
tags: [module, smart-object]
---

# Smart Objects (GOAT_SmartObject)

> **Status:** Implemented
> **Gem:** `GOAT_SmartObject`
> **Folder:** `Modules/SmartObject/`

---

## What it is

Lets props advertise what they can be used for, so an agent can look for "somewhere to sit"
rather than being told about a specific bench.

Put a **GOAT Smart Object** component on the prop, list what it offers, and agents can claim it.

---

## Authoring the prop

| Field | What it is |
| :--- | :--- |
| `Uses` | what an agent asks for, as in `sit` or `drink`. A program claims one by name. |
| `Anchor offset` | where the agent should stand, relative to this entity |
| `Capacity` | how many agents may use it at once |
| `Tags` | labels a claim can require, as in `indoor` |
| `Owner` | who it belongs to, as in a household; empty means anyone's |

It requires a `TransformService`, because the anchor is this entity's transform plus the offset.

---

## The verbs

| Verb | Does |
| :--- | :--- |
| `claim_smart_object` | finds and reserves an object offering a use |
| `use_smart_object` | uses one already claimed |

A claim publishes three variables the rest of your program can read:

| Variable | Holds |
| :--- | :--- |
| `so_entity` | the object claimed |
| `so_anchor` | where to stand — feed this straight to `move_to` |
| `so_use` | which use was claimed |

`claim_smart_object` takes the use, an optional `radius` in metres (`goat_smartObjectRadius`
otherwise) and an optional `owner`, which names a blackboard variable holding the owner to look
for: objects of that owner or of none match, so one program serves every household. Code claims
with a `SmartObjectQuery` (use, from, radius, owner, required tags), reads an agent's claim with
`FindClaim` and moves an object to another household with `SetOwner`. On an entity with the
component, `GOAT_SmartObjectEntityRequestBus::SetOwner` also keeps the owner in the component, so
an entity saved and loaded again belongs to the same household.

`use_smart_object { seconds = N }` holds the slot for N seconds of the agent's ticks, then gives it
back.

---

## A typical shape

```lua
sequence {
    claim_smart_object "sit" { owner = "household" },
    move_to { key = "so_anchor", tolerance = 0.5 },
    use_smart_object { seconds = 5 },
}
```

Claim, walk to the anchor, use it. Because `so_anchor` is an ordinary blackboard variable, the
navigation gem needs to know nothing about smart objects.

---

## Capacity and claims

Capacity is what stops six agents converging on one chair. A claim is held until the agent
releases it, claims something else, or is unregistered: the claim verb's `Forget` (every verb
hears `IActionState::Forget` when its agent unregisters) gives it back, so an agent that dies
mid-walk does not leave the bench reserved forever.

---

## Related

- [[Navigation]]
- [[Extensibility Model]]
- [[Adding New Actions]]

---

*Last updated: 2026-10-02*

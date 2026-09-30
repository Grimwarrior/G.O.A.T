# CLAUDE.md

GOAT is an O3DE gem for NPC decision making (see `README.md` and the Obsidian vault in `Docs/`, starting at `Docs/Home.md`).

- The core (`Code/Source/Core`, `Code/Include/GOAT`) runs agents: `IAgentSystem` (`AgentSystemInterface`) registers them, `AgentRuntime::Tick` advances one, and paradigm gems (`Code/Source/Backends/`: `tree` behavior trees, `htn`, `utility`) decide through `IDecisionBackend`. Programs are authored in Lua; verbs are `IActionState`s a game installs with a `VocabularyScope`.
- Agents tick in pacing bands of `AZ::ScheduledEvent`s (`AgentRegistry`), or on `ManualBand`, which nothing schedules: `IAgentSystem::TickAgent` ticks such an agent when its game says, e.g. once a turn.
- Tests are in `Code/Tests` (`GOAT.Tests`, enabled by `PAL_TRAIT_GOAT_TEST_SUPPORTED` in `Code/Platform/<OS>/PAL_<os>.cmake`). Build it in a project that enables the gem and run it with `AzTestRunner.exe GOAT.Tests.dll AzRunUnitTests`.
- Keep the docs in `Docs/` in step with the code when an API changes.

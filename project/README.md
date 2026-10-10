# Editable native project

Open `project.gbsproj` with GB Studio through the ModRetro Chromatic plugin. The start scene is `Toronto north-up city`, using the project-local `TORONTO` engine scene extension. The original labelled workshop is retained as a separate development scene.

The 1,024 × 976 background, palette/priority attributes and collision map are native editable resources. Vehicles, walkers and the beacon share original authored sprite cells. Driving, pedestrian routes, contracts, transit, UI and versioned SRAM persistence are implemented in `plugins/toronto-driving/engine`.

See [build instructions](../docs/BUILD.md), [controls and scope](../README.md) and [test evidence](../TESTING.md). Generated ROMs, local captures and save states are ignored.

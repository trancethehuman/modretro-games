# Native city atlas

Updated 2026-10-02. This milestone adds a browsable schematic of the four registered city areas to the existing paused map screen. Optimized candidate `e812f7ef…` has built through the official plugin, passed linked memory inspection and sampled ordinary-button native map/driving/transit checks. It follows unpublished intermediate `ec982d…`, whose slower redraws prompted the pattern-cache optimization. Publication, remaining native cases and physical checks are separate gates. Published Prototype 4, ROM `1da71ba5…`, contains the earlier camera-panning district map; its recordings do not verify this atlas. [TESTING.md](../TESTING.md) records evidence against each actual ROM.

## Coverage and controls

The atlas uses the offsets in [world.json](../content/districts/world.json), rather than district-ID order:

| West to east | District ID | World offset X | Atlas offset X |
| --- | --- | --- | --- |
| High Park / Junction | 2 | 0 | 0 |
| West End | 1 | 1,024 | 128 |
| Central Toronto | 0 | 2,048 | 256 |
| Toronto East End | 3 | 3,072 | 384 |

Each actual scene is 1,024 × 976 world pixels. At 1:8 scale, its 128 × 122 collision cells become map pixels. The combined atlas is **512 × 122 pixels**, padded to **512 × 128 / 64 × 16 tiles**. The last six pixel rows are solid padding. The map body displays **20 × 12 tiles / 160 × 96 pixels**, with two header rows and four legend/control rows on the 160 × 144 screen.

Open **Start → Scroll City Map**. The initial view centres the courier. D-pad pans across the atlas. A centres the active job objective. With no active job, it centres the Union **DEPOT** during free roaming, or the booked **STOP** when opened from paused transit waiting/riding. That trip view uses the destination in `td_cursor`, with a **TRIP** focus label. Active jobs continue to use `td_target`, including during transit. Select cycles **You → Car → Objective** on foot. Driving cycles **You → Objective**, skipping the redundant car focus. B or Start returns to the pause menu. Focus changes and exit can interrupt a partial redraw; normal panning waits for the twelve-row map body to finish drawing.

**P** marks the courier, **C** the current or parked vehicle, and **O** the job objective, depot or booked trip stop. On foot, the car uses its own district and saved local position, including when parked remotely. Overlapping markers retain a main letter and bottom ticks: left for P, centre for C, right for O. O has letter priority, then P, then C. The legend and focus text identify the selected view.

For foot-only Colborne, Withrow and Greenwood jobs, a driver's O marker uses the legal road parking anchor. Exiting the car immediately restores the actual client marker. This uses the existing mode-aware target getter and changes no handoff requirement or job stage. District routing and the local next-seam beacon remain separate from atlas browsing.

This coverage is the four compressed areas already registered in the game. Full former City of Toronto, its waterfront and fuller Islands remain the accepted expansion target. The proposed 17-district layout and its ID assignments are separate design work; unavailable districts are not drawn. The map era remains unadopted. Atlas generation and contract counts do not establish two hours of gameplay, human enjoyment or physical cartridge behaviour.

## Ground data and generation

[create_atlas.py](../scripts/create_atlas.py) reads the actual registered scene collision grids and hashes their resources, collision encodings/bytes, authoritative world offsets and authored water metadata. It writes [atlas.json](../content/atlas.json), [td_atlas.h](../project/plugins/toronto-driving/engine/include/td_atlas.h) and [td_atlas.c](../project/plugins/toronto-driving/engine/src/td_atlas.c). It changes no scene, collision, art asset, client or contract. No PNG is required; the generated schematic is native 2bpp data.

Each native tile contributes one map pixel: **0 solid/other**, **1 road** from collision 0, **2 walking ground** from collision 16, and **3 water** only where collision 15 also lies inside an authored water mask. Roads and walking ground take precedence, preserving reopened bridges and paths. Water samples tile centres at `(8x+4,8y+4)`. The core uses its authored river and harbour bounds with Island land exclusions; West and High Park use shore rectangles, and High Park also uses its pond polygon. Rectangles are half-open; points on the pond boundary are included. The pond's blocked fringe remains solid where its tile centre falls outside the polygon. East's authored water list is empty, so this atlas adds no Don or Port Lands water there. Roof artwork is not reduced into a misleading ground route.

The generator validates contiguous registered IDs, actual native scene identity/dimensions, metadata agreement, non-overlap, coordinate bounds, water geometry and native array limits. It verifies the dictionary reconstructs every schematic pixel, including padding, and checks **every one of the 225 possible 20 × 12 viewports**.

| Current generated budget | Value |
| --- | --- |
| Distinct 8 × 8 patterns | 457 |
| 2bpp dictionary | 7,312 bytes |
| 1,024 UWORD map indices | 2,048 bytes |
| District origins and names | 92 bytes |
| Total ROM data | **9,452 bytes** |
| Generator data ceiling | 12,288 bytes, leaving 4 KiB for BANKED code |
| Most patterns in a visible viewport | **161 of 172 reserved slots** |
| Static WRAM in the atlas data/API module | 0 bytes |

The arrays and getter code are autobanked together. Optimized candidate `e812f7ef…` retains the **10,100-byte atlas unit: 9,452 data + 648 API code**. Its inspected memory guard places heap end at **D950**, stack/OAM boundary at **DF00**, leaving **1,456 bytes**; fixed ROM bank 0 retains **127 bytes**. Packing leaves **7 bytes in UI bank 17** and **6 bytes in atlas bank 15**. The earlier `ec982d…` UI bank had 106 bytes free; that is not the optimized candidate's headroom. The data ceiling and linked guard establish allocation bounds, not deepest stack use or performance under every workload. Further changes need a fresh official build and inspection.

From the repository root:

```sh
python3 -B games/toronto-dispatch/scripts/create_atlas.py
python3 -B games/toronto-dispatch/scripts/create_atlas.py --check
python3 -B scripts/test_atlas.py
python3 -B scripts/test_atlas_ui.py
```

`--check` recomputes the source hashes, pixels and budgets and compares deterministic JSON/header/C output without writes. [test_atlas.py](../../../scripts/test_atlas.py) compiles the unchanged BANKED API through [atlas_harness.c](../../../tests/engine/atlas_harness.c) using host type/bank adapters. Its independent collision/water oracle covers all 65,536 padded raster pixels, row slices, placement/quantisation, names, viewport budgets, invalid/null inputs and unchanged failed outputs. The current source passed 299,366 ASan/UBSan checks. Host checks do not establish native ABI, upload timing, framebuffer appearance or hardware behaviour.

[test_atlas_ui.py](../../../scripts/test_atlas_ui.py) compiles the actual renderer and atlas API through [atlas_ui_harness.c](../../../tests/engine/atlas_ui_harness.c), with host VRAM/window adapters. The optimized renderer passed **8,036,093 ASan/UBSan checks**, including all 225 viewport renders, frozen world/player/job/transit state, VRAM tile/font ownership, P/C/O combinations, focus changes, cancellation during redraw and bounded pattern-cache lookup. The preceding renderer's 7,568,628-check result is historical. Host coverage of all 225 viewports does not mean all were executed natively; native timing and state samples are scoped below.

## Read API and renderer ownership

All five public getters are `BANKED`. Callers provide WRAM output buffers; FALSE leaves every output unchanged. Bounds are unpadded map pixels. `td_atlas_position()` accepts whole **local world pixels** and returns whole **atlas map pixels**. `td_atlas_row()` accepts map tile coordinates and 1–20 entries without wrapping across a row; its UWORD IDs select a 16-byte Game Boy 2bpp pattern through `td_atlas_pattern()`. `td_atlas_district()` returns a terminated nineteen-byte district name and rejects out-of-bounds/padded positions. Keep global atlas coordinates out of the GBVM Q5 camera/actor fields, which would overflow. The API uses bounded 16-bit arithmetic and no persistent world cache.

The [renderer in td_ui.c](../project/plugins/toronto-driving/engine/src/td_ui.c) uses a fullscreen window at `(0,0)`. Its 172-slot/344-byte pattern cache shares the existing 360-byte text-row cache through a union. Each redraw clears the slots to `0xFFFF`. The optimized lookup computes the first slot by repeated subtraction of 172 and probes with stride `1+2*(id&15)`. These odd strides 1–31 are coprime to `172=4×43`, so every slot is reachable; a compile-time guard fixes that capacity and lookup is bounded to 172 probes. This adds no WRAM or 32-bit arithmetic. Map drawing bypasses the text cache; exit invalidates text rows before repainting the pause screen. Row uploads are bounded across updates. The reserved CGB bank-1 tile ranges are **markers 8–14**, **ground 16–187**, and **font 192–240**. Marker combinations cover all P/C/O overlaps. Row uploads restore VRAM bank 0.

Read-only inspection of the exact Prototype 4 ROM confirmed the original ownership of these ranges: its bank-1 background sets use eight core/five West patterns at IDs 0–7, while the 56 bank-1 courier sprite tiles occupy the separate sprite region. The optimized candidate adds linked inspection and sampled native map/font restoration. Future assets or additional city areas must revalidate VRAM ownership, regenerate the atlas, and pass every visible pattern budget before a new build.

Browsing updates only transient UI view/focus/redraw state. It keeps the active gameplay scene loaded, saves/restores camera coordinates/settings and every current actor's hidden bit, and uses the existing paused update path to freeze movement, world clock, job deadlines and transit ride time. It does not write progress or alter district-qualified player/car positions, cash, cargo or fare state. The source retains the 58-byte version-6 save layout.

## Optimized candidate: sampled native evidence

Two finalized recordings identify the native checks on candidate `e812f7ef…`. Inputs were ordinary buttons; state, camera and graphics inspection was read-only. The records remain separate from host tests and the published Prototype 4 binary.

| Scenario | Video frames / events | Journal SHA-256 | Retained archive |
| --- | --- | --- | --- |
| Driving and city map | 5,434 / 690 | `f1ddccf5e5f87813d068b57a99ba5a534ad456b3250112a9c98505d998bf38ed` | `eea14575-c66e-42dc-ae5d-9b0b03e6cef2` |
| Transit and city map | 4,466 / 472 | `f6642524fcdbcdedebdaacc9607b82a432957d16b9aa2469af37482dc25e530f` | `dcc747d1-9578-4a23-8e98-96027fb388bc` |

The driving recording repeats first delivery at frame 740 and held-acceleration turning at frame 864, retaining speed 24. Atlas views render Central, High Park, East and West without changing the loaded world. The complete 58-byte state is identical across map frames **980→4,228**. Cancellation restores captured camera bytes `c0634066` (WRAM offset6,036), camera settings3, visible sprites and the font. Samples also cover focus changes during partial redraw, B/Start exit, opposed D-pad inputs, foot car/depot focus and subsequent car entry. An active job retains its **120-second deadline and world second 15** throughout frames **5,090→5,410**.

With no active job, the transit recording shows paused RIDE/WAIT maps using the booked **KING/UNION STOP**, with the TRIP label. WAIT frames **2,826→2,986** retain all 58 state bytes, including world second 20, cash 24 and subsecond 48. Later boarding reaches a paid ride at world second 36/cash 21. A soft reset while its map is paused reaches HELP at frame **4,338**, restoring that paid RIDE with world second 36, cash 21 and ride remainder 1. At frame **4,466**, the courier arrives at King, world second 38/cash 21, with no extra fare.

The earlier unpublished `ec982d…` rendered all four areas and retained frozen state through cancellation but exposed slower redraws; its linear lookup and recording do not substitute for the optimized candidate. Native checks above are sampled cases. Remote parked-car atlas focus, native parking-anchor/client switching, every viewport/focus combination, crowded-scene performance and remaining campaign cases still need coverage. Full former Toronto/waterfront/Islands, two measured hours of enjoyable gameplay, human handheld/audio review and physical cartridge boot/save/read-back remain pending. Record later acceptance and publication under their own identities in [TESTING.md](../TESTING.md).

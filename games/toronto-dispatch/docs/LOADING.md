# Load Toronto Dispatch onto your Chromatic

Select the tested local playtest candidate, **`toronto-dispatch-driving-only.gbc`**, **1,048,576 bytes**, SHA-256 **`8be1eff05236edfd05e475fe143f5b48e20db8d4a09c30f46e038e7d9643bdfb`**. It contains seven compressed districts, 104 contracts, 64 service points and save v10 / 58 bytes. The supported target is your **writable ModRetro DevDay cartridge**; it can be empty. This is a CGB-only homebrew ROM.

Use **`project/build/toronto-dispatch-driving-only-reviewed.zip`**, described in step 2. Official compilation and repository checks pass. The [fresh current native run](NATIVE_DRIVING_ONLY_WARNING.json) completes four jobs and verifies freight driving-only feedback, original-truck recovery, funded police penalty and committed in-worker reset. The retained 7ab ROM has separate genuine sixteen-job/all-eight-kind/seven-district tests; those totals are not transferred to 8be1. Detailed [build identity](BUILD.md) and [test scopes](../TESTING.md) preserve older results and failed attempts.

This candidate still needs full 104-contract play, measured two-hour human enjoyment and hardware checks. Motorcycle pacing and broader balance remain open. Emulator reset proves committed in-worker progress, with periodic saves, rather than physical cold-boot/battery persistence or every latest live field. No physical installation has been performed.

A focused host test reproduces blocked walking after saving/resetting during car entry. If this occurs beside your parked vehicle, press A to finish entering and recover; avoid saving mid-entry until the repair and native test are complete. This is host reproduction, separate from the current native playtest.

## 1. Prepare the computer and console

1. Install the ModRetro Chromatic plugin in Codex on the Mac connected to the console. It is already installed in this project owner's current setup. Open this repository as the project.
2. Use the [official ModRetro Updater](https://support.modretro.com/en_us/articles/chromatic-firmware-updater-ryhoYnzCx) for the firmware/setup steps described in the [DevDay quickstart](https://support.modretro.com/en_us/chromatic-devday-edition-quickstart-guid-By1iOlcMg). If this computer needs Developer Mode, press **Cmd-I** in the updater and enter the included activation code privately. Windows uses Ctrl-I. Keep the code out of chat and the repo. Plugin discovery cannot tell you whether activation is complete.
3. Power off the Chromatic, insert the writable ModRetro development cartridge, connect it using a USB **data** cable, then turn it on. Leave it connected during detection or installation. If more than one Chromatic is connected, identify the intended one instead of assuming Player 1.
4. Return to Codex after the updater has finished its work. macOS requires no extra vendor driver. If the plugin reports a concrete access or setup issue, follow that result and the [shared hardware workflow](../../../docs/HARDWARE.md).

The updater activates the computer and handles console firmware. The game itself is loaded through the plugin's cartridge write action. Do not use Cart Clinic's update button as a substitute for selecting this original ROM.

## 2. Select the actual game build

The editable project is `games/toronto-dispatch/project/project.gbsproj`. The selected loading ROM is:

```text
games/toronto-dispatch/project/build/toronto-dispatch-driving-only.gbc
```

Generated ROMs are excluded from Git. Any later source change needs a distinct output with its own matching debug artifacts, inspection and native record; do not reuse this measured candidate hash. Official downloadable bundles include `SHA256SUMS`, instructions and notices. Do not rename a browser export or `.gbsproj` file to `.gbc`.

The loading bundle filename is **`project/build/toronto-dispatch-driving-only-reviewed.zip`**. The reviewed ZIP is **146,497 bytes**, SHA-256 **`fe672246e433ec6ca48b52c49c61ec833f65d223bfbbce334c0051e47951a0b4`**, frozen at matching source **`de96a675e90d0941c0ac4acc62d62223e6cdcc5f`**; its [package check](DRIVING_ONLY_PACKAGE_AUDIT.json) passes 300 bounded assertions. Extract it and verify `SHA256SUMS` against the selected 8be1 ROM digest above. Retain `BUILDINFO.json`, the loading guide, licence and notices alongside the game. BUILDINFO identifies the matching source commit; the bundled guide remains frozen at that revision. Preserve the earlier 7ab vehicle-feedback reviewed ZIP unchanged. Generated ROMs and ZIPs remain local unless a release explicitly publishes them.

For a source build, ask Codex:

> Use the ModRetro Chromatic plugin. Select Toronto Dispatch and build current source to a new distinct filename under `build/` with matching debug artifacts, preserving measured `toronto-dispatch-vehicle-feedback.gbc`, `toronto-dispatch-ferry-clarity.gbc`, `toronto-dispatch-right-hand-traffic-filter.gbc`, `toronto-dispatch-north-initial.gbc` and all older files. Run the memory and compiled resource/frame/isolation/progress-table guards, inspect its exact path/size/SHA-256 and test that same ROM's quest previews/payment, street/courier/transit/save behavior and pacing before preparing installation. Retain older measured binaries separately.

Use `rom_inspect` on the final file. Match its digest to the tested build in [TESTING.md](../TESTING.md) or the downloaded release's checksum. A new build can have a different hash: compare it to its own new inspection/playtest rather than silently adopting an old checksum. The current engine uses MBC5 and battery SRAM; ROM header validity alone does not prove that a cartridge supports it.

Optional read-only checks for the expanded candidate from the repository root on macOS (substitute the exact new filename for a new build):

```sh
shasum -a 256 games/toronto-dispatch/project/build/toronto-dispatch-driving-only.gbc
wc -c < games/toronto-dispatch/project/build/toronto-dispatch-driving-only.gbc
```

For a downloaded bundle, from its extracted directory:

```sh
shasum -a 256 -c SHA256SUMS
```

Only proceed when the named ROM's checksum passes. Keep the bundle's build/source identity and notices alongside the ROM.

## 3. Install on the writable cartridge

The direct Codex/plugin route works independently of a browser preview. After the build is inspected, send this prompt and identify the selected device if there is a choice:

> Use the ModRetro Chromatic plugin to find my connected Chromatic and the writable ModRetro DevDay cartridge I inserted. Detect that cartridge if needed, then rediscover the intended device. I acknowledge that writing will erase this cartridge's existing game data, may lose saves, and makes no automatic backup. I approve writing the exact Toronto Dispatch ROM just inspected and tested to this selected development cartridge. Show me the file path, byte count, SHA-256 and selected device, then flash it through the supported plugin workflow and follow the original operation to completion. Do not alter unrelated firmware or another cartridge. Guide me through a real cartridge cold boot afterward.

The plugin must use the inspected ROM path, size and SHA-256 in `flash`, along with a fresh selection token and a unique request ID. If cartridge detection consumed a token, it must rediscover before writing. Keep the computer, cable and console connected until the original write operation has finished.

A working official browser preview also offers **Install on Chromatic**: choose the intended device, review the erasure warning and choose **Confirm Install**. This installs that preview's built ROM, which can differ from the native ROM. Use its exact inspection/test evidence. The project's previously paused preview has an unresolved close acknowledgement; use the direct route above while that state is unresolved.

**A successful vendor write is installation evidence. It is not evidence that the cartridge cold-boots, runs smoothly or preserves saves.** If the tool supplies read-back verification, retain that result separately; if it does not, record read-back as unavailable.

## 4. Verify the cartridge, then play

After a successful write, power off and disconnect USB, then power on with the same cartridge inserted. This tests stored cartridge execution independently of host streaming. Use the physical buttons; emulator inputs cannot control the console.

| Check | Expected result |
| --- | --- |
| Cold boot | Toronto Dispatch's title/help/start flow appears and enters the city; no blank screen, corrupt tiles or reset loop |
| Driving | A accelerates, left/right steer, B brakes and reverses near rest; corners retain momentum |
| First delivery | Select opens dispatch; accept the first Union-to-St. Lawrence job with A, Select collects at Union, drive east on Front Street, brake and Select delivers at the marker |
| Menu exit controls | Held A/B used to close Help, Pause or dispatch stays consumed until release; fresh presses accelerate/brake/reverse. Momentum still coasts after resuming. RESULT B returns to roaming and A opens dispatch |
| Quest planning and payment | Dispatch Select jumps chapters; left/right selects offers and up/down browses stops. During a job open Pause → Dispatch jobs, check CURRENT STOP and A RESUME without replacing work. Check condition/base/time/credited pay after delivery, and zero payment after timeout |
| Walking and car entry | Stop, Start → Park / recover car; walk with D-pad, approach the parked car and press A to enter. With no active job, the completed-entry HUD shows the vehicle and driving controls |
| Map and pause | Start → Scroll City Map; D-pad pans across areas, A centres the job/booked stop/depot, Select changes focus, B returns; mission time freezes |
| Transit | On foot at a station, Queen curb sign or ferry terminal, B opens routes; choose with left/right and board with A. At Wellesley, up changes train/bus. Check direction, wait and ride time before boarding; fare is charged once and mission time continues |
| Islands | Offers show $8/$16/$24 ferry budgets; extra travel/fines are excluded. Park on the mainland and use public paths/bridges. If an active job leaves less than $4 at an Island dock, try the return, then follow Start → Cancel Active Job. Cancellation gives no payout/completion; B reopens $0 return assistance on the normal schedule. B cancelling WAIT alone does not abandon a job |
| Northern district | Follow Spadina or Yonge north of Bloor. Complete three starter jobs to unlock chapter 13's Baldwin Book Box, collect at Union and park below Baldwin Steps; walk the public stair route to deliver |
| Northern train stops | Walk to Summerhill or St Clair, press B, choose the other stop and board with A. Each trip costs $3; the parked car stays at its original location |
| Street consequences | Human impacts have non-graphic recovery, condition loss after pickup, $20/$40/$60 escalation and H1/H2/H3 pursuit. Capture clears attention and clamps its $25/$100/$225 fine at available cash. A car blocking a lane makes NPCs wait; move it clear and check police return |
| Saving | Use the pause menu's Save action, record cash and completed count, power off/on and confirm both persist |
| Audio | Start → Audio, then A cycles music + effects, effects only and silent; B returns. Check the city score, engine and braking sounds, short delivery/transit cues, and silence in silent mode. Menu/world pause stops music and engine; short interface/result cues may finish. The mode defaults on each boot |
| Readability/performance | Check text, building occlusion, traffic and pedestrians for flicker, slowdown or delayed input |

Keep a note of the ROM SHA-256 and any problem's location/action. Save v10 preserves the old 96 contract IDs, appends eight North contracts and explicitly imports valid v9 and older supported records through documented migration. Host and compiled checks cover older saves; native old-save imports and physical migration are unverified. See BUILD/TESTING. Emulator reset persistence does not prove power-off persistence on a physical cartridge.

## If installation does not finish

- **In progress, lost reply or unknown result:** keep the device connected and use the same operation's **Check status** or have Codex query its original operation ID. Do not press Install again or create a new flash request. A closed panel does not cancel a write.
- **Developer Mode required:** preserve the original error/outcome, complete activation privately in ModRetro Updater, then return for fresh discovery and a newly requested write after the old operation is resolved.
- **Connection or cartridge error:** use the original diagnostic and its recovery instructions. Power off before reseating the cartridge. A generic `device.program_failed` does not identify activation, driver or contact failure by itself. Only writable supported ModRetro cartridges are eligible for this path.
- **Finished failure:** use **Copy error** for the bounded report and original operation ID. Keep raw journals private. If the plugin confirms closure, acknowledge/dismiss that failed operation before an explicitly requested new installation; dismissing does not prove the cartridge was unchanged.
- **Successful write but bad boot:** preserve the exact ROM/hash and observation. Test the same ROM in the native emulator and report the physical failure before rewriting. Do not reset console firmware or use an unrelated flash profile as an automatic fix.

A host-streamed `play` demo is optional and never writes the cartridge. It can help assess the screen/buttons, but it cannot replace the cold-boot and save checks above.

Device procedure reviewed against installed plugin 1.0.33 deployment documentation; the selected driving-only ROM was inspected on 2026-10-04. No activation code, device token or private preview URL is required in these instructions.

## Evidence and older builds

The selected driving-only build's [focused audit](DRIVING_ONLY_BUILD_AUDIT.json) authenticates 206 inputs with only one same-length UI literal changed from 7ab. Its [fresh scoped replay](NATIVE_DRIVING_ONLY_WARNING.json) completes four unique deliveries, verifies both freight stages' visible warning, required-truck handoffs, funded H3 fine and later committed in-worker reset. UI/helper/gameplay banks have 150/1/10 free bytes, reserve 1,096, deepest stack unmeasured. Source inspection shows the custom Toronto bank 3 journal does not use the changed stock-save signature; native cross-ROM import and physical persistence remain untested. Supported discovery on 2026-10-04 found zero connected Chromatics; no physical loading is claimed.

The retained a935 ferry-clarity build's [focused audit](FERRY_CLARITY_BUILD_AUDIT.json) and [fresh scoped replay](NATIVE_FERRY_CLARITY.json) keep their own identities. That separate replay ends with one new completion/$110 after paid travel, low-cash cancellation, scheduled assistance and car recovery. Its UI/helper/gameplay banks have 122/1/10 free bytes. Native reset proves committed in-worker SRAM restoration, not physical cold-boot persistence.

The retained e4a9 traffic build's [source/build audit](RIGHT_HAND_TRAFFIC_BUILD_AUDIT.json) authenticates its lanes/police/ABI guards. Its own [fresh native replay](NATIVE_RIGHT_HAND_TRAFFIC.json) ends with three completions/$101 after $20/$40/$60 human fines, underfunded H3 capture, recovery delivery and two $3 train fares. The [initial traffic replay](NATIVE_TRAFFIC_INITIAL_REVIEW.json) remains needs-review. Synthetic coordinate-filter work reduction does not establish a native/whole-city speedup, and old gameplay outcomes are not assigned to a935.

### Historical menu-input build


The historical menu-input build's [resource audit](MENU_INPUT_BUILD_AUDIT.json) passes 219,647 checks; a separate linked instruction review passes 231. Its [fresh native record](NATIVE_MENU_INPUT_RELEASE.json) covers the controls and one delivery described above, with no progress imported from old ROMs. Curation passes 13,669 checks and independent native/prose comparison passes 34,355 with zero findings. Fresh plugin discovery on 2026-10-04 found no connected Chromatic; physical loading remains pending. The stopped Core / uncrowded North samples count 345 / 686 completed updates across separate 1,080-VBlank windows. They do not establish whole-city pacing or crowded performance. Gameplay bank 2 has one byte free, UI bank 1 three and save bank 29 fifteen; static reserve is 1,096 bytes, with deepest stack use unmeasured.

### Historical initial North build

The old `toronto-dispatch-north-initial.zip` remains unchanged: 138,530 bytes, SHA-256 `c721f8597f8b50f7bcff2fc61ca1a0dedef53004499b1588182ec4e3e4bb49ac`, ROM `22157d720c8746118da93bb7c3027ef24693a2e4697118e70e5a95fecddf0d95`, source `8290eefba2c909ef004970669b05588c471b2d10`. Its [package audit](NORTH_PACKAGE_AUDIT.json) passes 1,235 checks. This older build has the reproduced Pause B-input leak; select the current driving-only candidate above for current playtesting. Its bundled guide and same-ROM evidence retain their original identities.

The historical `22157…` North build's [compiled audit](NORTH_BUILD_AUDIT.json) verifies 47,502 assertions and 203 frozen project inputs. Its [fresh record](NATIVE_NORTH_FRESH.json) completes three Core jobs and loads North while carrying job 96. Its [separately recorded continuation](NATIVE_NORTH_CONTINUED.json) completes the Baldwin stairs delivery, verifies two train trips and map/reset/car retention, and exercises reciprocal northern seams. Progress imported from the fresh same-ROM checkpoint is disclosed. Independent fresh and continued reviews pass 37,871 and 55,785 evidence checks, respectively; the continuation also passes 359 documentation/link/privacy checks. A [third scoped record](NATIVE_NORTH_SUPPLEMENT.json) imports those four genuine completions and adds only jobs 3 and 6: undamaged truck pay 157 and condition-80 signed-return pay 120, ending cash 455 / done 6 after $20/$40/$60 pedestrian fines and a $225 H3 police fine. Its independent evidence review passes 31,888 checks. It adds no other northern job or measured campaign-duration proof. Wider regional jobs and performance remain pending. Main code bank 2 is full, UI bank 1 has three bytes free and save bank 29 has fifteen; further code changes require a fresh capacity review. The linked 1,096-byte static reserve does not measure deepest stack use.

The retained [03e09 native record](NATIVE_UI_POLISH_SAMPLES.json) keeps its own six-district, v9 identity: `toronto-dispatch-ui-polish-direct.gbc`, 524,288 bytes, SHA-256 `03e09fa85351c91f56d7370a3f0e0f856cd147a3c8d5ebfd328a2786a38f25d6`. Its older ZIP is 130,616 bytes, SHA-256 `67b7412321d04d9385f9201451fcb8f2755084262042fcfcecc6c26aeb238c21`, source `9816ac43b9c6163882098189c5ac9c93943a70d6`. Its scope and 4,052 packaging checks are not assigned to North.

The [retained e797 22-job campaign](NATIVE_CAMPAIGN_TWENTY_TWO_SAMPLES.json) and [825 workload comparison](NATIVE_TERRAIN_CACHE_COMPARISON.json) keep their original ROM identities. Neither its 22 completions nor the measured 6.648%/3.896% gains are assigned to this North ROM.

The retained `964f…` [five-job continuation](NATIVE_COURIER_CHAIN_SAMPLES.json), earlier matched-route, bus/heat/patrol, Queen and Island records retain their own identities in [TESTING.md](../TESTING.md). Their wider acceptance is not inherited by this ROM. Failed `c4fa…`, `8a96…` and `cf2f…` candidates must not be selected. Downloadable [Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) remains an older separate release.

Full Old Toronto, remaining Island jobs, all contracts/balance, two enjoyable human hours, wider vehicle/crowd/pacing/stack checks, native older-save imports, human handling/audio, browser recovery and hardware remain open. All 36 traffic circuits now use authored right-hand lanes; broader queue/performance behavior and permanent-deadlock hypotheses remain separate. Host migration checks and emulator resets do not establish physical power-off persistence.

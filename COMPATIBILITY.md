# Compatibility and evidence levels

Evidence is scoped to a specific source revision, input and observation. Unit tests do not establish guest application compatibility. Compilation does not establish runtime execution. A submitted command or present call does not establish a sampled CLCD frame or sustained FPS.

| Layer | Result | Qualification |
|---|---|---|
| Portable CPU/devices and host policies | Public CTest coverage | S5LBox-derived unit tests; firmware-dependent branches may self-skip and are not guest-boot evidence. |
| XNU, launchd, SpringBoard | Historical local execution | iPhone 3G/iPhone OS 3.1.3 build 7E18, authorized private components. No boot performed for public-repo preparation. |
| RMUIKit | Historical guest execution | Window and controlled touch/no-touch outcomes. Gallery shows the authentic blue no-touch control. |
| RMGLES1 | Historical guest execution | Matrix-transformed RGB triangle and distinct 190°/330° sampled scanouts. Dither-disabled diagnostic variant. |
| RMTexture | Historical guest execution | Own opaque RGBA8 8×8 texture, triangle/two-triangle square, nearest then linear. Publicly recheckable own input metadata. |
| VGP decoder | Narrow measured support | Six complete normalized envelopes, not arbitrary shaders or VGP instruction interpretation. Firmware-free tests exercise rejection and semantic geometry; private program-byte acceptance requires local authorized inputs. |
| Public scene replays | Reproducible without firmware | Five original synthetic decoded scenes, production semantic C rasterizer and independent image comparison. |
| Z-buffer | Unresolved | Observer only. No validated 16/24-bit tiled format, clear, compare, write or true depth-tested guest scene. |
| Transparency | Partial host semantics only | Straight-alpha semantic tests; requested guest blend family remains unsupported. |
| Dithering | Local GLES family rejected | Current public upstream separately accepts its BGRA8 dither control as a no-op; this project did not import that behavior. |
| Historical Unity | Unexecuted | Import analysis and cross-compilation are not game execution. No Unity runtime is included. |
| Temple Trouble | Unvalidated | No game, assets or commercial IPA distributed or executed in this mission. |
| Android ARM64 | Prior cross-compilation only | Nine targets, API 26/NDK r28c. No APK/JNI, physical device acceptance or performance claim. |
| iOS native host shell | Unvalidated in this project | Attributed upstream reference sources retained; public validation targets macOS desktop CMake. |

No sustained FPS, hardware accuracy, general OpenGL ES 1.1 compliance, Unity compatibility or Android gameplay claim is made. Public reports separate inherited test passes, synthetic replays, historical guest captures and unresolved work.

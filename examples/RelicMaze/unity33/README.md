# Relic Maze — Unity 3.3 source prototype (NOT exported)

This folder contains a **source script** designed around Unity 3.x-era APIs. It is **not** a Unity-generated project, Unity 3.3-validated scene, Unity iPhone player, or iOS IPA.

To try it on a compatible, properly licensed Unity 3.3 installation:

1. Create an **empty** Unity project using that editor.
2. Copy `Assets/Scripts/RelicMaze.cs` into the project's `Assets/Scripts` folder.
3. Create an empty scene and a `GameObject`, and attach the `RelicMaze` script.
4. Add that scene to Build Settings and test in the editor first.
5. Set a **portrait** iPhone target only if the historical Unity iPhone build support and legally usable historical Apple toolchain are actually available.

The script uses `MonoBehaviour`, `Start`, `Update`, `OnGUI`, `Input.GetKeyDown`, `GUI.Button`, `GUI.DrawTexture`, and `Texture2D.SetPixel`. These APIs are old-style, but **actual compilation with Unity 3.3 is unverified**, because that editor is unavailable here. Adjustments may be required when validated against the historical editor.

The GUI rendering is a deliberate **Unity runtime smoke test**, not the same graphics command stream as the native OpenGL ES renderer. Never equate an editor playthrough with a Unity 3.3 player running in iPhone OS 3.1.3.

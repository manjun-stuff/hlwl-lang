# HLWL — High-Level Web Language (`v1.0`)

**HLWL** (`.hlwl`) is a zero-JavaScript, GPU-first web systems language compiled (or directly interpreted for rapid testing) by a **C++20** toolchain (`hlwlc`, `hlwlfmt`, `hlwlls`).

* **Compiled Mode (`interpret = false`, default):** Emits a static browser bundle (`index.html`, `app.wasm`, `shaders.wgsl`, `runtime.js`) or C++20 source (`compileCPP: true`).
* **Interpreted Mode (`interpret = true`):** Executes `.hlwl` directly inside `hlwlc`'s built-in native C++ interpreter for instant local testing without build latency.

---

## 1. Structural Blocks (`<something> ... </something>`)

A `.hlwl` file is organized into HTML-style blocks. Multiple `<file> ... </file>` modules may exist in a single physical file (isolated per module, though one `<file>` per physical file is recommended).

### Necessary & Top-Level Blocks
* **`<file, version, safety_check_level, name, interpret> ... </file>`** *(Required)*
  * `version`: Defaults to the locally installed `hlwlc` version.
  * `safety_check_level` *(optional, default `1`)*: `0` (C++ control), `1` (Rust-style ownership/mutability safety), `2` (Python-style strict runtime guards).
  * `name`: Module identifier.
  * `interpret` *(optional, default `false`)*: Runs via the native C++ interpreter when `true`.
* **`<config> ... </config>`**
  * JSON-compatible key-value pairs wrapped implicitly as a root JSON object (`nlohmann::json` compatible), e.g., `"browserTargets": ["Chrome", "Firefox", "Safari"]`, `"compileCPP": true`, `"includePaths": ["./libs"]`.
* **`<include> ... </include>`** *(Optional, standard in almost every file)*
  * Built-ins (`<stdMath>`), local paths (`"easyGraphics"`), and selective imports (`"mathTypes": Vector2, Vector3`).
* **`<globals> ... </globals>`** *(Optional)*
  * Module-wide constants (`SCREAMING_SNAKE_CASE`) and `state` UI variables shared across `<main.*>`, `<gpu.*>`, and `<layout>`.

### Main Execution Lifecycle (`<main.*>`)
* **`<main.func>`**: Entry functions, helpers, and initialization.
* **`<main.loop>`**: Per-frame update loop (`cfloat dt` injected automatically).
* **`<main.if_func_once, condition>`**: Edge-triggered block; executes **once** when `condition` transitions from `false` to `true`.
* **`<main.if_func_always, condition>`**: Level-triggered block; executes **every frame** while `condition` remains `true`.
* **`<main.render, render_pipeline>`**: Binds frame rendering to the named `<gpu.main_render_pipline>`.
* **`<main.cpu_gpu, shader>`**: CPU↔GPU synchronization and dispatch (`hlwlRunGPU(...)`) for a compute or material shader.

### GPU Pipelines & Memory (`<gpu.*>`)
Uses Apple Metal-style shader entry points (`vert`, `frag`, `comp`) lowered to `shaders.wgsl`:
* **`<gpu.main_render_pipline, name>`**: Main rasterization pass (`vert` + `frag`).
* **`<gpu.material_shader, name>`**: Reusable surface/object shader (`frag`).
* **`<gpu.post_processing_shader, name>`**: Fullscreen post-process pass (`frag`).
* **`<gpu.compute, name>`**: GPU compute kernel (`comp`).
* **`<gpu.uniform, name>`**: Auto-aligned (`std140`) uniform struct synced automatically before `hlwlRunGPU(...)`.
* **`<gpu.buffer, name, shader, script>`**: High-capacity GPU storage buffer (`std430`) linked to its target `shader` and owning `script`.

### Side & Extras
* **`<helpers>`** & **`<code>`**: Standalone utility functions, custom structs, and modular code.
* **`<layout>`**: HLWL code defining the render viewport and HUD elements (if omitted, the render canvas fills 100% of the window).
* **`<right_click_options>`**: Custom context-menu actions replacing the browser default right-click menu.

---

## 2. Syntax, Namespaces & Naming Conventions

### 4-Quadrant Mutability & Type Inference

| Mode | Mutable (Default) | Immutable / Pure (`c`-Prefix) |
| :--- | :--- | :--- |
| **Type-Stated** | `float val = 5;`<br>`float Function()` | `cfloat val = 5;`<br>`cfloat Function()` |
| **Type-Inferred** | `var val = 5f;`<br>`fn Function()` | `cvar val = 5f;`<br>`cfn Function()` |

### Three-Tier Namespace Resolution
1. **Unscoped (Selective Import):** `CreateUIComponent(loc, dim, type, text)`
2. **Scoped (Package Import):** `(easyGraphics) DrawGrid()`
3. **Hidden / Built-In Core (`::`):** `std::float`, `ui::BUTTON`, `gpu::CreateBuffer(...)`, `io::WriteToConsole(...)` *(always available without `<include>`)*

### Official Style Guide
* **Functions:** `PascalCase` (`Add()`, `ComputePulse()`)
* **Member Functions:** `_PascalCase` (`_UpdatePosition()`)
* **Types & Structs:** `PascalCase` (`Vector3`, `ui::Component`)
* **Local Variables:** `camelCase` (`frameTime`, `buttonComponent`)
* **Member Variables:** `_camelCase` (`_moveSpeed`)
* **Global Variables:** `SCREAMING_SNAKE_CASE` (`PARTICLE_COUNT`)
* **Namespaces:** `camelCase` (`std`, `ui`, `gpu`, `io`, `easyGraphics`)
* **Directives (`#...`):** `flatcase` (`#onlyrun`, `#exclrunend`)
* **Component Headers:** `flatcase.snake_case` (`<main.if_func_once>`, `<gpu.post_processing_shader>`)
* **Brackets / Braces:** **Allman style** (opening `{` on its own aligned newline)

---

## 3. Quick-Test Example

```hlwl
<file, name = "Example", interpret = true>
<config>
    "browserTargets": ["Chrome", "Firefox", "Safari"]
</config>
<main.func>
    cint Add()
    {
        io::WriteToConsole(1 + 2);
        return 1;
    }
</main.func>
</file>
```

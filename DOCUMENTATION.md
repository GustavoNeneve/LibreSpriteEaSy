# LibreSprite EaSy — Complete Documentation

> **LibreSprite EaSy** is a free and open-source animated sprite editor and pixel art tool, forked from the last GPLv2 release of Aseprite.  
> This document covers every feature from the smallest utility to the main editing pipeline, along with the logic behind each one.

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Architecture & Module Layers](#2-architecture--module-layers)
3. [Core Concepts](#3-core-concepts)
   - [Sprite](#31-sprite)
   - [Layers](#32-layers)
   - [Frames & Animation](#33-frames--animation)
   - [Cels](#34-cels)
   - [Palette & Color Modes](#35-palette--color-modes)
4. [User Interface](#4-user-interface)
   - [Main Window](#41-main-window)
   - [Toolbox](#42-toolbox)
   - [Timeline](#43-timeline)
   - [Color Selector & Palette Editor](#44-color-selector--palette-editor)
   - [Preview Window](#45-preview-window)
   - [Onion Skinning Panel](#46-onion-skinning-panel)
5. [Drawing Tools](#5-drawing-tools)
   - [Ink System](#51-ink-system)
   - [Pencil / Freehand](#52-pencil--freehand)
   - [Eraser](#53-eraser)
   - [Spray / Airbrush](#54-spray--airbrush)
   - [Paint Bucket / Flood Fill](#55-paint-bucket--flood-fill)
   - [Selection Tools](#56-selection-tools)
   - [Shape Tools](#57-shape-tools)
   - [Eyedropper](#58-eyedropper)
   - [Hand / Scroll](#59-hand--scroll)
   - [Zoom](#510-zoom)
   - [Contour Tools](#511-contour-tools)
   - [Symmetry Mode](#512-symmetry-mode)
   - [Tiled Mode](#513-tiled-mode)
   - [Pixel Perfect Mode](#514-pixel-perfect-mode)
   - [Shading Ink](#515-shading-ink)
   - [Blur & Jumble](#516-blur--jumble)
6. [Commands & Operations](#6-commands--operations)
   - [File Operations](#61-file-operations)
   - [Edit Operations](#62-edit-operations)
   - [Sprite Operations](#63-sprite-operations)
   - [Layer Operations](#64-layer-operations)
   - [Frame & Tag Operations](#65-frame--tag-operations)
   - [Selection Operations](#66-selection-operations)
   - [View & Navigation](#67-view--navigation)
   - [Palette Operations](#68-palette-operations)
   - [Scripting Operations](#69-scripting-operations)
   - [Miscellaneous](#610-miscellaneous)
7. [Undo / Redo System](#7-undo--redo-system)
8. [File Formats](#8-file-formats)
9. [Filters & Effects](#9-filters--effects)
10. [Scripting API (JavaScript)](#10-scripting-api-javascript)
11. [Theming System](#11-theming-system)
12. [Internationalization (i18n)](#12-internationalization-i18n)
13. [Configuration & Preferences](#13-configuration--preferences)
14. [Crash Recovery](#14-crash-recovery)
15. [Platform Abstraction Layer (SHE)](#15-platform-abstraction-layer-she)
16. [Build System](#16-build-system)
17. [Contributing](#17-contributing)
18. [Security](#18-security)
19. [Third-Party Libraries](#19-third-party-libraries)

---

## 1. Project Overview

**LibreSprite** originated as a fork of [Aseprite](https://www.aseprite.org) (by David Capello). When Aseprite moved from GPLv2 to a proprietary license on August 26th, 2016, the community forked it at the [last GPL-covered commit](https://github.com/aseprite/aseprite/commit/03be4aa23db465219962f4c62410f628e7392545) and continued developing it independently.

**LibreSprite EaSy** builds on LibreSprite with additional bug fixes (e.g., the `ui::Splitter` widget paint-event fix) and translation updates.

### Key Features at a Glance

| Feature | Description |
|---|---|
| Real-time animation preview | Animated frames are rendered live as you paint |
| Onion skinning | Transparent ghost of surrounding frames helps drawing smooth motion |
| Multi-sprite editing | Multiple sprites can be open and edited simultaneously |
| Layers & frames | Non-destructive layer stack combined with a timeline of frames |
| Tiled drawing mode | Canvas wraps seamlessly for pattern/texture creation |
| Pixel-precise tools | Pixel-perfect stroke algorithm, polygon, filled contour, shading |
| Scripting | Automate tasks with JavaScript (Duktape / QuickJS / V8 engine) |
| Themeable UI | Swap entire color scheme and widget graphics via skin packs |
| Cross-platform | Linux, Windows, macOS, Android, WebAssembly |
| Many file formats | Native `.ase`/`.aseprite`, PNG, GIF, JPG, BMP, WebP, FLI/FLC, ICO, … |
| 30+ built-in palettes | Classic hardware palettes (NES, C64, GameBoy, CGA, Pico-8, …) |
| 14 UI languages | English, Spanish, French, German, Portuguese, Italian, Korean, Chinese, Japanese, Arabic, Polish, Russian, Indonesian, Hindi |

---

## 2. Architecture & Module Layers

The source code lives inside `src/` and is organized into five dependency layers. Each layer may only depend on layers below it.

```
Layer 5 ── main/           Entry point (main.cpp)
Layer 4 ── app/            Application logic (705 files)
Layer 3 ── filters/        Image effects
           render/         Sprite rendering engine
           ui/             Portable GUI widget library
Layer 2 ── doc/            Document model (sprites, layers, images)
           she/            Platform abstraction (SDL2 / GTK / Win / OSX)
Layer 1 ── cfg/            INI file loading/saving
           gen/            XML-to-C++ code generator
Layer 0 ── base/           Core utilities (threads, UTF-8, SHA1, FS, …)
           clip/           Clipboard (copy/paste text & images)
           css/            CSS-like style sheet parser
           fixmath/        Fixed-point arithmetic
           flic/           FLI/FLC animation format I/O
           gfx/            Graphics primitives (Point, Rect, Color, Region)
           script/         JavaScript engine wrapper (Duktape/V8/QuickJS)
           undo/           Generic undo/redo history
           wacom/          Wacom tablet (Wintab) API definitions
```

### Why layering matters

Each layer has a well-defined contract. For instance, `doc/` knows nothing about the screen; it only defines the data structures. `render/` knows how to draw those structures but does not manage windows. `ui/` does not know about sprites. This separation makes each module independently testable and reusable.

---

## 3. Core Concepts

### 3.1 Sprite

A **sprite** is the root document object. It owns:
- a canvas size (width × height in pixels)
- a **color mode** (RGBA, Grayscale, or Indexed)
- an ordered list of **layers**
- an ordered list of **frames** (each frame has a duration in milliseconds)
- a set of **palettes** (one palette can be active per frame)
- a filename and dirty/saved state

**Logic:** When the user opens or creates a file, `app/file/` creates a `doc::Sprite` object in memory. All editing commands operate on this in-memory object via transactions, and saving serializes it back to disk.

### 3.2 Layers

A **layer** is a named, stackable drawing surface. Layers support:

| Property | Effect |
|---|---|
| `visible` | Whether the layer is composited into the final image |
| `editable` (locked) | Prevents any changes when locked |
| `background` | Fixed to the bottom of the stack; cannot be moved or made transparent |
| `transparent` | Non-background layers that support alpha |
| `continuous` | When true, cels are preferred to be linked when copied |
| `folder` (group layer) | Contains child layers; used for organizing |

**Logic:** Layers are stored as a doubly-linked list inside the sprite. The renderer iterates from the bottom layer to the top, compositing each visible layer's cel image for the current frame.

### 3.3 Frames & Animation

A **frame** is a time slice. Each frame has:
- an index (0-based)
- a duration in milliseconds

Frames are played back sequentially to produce animation. The `cmd_play_animation` command drives a timer that advances the current frame index at the rate defined by each frame's duration.

**Frame tags** are named ranges of frames with a play mode:
- **Forward** — play from first to last
- **Backward** — play from last to first
- **Ping-pong** — play forward, then backward, repeat

### 3.4 Cels

A **cel** is the intersection of a layer and a frame. It holds:
- a reference to an `Image` object
- an (x, y) offset within the canvas
- an opacity (0–255)

Cels can be **linked**: two cels in the same layer at different frames share the same `Image` object. Painting on one automatically paints on the other, which is useful for static elements that don't change between frames.

**Logic:** The command infrastructure tracks which cels are linked via `cmd_link_cels` and `cmd_unlink_cel`. Internally, a linked cel simply stores a pointer to the same `doc::Image` as another cel.

### 3.5 Palette & Color Modes

LibreSprite supports three color modes:

| Mode | Description |
|---|---|
| **RGBA** | 32-bit full color (8 bits per channel: red, green, blue, alpha) |
| **Grayscale** | 16-bit (8-bit luminance + 8-bit alpha) |
| **Indexed** | 8-bit index into a 256-entry color palette |

A **palette** is an array of up to 256 RGBA entries. In RGBA/Grayscale mode the palette is still present (for reference), but pixels are stored as direct color values.

**Included palettes:** The `data/palettes/` folder ships with 30+ classic hardware and community palettes:
`arne16`, `arne32`, `db16`, `db32`, `nes`, `gameboy`, `commodore64`, `cga`, `pico-8`, `monokai`, `solarized`, `vga-13h`, `web-safe-colors`, and more.

---

## 4. User Interface

The UI is built entirely in the custom `ui/` library — a lightweight, skinnable widget toolkit that draws using SDL2 and themes defined via XML + PNG skin sheets.

### 4.1 Main Window

The main window is divided into regions:

- **Menu bar** — File, Edit, Sprite, Layer, Frame, Select, View, Help
- **Main toolbar** — Quick access to open, save, undo, redo, zoom, and palette
- **Side toolbox** — All drawing tools organized in tool groups
- **Canvas area** — Central editing area; supports zoom, panning, multiple tabs
- **Status bar** — Current tool, coordinates, zoom level, color info, frame info
- **Timeline panel** — Layer + frame grid (collapsible)
- **Color bar** — Foreground and background color swatches

### 4.2 Toolbox

The toolbox groups tools into categories. Each tool supports two buttons (left-click and right-click), each independently configurable with an **ink** (painting mode).

Tool groups:
1. Selection tools (marquee, lasso, …)
2. Pencil / Brush
3. Eraser
4. Fill / Flood-fill
5. Eyedropper / Color picker
6. Shape tools (rectangle, circle, line, …)
7. Hand / Scroll
8. Zoom

### 4.3 Timeline

The **timeline** shows a 2D grid of **layers** (rows) × **frames** (columns). Each cell represents a cel.

**Logic:**
- Clicking a cell selects that layer + frame, making it the active editing context.
- Dragging frames reorders them.
- Ctrl+click selects multiple cels for bulk operations.
- Frame tags appear as colored bands across the top of the timeline.
- Onion skin range is shown visually in the timeline header.

### 4.4 Color Selector & Palette Editor

The **color selector** panel lets the user set foreground and background colors using:
- HSV color wheel
- RGB sliders
- Hex input
- Palette entry click

The **palette editor** (`cmd_palette_editor`) shows all palette entries as swatches. The user can:
- Click to select an entry and edit it
- Add / remove entries
- Drag to reorder
- Load or save `.gpl` / `.pal` files
- Run color-quantization to reduce a full-color image to an indexed palette

### 4.5 Preview Window

Opened via `cmd_toggle_preview`, this is a floating window that shows the full canvas at a small zoom level, updating in real-time as you paint. Useful for checking the overall appearance without zooming out the main canvas.

### 4.6 Onion Skinning Panel

Onion skinning overlays ghost images of adjacent frames over the current frame:
- **Previous frames** appear in a configurable tint (default: red)
- **Next frames** appear in a different tint (default: blue)
- The number of visible frames before/after and their opacity are configurable
- Toggled via `cmd_onionskin`

---

## 5. Drawing Tools

Every tool is composed of four orthogonal concepts:

| Concept | What it defines |
|---|---|
| **Controller** | How mouse movement translates to a set of points (freehand, line, rectangle, …) |
| **PointShape** | What is drawn at each point (pixel, brush, flood-fill, spray) |
| **Intertwiner** | How points are connected between mouse events (lines, pixel-perfect) |
| **Ink** | What action is performed when a point is painted (paint, erase, pick color, …) |

This design means new tools can be created by mixing existing components.

### 5.1 Ink System

An **ink** defines the painting action. Available inks:

| Ink | Behavior |
|---|---|
| `paint` | Writes the foreground color onto the canvas |
| `paint_fg` | Always uses foreground regardless of mouse button |
| `paint_bg` | Always uses background |
| `paint_copy` | Copies source pixels unchanged |
| `paint_lock_alpha` | Paints color but preserves original alpha |
| `shading` | Cycles through the palette (shading mode) |
| `eraser` | Writes transparency (or background color on BG layers) |
| `replace_fg_with_bg` | Replaces all pixels matching foreground color with background |
| `replace_bg_with_fg` | The reverse replacement |
| `pick_fg` | Sets foreground from canvas pixel |
| `pick_bg` | Sets background from canvas pixel |
| `zoom` | Zooms in/out at the cursor |
| `scroll` | Pans the canvas |
| `move` | Moves the active cel |
| `blur` | Blurs pixels under the brush |
| `jumble` | Smears / warps pixels |

### 5.2 Pencil / Freehand

The **pencil** tool (`id: pencil`) draws individual pixels or brush shapes along the mouse path. 

**Logic:** The `FreehandController` records every mouse-move event as a point in a stroke. The `Intertwine` component connects consecutive points using Bresenham's line algorithm (or pixel-perfect variant). At each point the selected `PointShape` (pixel or brush) applies the active ink.

The **brush** size, shape, and angle are configurable. Custom brushes can be created with `cmd_new_brush`.

### 5.3 Eraser

The **eraser** uses the `EraserInk`. On RGBA layers it writes `rgba(0,0,0,0)` (transparent). On background (non-transparent) layers it writes the background color instead of clearing.

### 5.4 Spray / Airbrush

The **spray** tool scatters paint randomly within a circular radius. 

**Logic:** It uses the `SprayPointShape`, which generates random (x,y) offsets within a circle of configurable radius on every timer tick, regardless of mouse movement. The spray speed and radius are user-configurable.

### 5.5 Paint Bucket / Flood Fill

**Logic:** The flood-fill algorithm (in `doc/algorithm/floodfill`) starts at the clicked pixel, finds all connected pixels with the same color within a configurable tolerance, and replaces them with the current ink color. It supports 4-connectivity and 8-connectivity.

Fill can also operate on selection masks instead of paint.

### 5.6 Selection Tools

Selection tools use the `SelectionInk` and control which area of the canvas subsequent operations affect.

| Tool | Controller |
|---|---|
| **Rectangular Marquee** | `RectangleController` |
| **Ellipse Marquee** | `EllipseController` |
| **Lasso** | `FreehandController` |
| **Polygonal Lasso** | `PointByPointController` |

Selection operations (additive, subtractive, intersect) are controlled by modifier keys.

A **selection mask** is stored as a 1-bit-per-pixel bitmap. Commands such as cut, copy, transform, fill, and effects all respect the active mask.

### 5.7 Shape Tools

Shape tools draw geometric outlines or fills:

| Tool | Notes |
|---|---|
| **Line** | Draws a straight line; holding Shift snaps to 45° angles |
| **Rectangle** | Hollow or filled; holding Shift makes it a square |
| **Ellipse** | Hollow or filled; holding Shift makes it a circle |
| **Contour** | Traces the exact pixel outline of the current selection |
| **Polygon** | Click to add vertices, double-click to close |
| **Bezier Curve** | Click to place control points |

**Logic:** Shape controllers compute the intermediate points (e.g., `doc::algo_ellipse` for ellipses, `doc::algorithm::polygon` for polygons). The intertwiner connects them, and the ink applies paint.

### 5.8 Eyedropper

Clicking with the eyedropper samples the color at the cursor position and sets it as the foreground color. Right-clicking sets the background color.

**Logic:** `cmd_eyedropper` reads the pixel value from the composited (rendered) canvas at the cursor position, converts it to RGBA, and updates the color bar.

### 5.9 Hand / Scroll

The **hand** tool (`scroll` ink) pans the canvas viewport when dragged, without modifying any pixels. Useful when zoomed in.

### 5.10 Zoom

The **zoom** tool (`zoom` ink) zooms in on click and zooms out on right-click. The zoom levels jump through fixed steps (100%, 200%, 400%, 800%, …). `cmd_zoom` handles programmatic zoom; the tool itself works interactively.

### 5.11 Contour Tools

**Filled contour** traces the border of the current selection or a region and fills it, giving a precise pixel-art outline. The algorithm follows the edge pixels of the active mask and applies ink on the boundary.

### 5.12 Symmetry Mode

Toggled by `cmd_symmetry_mode`. When enabled, every paint stroke is mirrored horizontally and/or vertically across user-defined symmetry axes. 

**Logic:** The `Symmetry` object in `src/app/tools/symmetry.h` computes mirror points for each point in the stroke and adds them to the set of points to be painted.

### 5.13 Tiled Mode

Toggled by `cmd_tiled_mode`. When active, the canvas wraps: pixels painted near the right edge appear on the left edge, and vice versa. This makes it easy to create seamless textures and patterns.

**Logic:** The tile wrapping is applied in the tool-loop before the ink writes the pixel: coordinates are taken modulo `(sprite_width, sprite_height)`.

### 5.14 Pixel Perfect Mode

Toggled by `cmd_pixel_perfect_mode`. Removes diagonal "staircase" artifacts from freehand strokes by removing intermediate pixels that would create an L-shaped kink.

**Logic:** The `AsPixelPerfectIntertwiner` removes a pixel P(n-1) from the stroke when the pixel P(n-2) and P(n) are diagonally adjacent to it, following the algorithm by Sébastien Bénard and Carduus.

### 5.15 Shading Ink

The **shading ink** does not paint a flat color. Instead, for each pixel touched it finds the pixel's current color in the active palette and replaces it with the next or previous entry in a user-defined shading range. This produces smooth lighting effects without changing the base hue.

### 5.16 Blur & Jumble

- **Blur** (`blur` ink): for each pixel under the brush, computes an average color from surrounding pixels.
- **Jumble** (`jumble` ink): randomly swaps pixels within the brush area, creating a smear/warp effect.

---

## 6. Commands & Operations

All user actions are implemented as `Command` objects in `src/app/commands/`. Commands support parameters, are undo-able (they use the transaction/cmd infrastructure), and can be triggered from menus, keyboard shortcuts, or scripts.

### 6.1 File Operations

| Command | Logic |
|---|---|
| `cmd_new_file` | Creates a new `doc::Sprite` with user-specified dimensions and color mode; registers it as a document |
| `cmd_open_file` | Shows a file dialog; calls the appropriate format loader (via `app/file/` format handlers) to decode the file into a `doc::Sprite`; adds it to the document list |
| `cmd_save_file` | Serializes the current sprite via the active format handler; updates the saved state |
| `cmd_save_file` (as) | Same but prompts for a new filename first |
| `cmd_close_file` | Removes the document from the open list; prompts to save if dirty |
| `cmd_exit` | Closes all documents and exits the application |
| `cmd_export_sprite_sheet` | Flattens all frames/layers into a single image arranged as a grid or strip; saves the result and optionally a JSON data file |
| `cmd_import_sprite_sheet` | Reads a sprite sheet image and splits it into individual frames/layers |
| `cmd_repeat_last_export` | Re-runs the previous export with the same settings |
| `cmd_open_in_folder` | Opens the OS file manager at the directory containing the current sprite |
| `cmd_open_with_app` | Opens the current file with its default OS application |
| `cmd_share` | Platform-specific share action (mobile/web builds) |

### 6.2 Edit Operations

| Command | Logic |
|---|---|
| `cmd_undo` | Pops the last transaction from the undo stack and reverses all its commands |
| `cmd_undo_history` | Shows a panel listing all undo steps; user can jump to any point |
| `cmd_cut` | Copies selected pixels to clipboard, then clears them on the canvas |
| `cmd_copy` | Copies the selected region (or full cel) to the clipboard |
| `cmd_copy_merged` | Composites all visible layers for the current frame, then copies that merged image |
| `cmd_paste` | Pastes clipboard contents as a floating cel at the canvas center |
| `cmd_paste_text` | Renders a text string as pixels and pastes it |
| `cmd_clear` | Fills the selected region (or full cel) with transparent pixels |
| `cmd_clear_cel` | Clears the active cel completely |
| `cmd_flip` | Flips the selection or entire sprite horizontally or vertically |
| `cmd_rotate` | Rotates the selection or sprite by 90°, 180°, 270°, or a custom angle using the RotSprite algorithm |
| `cmd_crop` | Resizes the canvas to the selection bounds |
| `cmd_canvas_size` | Resizes the canvas with configurable anchor point and padding |
| `cmd_sprite_size` | Scales the entire sprite (all frames, all layers) to new dimensions |
| `cmd_switch_colors` | Swaps the foreground and background colors |

### 6.3 Sprite Operations

| Command | Logic |
|---|---|
| `cmd_sprite_properties` | Shows a dialog with sprite metadata: filename, dimensions, color mode, frame count |
| `cmd_change_pixel_format` | Converts between RGBA, Grayscale, and Indexed modes; runs color quantization when converting to Indexed |
| `cmd_color_quantization` | Analyzes all pixels and generates an optimal palette for Indexed mode |
| `cmd_duplicate_sprite` | Creates an in-memory copy of the current sprite and opens it as a new document |
| `cmd_duplicate_view` | Opens another editor tab for the same sprite, allowing different zoom/position |
| `cmd_background_from_layer` | Converts the selected transparent layer into the background layer |
| `cmd_layer_from_background` | Converts the background layer into a transparent layer |
| `cmd_flatten_layers` | Merges all visible layers into a single layer |
| `cmd_new_sprite_from_selection` | Creates a new sprite from the currently selected region |

### 6.4 Layer Operations

| Command | Logic |
|---|---|
| `cmd_new_layer` | Adds a new transparent layer above the active layer |
| `cmd_new_layer_set` | Adds a new layer group (folder) |
| `cmd_remove_layer` | Deletes the active layer and all its cels |
| `cmd_duplicate_layer` | Creates a copy of the active layer with all its cels |
| `cmd_merge_down_layer` | Merges the active layer onto the layer directly below it |
| `cmd_layer_properties` | Renames the layer and changes its blend mode / opacity |
| `cmd_layer_visibility` | Toggles the visibility of one or more layers |
| `cmd_goto_layer` | Moves the active layer selection up or down |
| `cmd_move_cel` | Repositions the active cel by a delta (or interactively) |
| `cmd_copy_cel` | Duplicates the active cel to another frame on the same layer |
| `cmd_link_cels` | Makes selected cels share the same image data |
| `cmd_unlink_cel` | Breaks the link so the cel gets its own independent copy |
| `cmd_cel_properties` | Opens a panel showing the active cel's opacity (0–255) and user data (arbitrary string/color annotation); changes are committed via a debounced timer |

### 6.5 Frame & Tag Operations

| Command | Logic |
|---|---|
| `cmd_new_frame` | Inserts a new blank frame after the active frame |
| `cmd_remove_frame` | Deletes the active frame |
| `cmd_frame_properties` | Changes the duration (ms) of the active frame |
| `cmd_new_frame_tag` | Creates a named tag spanning a range of frames |
| `cmd_frame_tag_properties` | Renames a tag or changes its range and loop mode |
| `cmd_remove_frame_tag` | Deletes a frame tag |
| `cmd_goto_frame` | Jumps to a specific frame number |
| `cmd_play_animation` | Starts / stops the animation playback loop |
| `cmd_reverse_frames` | Reverses the order of frames in the current tag or entire animation |
| `cmd_set_loop_section` | Defines which frames repeat during playback |

### 6.6 Selection Operations

| Command | Logic |
|---|---|
| `cmd_mask_all` | Selects the entire canvas |
| `cmd_deselect_mask` | Clears the selection mask |
| `cmd_reselect_mask` | Restores the previously cleared selection |
| `cmd_invert_mask` | Inverts the selection (unselected becomes selected) |
| `cmd_mask_by_color` | Selects all pixels within a color tolerance of the clicked color |
| `cmd_mask_content` | Selects the non-transparent pixels of the active cel |
| `cmd_move_mask` | Moves the selection outline without moving the pixels |
| `cmd_modify_selection` | Expands, contracts, or feathers the selection boundary |
| `cmd_select_tile` | In tiled mode, selects the repeating tile region |
| `cmd_load_mask` | Loads a selection from a `.msk` file |
| `cmd_save_mask` | Saves the current selection to a `.msk` file |

### 6.7 View & Navigation

| Command | Logic |
|---|---|
| `cmd_zoom` | Zooms in, out, or to a specific level |
| `cmd_scroll` | Pans the canvas by a delta |
| `cmd_scroll_center` | Centers the canvas in the viewport |
| `cmd_toggle_fullscreen` | Toggles borderless fullscreen mode |
| `cmd_fullscreen_preview` | Shows a full-screen animation preview with black background |
| `cmd_toggle_preview` | Shows/hides the floating preview window |
| `cmd_grid` | Shows/hides the pixel grid overlay; configures grid size |
| `cmd_onionskin` | Toggles onion skinning and opens its settings panel |
| `cmd_timeline` | Shows/hides the timeline panel |
| `cmd_show` | Toggles visibility of various UI panels (toolbox, palette, timeline, etc.) |
| `cmd_goto_tab` | Switches to a specific open document tab |
| `cmd_home` | Switches to the home/start screen |
| `cmd_alternate_timeline` | Switches the timeline between normal and compact views |
| `cmd_alternate_toolbar` | Toggles the main toolbar |
| `cmd_alternate_touchbar` | Toggles the touch-optimized toolbar (tablet/Android) |
| `cmd_toggle_touchbar` | Shows/hides the touchbar |
| `cmd_advanced_mode` | Hides all UI panels except the canvas for maximum workspace |

### 6.8 Palette Operations

| Command | Logic |
|---|---|
| `cmd_palette_editor` | Opens the full palette editor panel |
| `cmd_palette_size` | Changes the number of entries in the palette |
| `cmd_set_palette` | Applies a palette from the built-in palette list |
| `cmd_set_palette_entry_size` | Changes the visual size of swatches in the palette bar |
| `cmd_load_palette` | Loads a `.gpl`/`.pal` file from disk |
| `cmd_save_palette` | Saves the current palette to a `.gpl`/`.pal` file |
| `cmd_add_color` | Adds the current foreground color to the palette |
| `cmd_change_color` | Edits a specific palette entry |
| `cmd_set_color_selector` | Switches the color model shown in the color selector (HSV, RGB, Wheel, …) |
| `cmd_set_same_ink` | Makes left and right tool buttons share the same ink |
| `cmd_set_ink_type` | Changes the ink of the active tool button |

### 6.9 Scripting Operations

| Command | Logic |
|---|---|
| `cmd_run_script` | Executes a selected `.js` script file via the active JavaScript engine |
| `cmd_install_script` | Copies a script file into the user's scripts directory and registers it |
| `cmd_rescan_scripts` | Rescans the scripts directory and rebuilds the Scripts menu |
| `cmd_open_scripts_folder` | Opens the OS file manager at the scripts directory |
| `cmd_developer_console` | Opens an interactive JavaScript REPL console |

### 6.10 Miscellaneous

| Command | Logic |
|---|---|
| `cmd_about` | Shows the About dialog with version and credits |
| `cmd_options` | Opens the Preferences dialog |
| `cmd_keyboard_shortcuts` | Shows/edits the keyboard shortcut assignments |
| `cmd_cancel` | Cancels the current pending operation |
| `cmd_change_brush` | Selects a different brush preset |
| `cmd_new_brush` | Creates a custom brush from the current selection |
| `cmd_discard_brush` | Removes the custom brush and reverts to the default |
| `cmd_pixel_perfect_mode` | Toggles pixel-perfect stroke mode |
| `cmd_symmetry_mode` | Toggles symmetry drawing mode |
| `cmd_tiled_mode` | Toggles tiled/wrapping canvas mode |
| `cmd_refresh` | Reloads the current sprite from disk, discarding unsaved changes |
| `cmd_launch` | Launches an external URL (used for community links in the Help menu) |

---

## 7. Undo / Redo System

The undo system is implemented in `src/undo/` (a generic library) and used by `app/` via transactions.

**Architecture:**

```
User action
  └─▶ Transaction (opened by the Command)
        ├─▶ Cmd_1 (e.g., SetLayerName)
        ├─▶ Cmd_2 (e.g., ModifyImage)
        └─▶ Commit / Rollback
               └─▶ UndoHistory stores the transaction
```

- A **transaction** is a group of low-level `Cmd` objects that together represent one user-visible action.
- Each `Cmd` implements `execute()` and `undo()`.
- The `UndoHistory` is a linear stack. `Undo` pops the top transaction and calls `undo()` on each of its commands in reverse order. `Redo` re-executes them.
- Smart pointers to `doc::Image` objects are intentionally avoided inside `Cmd` objects to prevent conflicts when multiple `Cmd` trees coexist in the undo history (see `src/app/cmd/README.md`).

---

## 8. File Formats

Format handlers are registered in `src/app/file/`. Each handler implements a load and/or save function.

| Format | Extensions | Notes |
|---|---|---|
| **Aseprite** | `.ase`, `.aseprite` | Native format; preserves layers, frames, tags, palettes, cel positions |
| **PNG** | `.png` | Single frame export/import; supports RGBA |
| **GIF** | `.gif` | Animated export (indexed color, limited to 256 colors per frame) |
| **JPEG** | `.jpg`, `.jpeg` | Lossy; no alpha; flattens layers |
| **BMP** | `.bmp` | Basic bitmap |
| **WebP** | `.webp` | Optional; requires `WITH_WEBP_SUPPORT` build flag |
| **TARGA** | `.tga` | Targa image |
| **ICO** | `.ico` | Windows icon format |
| **FLIC** | `.fli`, `.flc` | Autodesk Animator format; animated, indexed color |
| **FLC** | `.flc` | Larger FLI variant |
| **PCX** | `.pcx` | Legacy PC Paintbrush format |
| **MSK** | `.msk` | Selection mask (load/save selection outlines) |
| **PIC** | `.pic` | Ludic / Animator Pro format |
| **PICPRO** | `.pic` | Animator Pro format variant |

File format specifications are documented in `docs/files/`.

---

## 9. Filters & Effects

Filters are in `src/filters/`. They process a `doc::Image` (or a masked region of it) and modify pixel values.

| Filter | Logic |
|---|---|
| **Color Curve** | Applies a piecewise-linear tone curve to R, G, B, or A channels independently (`color_curve_filter`). Control points define the input→output mapping; values between points are linearly interpolated |
| **Invert Color** | Replaces each channel `c` with `255 - c` (`invert_color_filter`) |
| **Replace Color** | Replaces all pixels whose color is within a configurable tolerance of a source color, setting them to a target color (`replace_color_filter`) |
| **Despeckle (Median)** | Applies a 3×3 median filter: each pixel is replaced by the median of its neighborhood, reducing isolated noise pixels (`median_filter`) |
| **Convolution Matrix** | Applies a user-defined N×N kernel to each pixel for blur, sharpen, emboss, edge-detect, and other linear effects (`convolution_matrix_filter`) |
| **Color Quantization** | Analyzes all pixels across selected frames and generates an optimized palette for Indexed mode; used internally by `cmd_color_quantization` |

All filters can be applied to the whole canvas, to the selection mask only, or to a range of frames, giving batch processing capability.

---

## 10. Scripting API (JavaScript)

LibreSprite EaSy embeds a JavaScript engine (Duktape by default; QuickJS and V8 are optional). Scripts live in `data/scripts/` and can be installed into the user's scripts folder.

Scripts are run via **File → Scripts** or the Developer Console.

### Key API Classes

#### `app` (global object)

| Member | Type | Description |
|---|---|---|
| `app.activeImage` | `Image` | The image of the active cel |
| `app.activeSprite` | `Sprite` | The currently focused sprite document |
| `app.activeLayer` | `Layer` | The currently selected layer |
| `app.activeCel` | `Cel` | The active cel |
| `app.pixelColor` | `pixelColor` | Utility object for color packing/unpacking |
| `app.open(filename)` | method | Opens a sprite file |
| `app.exit()` | method | Quits the application |
| `app.alert(msg)` | method | Shows a message dialog |
| `app.useTool(options)` | method | Programmatically uses a drawing tool |

#### `Sprite`

| Member | Type | Description |
|---|---|---|
| `width` | number (r/w) | Canvas width in pixels |
| `height` | number (r/w) | Canvas height in pixels |
| `filename` | string (r) | Path to the file |
| `colorMode` | number (r) | Color mode constant |
| `layerCount` | number (r) | Number of layers |
| `palette` | `Palette` (r) | Active palette |
| `save()` | method | Saves to the current filename |
| `saveAs(filename, asCopy)` | method | Saves with a new filename |
| `resize(w, h)` | method | Scales the sprite |
| `crop(x, y, w, h)` | method | Crops the canvas |
| `loadPalette(filename)` | method | Loads a palette file |
| `commit()` | method | Commits the current scripting transaction |
| `layer(n)` | method | Returns the nth `Layer` |

#### `Layer`

| Member | Type | Description |
|---|---|---|
| `name` | string (r/w) | Layer name |
| `isVisible` | bool (r/w) | Show/hide the layer |
| `isEditable` | bool (r/w) | Lock/unlock the layer |
| `isBackground` | bool (r) | True if this is the background layer |
| `isImage` | bool (r) | True for image layers, false for folders |
| `celCount` | number (r) | Number of cels in this layer |
| `cel(n)` | method | Returns the nth `Cel` |

#### `Image`

| Member | Type | Description |
|---|---|---|
| `width` | number (r) | Width in pixels |
| `height` | number (r) | Height in pixels |
| `format` | number (r) | Pixel format constant |
| `stride` | number (r) | Bytes per row |
| `getPixel(x, y)` | method | Returns 32-bit RGBA color at (x, y) |
| `putPixel(x, y, color)` | method | Writes a 32-bit RGBA color |
| `getImageData()` | method | Returns all pixels as `Uint8Array` |
| `putImageData(data)` | method | Writes all pixels from a `Uint8Array` |

#### `pixelColor` utility

```js
app.pixelColor.rgba(r, g, b, a)   // Pack RGBA → 32-bit int
app.pixelColor.rgbaR(color)       // Extract red
app.pixelColor.rgbaG(color)       // Extract green
app.pixelColor.rgbaB(color)       // Extract blue
app.pixelColor.rgbaA(color)       // Extract alpha
app.pixelColor.graya(gray, alpha) // Pack grayscale+alpha
app.pixelColor.grayaV(color)      // Extract luminance
app.pixelColor.grayaA(color)      // Extract alpha from grayscale
```

#### `Cel`, `Palette`, `Frame`, `Tag` and more

Additional classes documented fully in [SCRIPTING.md](SCRIPTING.md).

### Included Example Scripts

| Script | What it does |
|---|---|
| `Random.js` | Fills every pixel of the active image with a random greyscale color using `Math.random()` |
| `white_to_alpha.js` | Converts white pixels to transparent by computing luminance `(R+G+B)/3` and setting `alpha = 255 - luminance` |

### Script Widgets (UI)

Scripts can show custom dialogs by constructing widget objects:

| Widget class | Description |
|---|---|
| `LabelWidget` | Static text label |
| `IntentryWidget` | Integer input field with min/max validation |
| `PalettelistboxWidget` | Scrollable list of palettes with an "Add" button |
| `ButtonWidget` | Clickable button |
| `ComboboxWidget` | Dropdown selector |
| … | Many more via `app.createWidget(type)` |

---

## 11. Theming System

Themes are defined in `data/skins/` as directories containing:
- `skin.xml` — widget layout and metric definitions
- `skin.png` — the spritesheet of widget graphics
- Additional PNG resources for icons

### Built-in Themes

| Theme | Description |
|---|---|
| `default` | Modern dark interface |
| `classic` | Lighter, more traditional look |

### Installing a Community Theme

1. Download the theme `.zip` from [libresprite.github.io/resources](https://libresprite.github.io/#!/resources)
2. Extract and copy the folder into the `data/skins/` directory beside `libresprite` (or the installed application data path)
3. Open LibreSprite → **Edit → Preferences → Theme**
4. Select the new theme in the list and click **Select**
5. Restart LibreSprite

### Uninstalling a Theme

Go back to **Preferences → Theme**, select a different theme, click **Select**, restart. Then you may delete the old theme folder.

> **Warning:** Do not delete a theme folder while it is still the active theme. If this happens, copy the folder back with the original name and relaunch.

### How the CSS System Works

The `css/` library parses a subset of CSS-like rules applied to widget types and IDs. The `skin.xml` references these rules to control padding, borders, colors, and background images. This is evaluated at startup and whenever the theme changes.

---

## 12. Internationalization (i18n)

UI strings are stored as JSON files in `data/languages/`:

| Code | Language |
|---|---|
| `en` | English |
| `es` | Spanish |
| `fr` | French |
| `de` | German |
| `pt` | Portuguese |
| `it` | Italian |
| `kr` | Korean |
| `zh` | Chinese (Simplified) |
| `jp` | Japanese |
| `ar` | Arabic |
| `pl` | Polish |
| `ru` | Russian |
| `id` | Indonesian |
| `hi` | Hindi |

**Logic:** At startup, `app/modules/i18n` loads the JSON file matching the OS locale (or the user-selected language from preferences). The `i18n("key")` function returns the localized string, defaulting to English if the key is missing.

---

## 13. Configuration & Preferences

Preferences are managed by the `pref/` module and stored in INI format via the `cfg/` library.

### Preference Categories

| Category | Examples |
|---|---|
| **General** | Language, cursor style, scroll speed, zoom with mouse wheel |
| **Editor** | Grid size/color, background checker pattern, pixel grid |
| **Theme** | Active skin name |
| **Tools** | Per-tool settings (brush size, spray radius, fill tolerance, …) |
| **Keyboard** | Custom shortcut bindings |
| **Animation** | Onion skin count, tint colors, opacity |
| **Color** | Default color spaces, fore/background color on startup |
| **Performance** | Undo stack depth, screen rendering method |

Keyboard shortcuts can be completely remapped via **Edit → Keyboard Shortcuts**. Assignments are stored per-command by `cmd_keyboard_shortcuts`.

---

## 14. Crash Recovery

The `app/crash/` module writes periodic backups of open sprites to a crash directory. If the application is terminated unexpectedly:

1. On the next launch, LibreSprite detects the crash backup files.
2. A recovery dialog lists all recovered sessions.
3. The user can restore any session, discarding others.

**Triggering a deliberate crash:** `Ctrl+Shift+Q` crashes the application intentionally (for testing the anti-crash system or generating a memory dump).

---

## 15. Platform Abstraction Layer (SHE)

The `she/` (System/Hardware/Environment) layer abstracts OS-specific services:

| Service | Description |
|---|---|
| Window creation | Creates resizable windows, sets titles, handles DPI |
| Event loop | Mouse, keyboard, touch, stylus events |
| OpenGL / software rendering | Frame buffer delivery to the screen |
| Clipboard | Integrates with `clip/` for OS clipboard access |
| File dialogs | Native file open/save dialogs (with optional GTK override) |
| Font rendering | FreeType integration for text rendering |
| Tablet / stylus | Wacom Wintab pressure-sensitivity support (Windows) |

**Backends:**
- **SDL2** (primary, all platforms)
- **GTK** (optional native file dialogs on Linux)
- **macOS** (Cocoa native integrations via `she/osx/`)
- **Windows** (Win32 integrations via `she/win/`)

---

## 16. Build System

The project uses **CMake** (4.1+) with **Ninja** as the recommended generator.

### Quick Start

```bash
# Clone with all submodules
git clone --recursive https://github.com/GustavoNeneve/LibreSpriteEaSy

cd LibreSpriteEaSy
mkdir build && cd build

# Configure (Linux / macOS)
cmake -G Ninja ..

# Build
ninja libresprite

# Install
ninja install
```

### Platform-specific Notes

| Platform | Extra steps |
|---|---|
| **Windows** | Use MSYS2/MinGW. Run `cmake -G Ninja ..` inside `mingw32.exe` |
| **macOS** | Pass `-DCMAKE_OSX_SYSROOT=…` pointing to the Xcode SDK |
| **Android** | Requires a native host build first; then open `android/` in Android Studio with the `ls-android-deps` package |
| **WebAssembly** | Use the Emscripten toolchain; see `emscripten/CMakeLists.txt` |

### Build Options (CMake flags)

| Flag | Default | Effect |
|---|---|---|
| `WITH_WEBP_SUPPORT` | OFF | Enable WebP image format |
| `WITH_GTK_FILE_DIALOG_SUPPORT` | OFF | Use native GTK file dialogs on Linux |
| `WITH_DESKTOP_INTEGRATION` | OFF | Install `.desktop` file and icons (Linux) |
| `WITH_QT_THUMBNAILER` | OFF | Build KDE/Qt thumbnail plugin |
| `ENABLE_MEMLEAK` | OFF | Enable memory-leak detection (debug builds) |
| `ENABLE_TESTS` | OFF | Build and run unit tests (gtest) |
| `USE_SDL2_BACKEND` | ON | Use SDL2 for display and input |
| `USE_V8_SANDBOX` | OFF | Use V8 JavaScript engine instead of Duktape |

### Packaging

- **Linux AppImage:** `package_linux.sh` uses `lib4bin`, `sharun`, and `appimagetool`
- **Windows installer:** `package_win.js` (Node.js script)
- **Snap:** `.snapcraft.yaml`

### CI/CD Workflows (`.github/workflows/`)

| Workflow | Trigger | Purpose |
|---|---|---|
| `cmakeLinux.yml` | push/PR | Automated Linux build |
| `cmakeWin64.yml` | push/PR | Windows 64-bit build |
| `cmakeMacOs.yml` | push/PR | macOS build |
| `cmakeAndroid.yml` | push/PR | Android build |
| `codeql-analysis.yml` | scheduled | Static security analysis |
| `submodules-CD.yml` | scheduled | Automatic submodule updates |

---

## 17. Contributing

### Quick Workflow

1. **Fork** the repository on GitHub.
2. **Clone** your fork with `git clone --recursive`.
3. **Create a feature branch:** `git checkout -b my-feature`
4. **Make changes** in small, meaningful commits.
5. **Keep up to date:** `git fetch upstream && git rebase upstream/master`
6. **Push** to your fork and **open a Pull Request**.

Full details in [CONTRIBUTING.md](CONTRIBUTING.md) and build instructions in [INSTALL.md](INSTALL.md).

### Issue Templates

Located in `.github/ISSUE_TEMPLATE/`:
- `bug_report` — for reproducible crashes or incorrect behavior
- `code_quality` — for refactoring or technical-debt proposals
- `documentation` — for documentation improvements or corrections

---

## 18. Security

The security policy is documented in [SECURITY.md](SECURITY.md).

- Supported version: **1.0** (current stable)
- To report a vulnerability, open an issue on GitHub with a full description, screenshots, and your OS/hardware specifications
- For time-sensitive issues, contact the team via [Discord](https://discord.com/invite/95gbyU5) or [Reddit](https://www.reddit.com/r/LibreSprite/)

---

## 19. Third-Party Libraries

| Library | License | Purpose |
|---|---|---|
| [SDL2](https://www.libsdl.org) | Zlib | Display, input events |
| [FreeType](https://www.freetype.org) | FTL | Font rendering |
| [libpng](http://www.libpng.org) | Libpng | PNG format |
| [libjpeg](http://libjpeg.sourceforge.net) | IJG | JPEG format |
| [giflib](http://giflib.sourceforge.net) | MIT | GIF format |
| [libwebp](https://developers.google.com/speed/webp) | BSD-3-Clause | WebP format |
| [zlib](https://www.zlib.net) | Zlib | Compression |
| [pixman](http://www.pixman.org) | MIT | Pixel compositing |
| [curl](http://curl.haxx.se/) | curl | Networking |
| [tinyxml2](https://github.com/leethomason/tinyxml2) | Zlib | XML parsing (UI skins) |
| [duktape](https://duktape.org) | MIT | JavaScript engine |
| [simpleini](https://github.com/aseprite/simpleini) | MIT | INI configuration |
| [flic](https://github.com/aseprite/flic) | MIT | FLI/FLC animation |
| [observable](https://github.com/dacap/observable) | MIT | Signal/slot system |
| [EasyTab](https://github.com/ApoorvaJ/EasyTab) | — | Tablet input |
| [modp_b64](https://github.com/nicowillis/modp_b64) | BSD | Base64 encoding |
| [quickjs](https://bellard.org/quickjs/) | MIT | Alternative JS engine |
| [qoi](https://qoiformat.org) | MIT | QOI image format |
| [gtest](https://github.com/google/googletest) | BSD-3-Clause | Unit testing |

---

*Documentation generated for LibreSprite EaSy — March 2026.*

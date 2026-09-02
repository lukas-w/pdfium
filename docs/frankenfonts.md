# Project Proposal: Frankenfonts — Hybrid Rust Font Parsing & FreeType/AGG Rasterization

**Author:** Tom Sepez (`tsepez@google.com`)
**Target Project:** PDFium (`core/fxge`, standalone, and Chromium integration)
**Status:** Proposed Architecture

---

## 1. Executive Summary

PDFium historically relies on **FreeType** for both binary font table parsing
and glyph pixel rasterization. However, the overwhelming majority of historical
vulnerabilities in font engines stem from **parsing untrusted, malformed font
tables** (such as embedded TrueType, OpenType, and CFF streams in malicious
PDFs).

**Project Frankenfonts** proposes a hybrid font architecture:
1. **Frontend (Parsing & Hinting):** Use Google's memory-safe **Fontations**
   (`read-fonts` + `skrifa`) Rust crates to parse font tables, decode CMaps,
   compute metrics, and execute TrueType VM / CFF hinting instructions in safe
   Rust.
2. **Backend (Rasterization):** Pass the extracted vector outlines directly
   into **FreeType**'s scanline rasterizer (`FT_Render_Glyph` in `ftsmooth.c` /
   `ftgrays.c`) or PDFium's **AGG** rasterizer to generate anti-aliased bitmap
   masks.
3. **Decoupling from Skia:** Remove the `pdf_use_skia` requirement from
   `pdf_enable_fontations`, allowing standalone PDFium (including AGG builds)
   to benefit from memory-safe font parsing.

---

## 2. Motivation: The FreeType Dilemma

### 2.1. The Vulnerability Surface of Embedded PDF Fonts
PDF files frequently embed custom, subsetted, or heavily modified font files.
Over the last two decades, embedded font parsing in FreeType has been one of the
primary attack vectors for remote code execution and memory corruption in
document viewers (integer overflows in table offsets, malformed CMap subtables,
corrupt CFF charstrings).

### 2.2. Parsing vs. Rasterization
FreeType performs two fundamentally distinct tasks:
* **Task A: Binary Font Parsing & Hinting (High Risk):** Reading raw bytes from
  disk/memory into C structs (`CMap`, `Hmtx`, `OS/2`, `Name`, `Post`, `glyf`,
  `CFF`) and running Turing-complete bytecode interpreters in C.
* **Task B: Scanline Rasterization (Low Risk / High Fidelity):** Taking
  mathematical Bézier curves and generating anti-aliased 8bpp grayscale pixel
  masks (`ftgrays.c`) or subpixel LCD masks (`ftsmooth.c`).

FreeType's rasterization math is battle-tested and produces the exact
anti-aliasing curves PDFium expects. The risk is located in **Task A
(Parsing)**.

---

## 3. The Frankenfonts Architecture

```
                       ┌──────────────────────────────────────────────┐
                       │          Untrusted PDF Font Data             │
                       └──────────────────────┬───────────────────────┘
                                              │
                                              ▼
                       ┌──────────────────────────────────────────────┐
                       │     Fontations Frontend (Pure Safe Rust)     │
                       │     - `read-fonts`: Safe Table Parsing       │
                       │     - `skrifa`: CMaps, Metrics, BBoxes       │
                       │     - `HintingInstance`: TrueType / CFF VM   │
                       │     - `OutlinePen`: Bézier Curve Extraction  │
                       └──────────────────────┬───────────────────────┘
                                              │
                                              ▼ Extracted `FT_Outline` / `CFX_Path`
                       ┌──────────────────────────────────────────────┐
                       │     Rasterization Backend (FreeType / AGG)   │
                       │     - Feeds safe Bézier curves into rasterizer│
                       │     - FreeType `FT_Render_Glyph` or AGG AA   │
                       └──────────────────────┬───────────────────────┘
                                              │
                                              ▼
                       ┌──────────────────────────────────────────────┐
                       │               CFX_GlyphBitmap                │
                       │             (Anti-Aliased Mask)              │
                       └──────────────────────────────────────────────┘
```

---

## 4. Key Architectural Benefits

### 4.1. Complete Elimination of Font Parsing CVEs
Because `read-fonts` and `skrifa` are written in 100% memory-safe Rust with
strict bounds-checking, zero-copy slicing, and fuzz-tested arithmetic, PDFium
becomes completely immune to font-table parsing vulnerabilities. FreeType is
never exposed to raw untrusted font byte streams.

### 4.2. High Fidelity & Perceptual Parity
By feeding Skrifa's extracted vector outlines into FreeType's existing
rasterizer (`ftsmooth.c` / `ftgrays.c`), the resulting bitmap masks maintain visual fidelity
with legacy FreeType output. Subpixel hinting variances inherent to independent
TrueType VM implementations are handled via windowed MSE fuzzy test matching
and expectation suffixes.

### 4.3. Full Standalone Independence (No Skia Required)
Currently, `pdf_enable_fontations` is artificially gated on `pdf_use_skia =
true`. Frankenfonts decouples Fontations from Skia:
* `skrifa` and `read_fonts` are vendored directly via `gnrt` in
  `//third_party/rust/`.
* Standalone PDFium (`pdf_use_agg = true`, `pdf_use_skia = false`) can use
  Fontations with zero dependencies on Skia.

---

## 5. Implementation Roadmap

### Phase 1: Decouple GN Flags & Public API
* Updated `pdfium.gni` to assert that `pdf_enable_fontations` only requires
  `enable_rust = true`.
* Defined `PDF_ENABLE_FONTATIONS` in `BUILD.gn` whenever
  `pdf_enable_fontations` is enabled.
* Exposed `CFX_FontMgr::GetFontBackend()` and `FPDF_InitLibraryWithConfig()`
  runtime selection in AGG builds.
* Wired `--fontations` CLI flag into `pdfium_test`, `pdfium_embeddertests`,
  and `pdfium_unittests`.

### Phase 2: Route Font Table & Outline Queries to `skrifa`
* Implement `hinted_outline()` in `main.rs` using `skrifa::outline::HintingInstance`.
* Connect `CFX_Face::RenderGlyph()` to extract outlines through Skrifa at
  the target display PPEM scale and rasterize through `FT_Render_Glyph()`.

---

## 6. Conclusion

Project Frankenfonts delivers **uncompromising memory safety against hostile
font exploits in untrusted PDFs** paired with **FreeType scanline
rasterization**, keeping standalone PDFium lean, fast, and secure.

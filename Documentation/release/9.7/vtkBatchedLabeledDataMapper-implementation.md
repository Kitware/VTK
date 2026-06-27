## vtkBatchedLabeledDataMapper: Implementation

### Background

`vtkFastLabeledDataMapper` was introduced in VTK 9.5 as a faster replacement
for `vtkLabeledDataMapper`. Instead of issuing one draw call per label, it
packs all glyphs into a single texture atlas and emits them in one
geometry-shader pass, making label-heavy scenes significantly cheaper to render.
Its initial implementation lived entirely in `Rendering/OpenGL2`, which tied the
atlas-building logic to the OpenGL backend and made it impossible to reuse the
same approach on other backends or in non-3D contexts.

### New class: vtkBatchedLabeledDataMapper

`vtkBatchedLabeledDataMapper` is a `vtkLabeledDataMapper` subclass that brings
the same glyph-atlas batch rendering to both world-space and display-space label
overlays. It inherits the full API of `vtkLabeledDataMapper` (coordinate systems,
transform, label mode, text properties) but replaces the one-draw-call-per-label
rendering with a glyph atlas batch approach so that all labels are drawn in a
single pass.

It follows a backend-split architecture:

- **`vtkBatchedLabeledDataMapper`** (`Rendering/Label`) — backend-agnostic base
  class that accepts any `vtkDataSet`, builds the glyph atlas, and prepares the
  per-label geometry arrays.
- **`vtkOpenGLBatchedLabeledDataMapper`** (`Rendering/OpenGL2`) — OpenGL backend
  that uploads the atlas to a `vtkTextureObject` and renders all labels in one
  geometry-shader pass using a `vtkOpenGLPolyDataMapper` helper.
- **`vtkWebGPUBatchedLabeledDataMapper`** (`Rendering/WebGPU`) — WebGPU backend
  using instanced rendering: 18 vertices per label instance (3 layers × 6
  vertices) with WGSL shader replacements for atlas sampling, background, and
  frame rendering.

The API extends `vtkLabeledDataMapper` with two additions:

- **`TextAnchor`** — positions the label bounding box relative to its screen
  point (`LowerLeft`, `UpperRight`, `Center`, etc.).
- **`DisplayOffset`** — applies a fixed pixel nudge `(dx, dy)` to every label
  in display space, useful for pushing labels away from their anchor point.

```cpp
vtkNew<vtkBatchedLabeledDataMapper> mapper;
mapper->SetLabelModeToLabelFieldData();
mapper->SetFieldDataName("names");
// drive per-label text property selection from the "types" array
mapper->SetInputArrayToProcess(
  0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "types");
mapper->SetLabelTextProperty(prop0, 0);
mapper->SetLabelTextProperty(prop1, 1);
mapper->SetTextAnchor(vtkBatchedLabeledDataMapper::LowerLeft);
int offset[2] = { 4, 4 };
mapper->SetDisplayOffset(offset);
mapper->SetInputData(dataset);

vtkNew<vtkActor2D> actor;
actor->SetMapper(mapper);
renderer->AddViewProp(actor);
```

### Breaking change: vtkLabeledDataMapper::Implementation

The protected data member `Implementation` in `vtkLabeledDataMapper` has been
changed from a raw pointer (`Internals* Implementation`) to a
`std::unique_ptr<Internals>`. Subclasses that stored or transferred ownership of
this pointer must be updated accordingly.

### Deprecation of vtkFastLabeledDataMapper

`vtkFastLabeledDataMapper` (the original `vtkOpenGLPolyDataMapper`-based 3D label
mapper in `Rendering/OpenGL2`) is deprecated as of VTK 9.7. Use
`vtkBatchedLabeledDataMapper` instead, which supports both world-space and
display-space rendering and works across all backends via the object factory.

### vtkGridAxesActor2D: batch tick label rendering

`vtkGridAxesActor2D` now uses `vtkBatchedLabeledDataMapper` for tick labels.
Previously, each tick mark on each axis edge was backed by an individual
`vtkBillboardTextActor3D`, meaning the renderer issued one draw call per
visible tick on every frame. Each of the four axis edges now owns a single
`vtkBatchedLabeledDataMapper` fed by a per-edge `vtkPolyData`, so the entire
set of tick labels for an edge is rendered in one batched pass. Title labels
remain as `vtkBillboardTextActor3D` actors since there is at most one per edge.

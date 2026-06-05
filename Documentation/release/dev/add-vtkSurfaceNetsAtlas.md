## Add `vtkSurfaceNetsAtlas`

`vtkSurfaceNetsAtlas` is a new filter that consumes the polygonal output of any SurfaceNets filter
(`vtkSurfaceNets2D`, `vtkSurfaceNets3D`, or `vtkGeneralizedSurfaceNets3D`) and exposes it as a
queryable label atlas: a database of Regions (one per label) and Patches (one per adjacent label
pair) that can be efficiently re-queried without re-running upstream surface extraction.

The atlas is built once per input mesh change (gated on the input `vtkPolyData` MTime). Changing
extraction parameters does not trigger a rebuild, making iterative workflows (e.g. interactive label
visibility toggling) inexpensive.

### Output

The filter produces a `vtkPartitionedDataSetCollection` with a `vtkDataAssembly` organized into two
subtrees: `Regions` (one PDS per label) and `Patches` (one PDS per adjacent label pair). Assembly
node names default to `Label_N` / `Label_N_Label_M` and are replaced by user-provided names when
set. Each node carries `Name`, `Label`, `LID` attributes (regions) or `Name0`, `Name1`, `Label0`,
`Label1`, `LID0`, `LID1` (patches).

Each output partition carries identifying cell-data arrays so downstream consumers can work without
the assembly:

- **Region**: `"BoundaryLabels"` (2-component, active scalar; Label0 is always the region's own
  label so normals point outward), `"Label"` (`vtkIdType`, constant), `"LID"` (`int`, constant).
  Field data: `"AdjacentLabels"` (`vtkIdType[]`), `"PatchIDs"` (`int[]`).
- **Patch**: `"BoundaryLabels"` (2-component, active scalar; Label0 < Label1 by value, background
  last), `"PatchID"` (`int`, constant).
- **PDC field data**: `"LIDToLabel"` (`vtkIdType[]`), `"PatchLIDs"` (2-component `int`),
  `"PatchLabels"` (2-component `vtkIdType`). All three are always present.

Constant arrays are backed by `vtkConstantArray` (O(1) storage).

### Extraction

- `GenerateRegions` (default: on) emits one Region PDS per selected label.
- `GeneratePatches` (default: on) emits one Patch PDS per adjacent label pair where at least one
  label is selected.
- `OutputStyle` (default: `Boundary`) controls which cells appear in each Region:
  - `Boundary`: only cells whose interface touches the background label. Combined with
    `GeneratePatches`, this forms an exact partition of the upstream output — no cell duplication,
    same total count.
  - `All`: every cell of the region surface, so interior interface cells appear in both adjacent
    Region partitions.
- `ExtractionMode` (default: `EXTRACT_ALL`) selects which labels are emitted:
  - `EXTRACT_ALL`: every label in the input.
  - `EXTRACT_LABEL_SET`: only labels in the `SelectedLabels` list (empty list → empty output).
- `SelectedLabels` API: `AddSelectedLabel`, `RemoveSelectedLabel`, `ClearSelectedLabels`,
  `SetSelectedLabel(i, label)`, `GetSelectedLabel(i)`, `SetSelectedLabels(vector)`,
  `GetSelectedLabels()`, and `GetNumberOfSelectedLabels()`.
- `GenerateSelectedLabels(numLabels, rangeStart, rangeEnd)` populates the selection with evenly
  spaced integer labels (analogous to `vtkContourValues::GenerateValues`). The two-argument overload
  `GenerateSelectedLabels(rangeStart, rangeEnd)` selects every integer in the range, covering the
  common case of sequential segmentation labels.

### Label names

`SetLabelName` / `AddLabelName` / `RemoveLabelName` / `ClearLabelNames` / `GetLabelName` /
`GetNumberOfLabelNames` assign human-readable names to labels. Names are independent of the
`SelectedLabels` list and work with both extraction modes. They replace the default `"Label_N"` /
`"Label_N_Label_M"` assembly node names and are stored as `"Name"` / `"Name0"` / `"Name1"`
attributes on each node. Default names are filled in automatically at atlas build time for any label
without a user-provided name.

### Topological queries

Post-`Update()` queries cover label↔LID mapping, adjacency, and patch topology:
`GetNumberOfLabels`, `HasLabel`, `GetLIDForLabel`, `GetLabelForLID`, `GetLabelForName`,
`AreAdjacent`, `GetAdjacentLabels`, `GetNumberOfPatches`, `GetPatchID`, `GetPatchLabels`,
`GetPatchCellCount`, `GetPatchesForLabel`. All methods that accept a label value also have a
`const std::string&` name-based overload that resolves via `GetLabelForName`.

### Optional post-processing

`ResolveNonManifoldPoints` splits non-manifold points in each output partition by local connected
component. If the input carries a `NonManifoldTableIndices` point-data array (produced by
`vtkSurfaceNets3D`), only candidate points are processed; otherwise all points are scanned. The
array is never forwarded to output partitions.

---

The following supporting changes were made:

- **`vtkSurfaceNets3D` deprecations**: `OUTPUT_STYLE_BOUNDARY` and `OUTPUT_STYLE_SELECTED`, along
  with the associated `SelectedLabels` API, are deprecated in VTK 9.7 in favor of using
  `vtkSurfaceNetsAtlas` downstream. For backward compatibility these code paths now delegate
  internally to a `vtkSurfaceNetsAtlas` instance and merge the per-label partitions back into a
  single `vtkPolyData` using `vtkAppendPolyData`.

- **`vtkGeneralizedSurfaceNets3D` dispatch**: now dispatches over the concrete scalar type of the
  region-ids input array (via `vtkArrayDispatch`) instead of requiring `vtkIntArray`. Any integer
  array type (`vtkShortArray`, `vtkIntArray`, `vtkLongArray`, etc.) is handled natively without a
  copy.

- **Consistent `"BoundaryLabels"` convention**: `vtkSurfaceNets2D` and `vtkGeneralizedSurfaceNets3D`
  were updated to name the boundary-label cell-data array `"BoundaryLabels"` and to use a
  consistent 2-tuple ordering (background label last; smaller label first when both are
  non-background), matching `vtkSurfaceNets3D`. `vtkGeneralizedSurfaceNets3D` also reverses polygon
  winding when swapping labels to preserve outward-facing normals.

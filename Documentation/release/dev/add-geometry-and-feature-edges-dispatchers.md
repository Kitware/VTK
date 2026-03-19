## Geometry and Feature Edges Dispatcher Filters

Two new dispatcher filters provide unified geometry extraction and feature edge
detection across all VTK data types. These filters were previously part of
ParaView's rendering infrastructure and are now available as standalone VTK
components.

### vtkGeometryFilterDispatcher

`vtkGeometryFilterDispatcher` extracts renderable geometry (surfaces or outlines)
from any VTK data object, automatically dispatching to the appropriate
type-specific implementation. It accepts all major VTK data types as input:

- `vtkImageData`, `vtkRectilinearGrid`, `vtkStructuredGrid`
- `vtkUnstructuredGrid`, `vtkPolyData`
- `vtkHyperTreeGrid`, `vtkExplicitStructuredGrid`
- `vtkCellGrid`, `vtkGenericDataSet`
- Composite datasets (`vtkMultiBlockDataSet`, `vtkPartitionedDataSetCollection`,
  `vtkUniformGridAMR`)

Key capabilities:

- **Surface or outline extraction** controlled by the `UseOutline` flag. When
  enabled (the default), structured volumes produce a bounding box outline;
  when disabled, a full surface is extracted.
- **Feature edge generation** via `GenerateFeatureEdges`. For HyperTreeGrid
  inputs, this delegates to `vtkHyperTreeGridFeatureEdges` rather than
  operating on the extracted polydata.
- **Normal computation** with `GeneratePointNormals` and `GenerateCellNormals`,
  including `FeatureAngle` and `Splitting` controls for sharp edge handling.
- **Nonlinear subdivision** for higher-order cells, controlled by
  `NonlinearSubdivisionLevel`.
- **Cell/point ID passthrough** with `PassThroughCellIds` and
  `PassThroughPointIds` for selection and picking workflows.
- **Parallel support** via `SetController()` for distributed-memory execution,
  with optional `GenerateProcessIds` arrays.
- **AMR-specific options** including `HideInternalAMRFaces` and
  `UseNonOverlappingAMRMetaDataForOutlines`.
- **Mesh caching** for improved performance on repeated updates.

```cpp
vtkNew<vtkGeometryFilterDispatcher> geometry;
geometry->SetInputConnection(reader->GetOutputPort());
geometry->SetUseOutline(0);           // extract surface, not outline
geometry->SetGeneratePointNormals(true);
geometry->Update();
```

### vtkFeatureEdgesDispatcher

`vtkFeatureEdgesDispatcher` extracts feature edges from either `vtkPolyData` or
`vtkHyperTreeGrid` input, dispatching to `vtkFeatureEdges` or
`vtkHyperTreeGridFeatureEdges` respectively.

For polydata input, the following edge types can be extracted independently:

- `BoundaryEdges` — edges used by only one cell
- `FeatureEdges` — edges where the dihedral angle exceeds `FeatureAngle`
- `NonManifoldEdges` — edges shared by more than two cells
- `ManifoldEdges` — interior edges shared by exactly two cells

For HyperTreeGrid input, `MergePoints` controls whether coincident points
are merged using a locator.

```cpp
vtkNew<vtkFeatureEdgesDispatcher> edges;
edges->SetInputConnection(source->GetOutputPort());
edges->SetFeatureAngle(45.0);
edges->SetBoundaryEdges(true);
edges->SetNonManifoldEdges(false);
edges->Update();
```

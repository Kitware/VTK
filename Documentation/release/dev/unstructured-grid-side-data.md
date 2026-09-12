## Side data on vtkUnstructuredGrid

`vtkDataObject` has a new attribute type, `SIDE`, and field association,
`FIELD_ASSOCIATION_SIDES`, for data on the sides shared between cells: the
faces of 3D cells and the edges of 2D cells. `vtkUnstructuredGrid` is the first
data object to carry it. `GetSideData()` returns a `vtkSideData`, the peer of
`vtkPointData` and `vtkCellData`, indexed by side id, and a side shared by two
cells has one id and so one tuple that both cells read.

That matters for data defined on sides rather than at cell centres or points.
Staggered-grid ocean and atmosphere models are the motivating case: they carry
the component of velocity normal to each face, and until now it had to be
interpolated somewhere else before VTK could hold it.

```cpp
vtkNew<vtkDoubleArray> flux;
flux->SetNumberOfTuples(grid->GetNumberOfSides()); // one tuple per shared side
grid->GetSideData()->AddArray(flux);
vtkUnstructuredGridSideMap* sides = grid->GetSideMap();
const vtkIdType sideId = sides->GetCellSide(cellId, localSide);
const int sign = sides->GetSideOrientation(cellId, localSide);
```

From Python the property sits beside the others, and setting a scalar
broadcasts over the sides:

```python
grid.side_data["flux"] = numpy_array
grid.side_data["flux"] = 0.0
```

Side data is reached the usual ways too: `GetAttributes(vtkDataObject::SIDE)`,
`GetNumberOfElements(vtkDataObject::SIDE)`, and input arrays selected with
`FIELD_ASSOCIATION_SIDES`.

The side ids come from `vtkUnstructuredGridSideMap`, reached through
`GetSideMap()`. It stores no side-to-point connectivity: a side's points are
recovered from the cell that represents it, through `vtkCellTypeSides`, so the
only thing kept is which cell-local sides name the same side, which
`vtkUnstructuredGridSides` matches. Cell to side, side to points, side to cells
and orientation are all O(1).

Some things worth knowing:

* The numbering is made on first use. `GetSideMap()` and `GetNumberOfSides()`
  build it, and `HasSideMap()` says whether it has been made; enumerating
  attribute types does not build it. Once the side data holds an array, the
  numbering is pinned. Building it is not thread safe, so call `GetSideMap()`
  once before reading the map from several threads.

* Every cell type with sides in `vtkCellTypeSides` takes part, linear,
  quadratic and arbitrary-order (Lagrange and Bezier), polygons and polyhedra,
  voxels and pixels included. A side above linear order carries its mid-edge,
  face and interior nodes, so the two cells sharing one have to name all of them
  and not just the corners. The higher-order pyramids have no sides here.

* The numbering depends on the topology only. Moving the points keeps it, along
  with the side data and a supplied numbering; changing the cells, their types,
  the polyhedron faces or `HigherOrderDegrees` drops the numbering and the side
  data, with a warning.

* No orientation array is stored. VTK's face templates are outward-wound, so
  the two cells sharing a side traverse it in opposite directions;
  `GetSidePoints()` returns the representative cell's traversal and
  `GetSideOrientation()` returns +1 or -1 for a given cell. Signed side data,
  such as a flux, is relative to the representative cell's outward normal. For
  2D cells the winding relation is a property of the mesh rather than of each
  cell, so `CheckWindingConsistency()` reports the sides where it fails.

* A reader can supply the numbering rather than have one derived, through
  `vtkUnstructuredGrid::SetSideIds()`, so that side ids match the ones a file already uses and data
  dimensioned on sides reads straight in with no permutation.

* Ghost cells take part like any other cell, so the numbering is per process:
  a side on the boundary between two processes has an id on each, and there is
  no global side numbering.

* `ShallowCopy` and `DeepCopy` carry the numbering and the side data.
  `CopyStructure` carries the numbering, and `CopyAttributes` the side data,
  as they do for the cells and the cell data; filters that pass only point and
  cell data drop the side data, and filters that change the topology drop both.

* `.vtu` files keep it: `vtkXMLUnstructuredGridWriter` writes the side data of
  each piece in a `SideData` element, and a supplied numbering in a `SideIds`
  element, in XML file format version 2.4. `vtkXMLUnstructuredGridReader` lists the side arrays before reading,
  reads the ones selected with `SetSideArrayStatus()`, and reads them back when
  one piece is read into the output; `vtkXMLPUnstructuredGridReader` does the
  same for one piece of a `.pvtu` file. With several pieces in one output, the
  side data is skipped with a warning. The state serialization of
  `vtkUnstructuredGrid` carries it too.

Not yet covered: merging the side data of several pieces into one grid, the
legacy and HDF formats, selecting sides through `vtkSelection`, and
`vtkPolyData`, which is the natural second user of the side association.

## Add vtkUnstructuredGridSides: the sides of the cells, without cell links

`vtkUnstructuredGridSides` (new) finds the sides (faces, edges or vertices) of the cells of a
`vtkUnstructuredGrid` and keeps them depending on how many cells share them, e.g., the boundary
faces, or groups the equal ones, e.g., a face and the cells that share it, without building cell
links, as `vtkCellGridComputeSides` does for cell grids. Hidden, refined and duplicate ghost cells
are handled, so that the interfaces between processes are not boundaries. The changes below lead
to it, and build on it.

### Scattering elements by key

`vtk::keys` (new, `vtkKeys.h`) scatters elements in parallel into the order of their keys: with a
counting sort for dense keys such as point ids, as DP-Hash-Count does, or with a sort for sparse
keys of any ordered type, then reduces, groups or compacts the elements of every key with
functors on ranges of keys. The sides are scattered by their smallest point id.

### Static element locator

`vtkStaticElementLocator` (new), built on `vtk::keys`, locates or groups any topological
element, e.g., `EdgeTuple`, `TriangleTuple` or `QuadTuple`. `vtkStaticEdgeLocatorTemplate` is
deprecated in favor of it, since the offsets of its `MergeEdges()` can overflow the id type, and
`vtkStaticEdgeLocatorTemplate.txx` is removed.

### Face hash links

`vtkStaticFaceHashLinksTemplate` is now implemented on
`vtkUnstructuredGridSides::BuildScatteredSides()`: its faces are in the order of their cells, the
same from one run to the next, and only the 4 combinations of types of `vtkUnstructuredGridSides`
are supported.

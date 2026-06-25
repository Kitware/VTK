## Pythonic APIs for vtkCellArray, vtkPoints, and vtkUnstructuredGrid

`vtkCellArray`, `vtkPoints`, and `vtkUnstructuredGrid` now have Pythonic
constructors and improved string representations, making it easier to create
and inspect mesh data from Python.

**vtkCellArray** accepts offsets and connectivity arrays directly in its
constructor. These can be NumPy arrays, Python lists, or `vtkDataArray`
instances:

```python
from vtkmodules.vtkCommonDataModel import vtkCellArray

cells = vtkCellArray(offsets=[0, 3, 6], connectivity=[0, 1, 2, 3, 4, 5])
```

**vtkPoints** accepts point data via the `data` keyword — as a NumPy
array, a list of coordinate tuples, or a `vtkDataArray`:

```python
from vtkmodules.vtkCommonDataModel import vtkPoints

points = vtkPoints(data=[[0, 0, 0], [1, 0, 0], [0, 1, 0]])
```

**vtkUnstructuredGrid** now supports setting cells via a `(cell_type,
cell_array)` tuple through its `cells` property:

```python
from vtkmodules.vtkCommonDataModel import vtkUnstructuredGrid, VTK_TRIANGLE

grid = vtkUnstructuredGrid()
grid.points = vtkPoints(data=[[0, 0, 0], [1, 0, 0], [0, 1, 0]])
grid.cells = (VTK_TRIANGLE, vtkCellArray(offsets=[0, 3], connectivity=[0, 1, 2]))
```

All three classes provide informative `repr` output for interactive use.

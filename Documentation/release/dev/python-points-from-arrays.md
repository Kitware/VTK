## Python: points keep their float type and can be assigned from arrays

`vtkPoints(data=...)` now keeps the type of float32 and float64 numpy
arrays, and uses a C-contiguous one without copying it. It used to convert
everything to float64, which silently copied float32 data and doubled its
size. Integer arrays and lists are still converted to float64. An explicit
`data_type` now wins over the type of the data instead of being ignored:

```python
import numpy as np
from vtkmodules.vtkCommonCore import vtkPoints, VTK_FLOAT

xyz = np.random.rand(100, 3).astype(np.float32)
pts = vtkPoints(data=xyz)                     # float, shares memory with xyz
pts = vtkPoints(data_type=VTK_FLOAT, data=xyz.astype(np.float64))  # float
```

The `points` property of datasets now accepts anything `vtkPoints(data=...)`
does, so points can be assigned from a numpy array, a list of lists or a
`vtkDataArray` directly, including in constructors:

```python
from vtkmodules.vtkCommonDataModel import vtkPolyData

pd = vtkPolyData(points=xyz)
pd.points = [[0, 0, 0], [1, 0, 0], [0, 1, 0]]
```

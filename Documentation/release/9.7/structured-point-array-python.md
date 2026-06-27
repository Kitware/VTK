## vtkStructuredPointArray Python Integration

VTK now provides full Python support for `vtkStructuredPointArray`, enabling
efficient lazy access to structured grid point coordinates without
materializing the full (N, 3) array.

### C++ API Additions

`vtkStructuredPointArray` now exposes the coordinate arrays stored in its
backend:

```cpp
vtkDataArray* GetXCoordinates();
vtkDataArray* GetYCoordinates();
vtkDataArray* GetZCoordinates();
bool GetUsesDirectionMatrix();
```

These methods are available on all `vtkStructuredPointArray` template
instantiations and are accessible from Python.

### VTKStructuredPointArray: Lazy NumPy Integration

With the mixin override, `vtkStructuredPointArray` instances are
automatically `VTKStructuredPointArray` objects that support
numpy-compatible operations without materializing the full coordinate
array. For a grid with dimensions (nx, ny, nz), only O(nx + ny + nz)
storage is used instead of O(nx * ny * nz * 3).

```python
from vtkmodules.vtkCommonDataModel import vtkImageData
import numpy as np

img = vtkImageData()
img.SetDimensions(100, 200, 300)
img.SetOrigin(1.0, 2.0, 3.0)
img.SetSpacing(0.5, 1.0, 1.5)

arr = img.GetPoints().GetData()
print(type(arr).__name__)  # VTKStructuredPointArray
print(arr.shape)           # (6000000, 3)
print(arr.dtype)           # float64

# Single point indexing — O(1), no materialization
print(arr[0])  # [1.0, 2.0, 3.0]

# Column indexing returns lazy axis arrays
x = arr[:, 0]  # VTKStructuredAxisArray, only 100 unique values stored
```

### Lazy Arithmetic and Ufuncs

Ufuncs and scalar arithmetic operate per-axis and return lazy results:

```python
# These all operate on compact axis arrays, not the full 6M-point array
result = arr + 10       # VTKStructuredPointArray (lazy)
result = arr * 2        # VTKStructuredPointArray (lazy)
result = np.sqrt(arr)   # VTKStructuredPointArray (lazy)
result = -arr           # VTKStructuredPointArray (lazy)
```

### Optimized Reductions

Reductions use O(nx + ny + nz) formulas:

```python
np.sum(arr)            # computed from axis sums
np.min(arr, axis=0)    # computed from axis mins
np.max(arr, axis=0)    # computed from axis maxes
np.mean(arr, axis=0)   # computed from axis means
```

### Explicit Materialization

When the full array is needed:

```python
full = arr.to_numpy()       # explicit (N, 3) ndarray
full = np.asarray(arr)      # also materializes
```

### Data Model Integration

The `points` property on `ImageData` and `RectilinearGrid` returns a
lazy `VTKStructuredPointArray`:

```python
img = vtkImageData()
img.SetDimensions(100, 200, 300)
points = img.points  # VTKStructuredPointArray, lazy
```

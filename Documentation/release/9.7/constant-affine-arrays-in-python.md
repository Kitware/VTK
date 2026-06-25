## VTKConstantArray and VTKAffineArray: NumPy-Compatible Mixins

`vtkConstantArray` and `vtkAffineArray` instances are now automatically wrapped
with numpy-compatible mixin classes (`VTKConstantArray` and `VTKAffineArray`)
that support efficient operations without materializing large arrays.

### VTKConstantArray

```python
from vtkmodules.vtkCommonCore import vtkConstantArray
import numpy as np

arr = vtkConstantArray['float64']((1000000, 3), 5.0)

# Array-like properties work without materialization
print(arr.shape)  # (1000000, 3)
print(arr.dtype)  # float64
print(arr.value)  # 5.0

# Arithmetic with numpy arrays uses the scalar constant
np_arr = np.random.rand(1000000, 3)
result = arr + np_arr  # Equivalent to 5.0 + np_arr (no memory explosion)

# Arithmetic between constant arrays returns a new constant array
const2 = arr * 2  # Returns VTKConstantArray with value 10.0

# Ufuncs use the scalar, not a full array
np.sqrt(arr)      # Returns constant array with sqrt(5.0)
```

### VTKAffineArray

```python
from vtkmodules.vtkCommonCore import vtkAffineArray
import numpy as np

arr = vtkAffineArray['float64'](100, 2.0, 5.0)  # slope=2.0, intercept=5.0

print(arr.shape)      # (100,)
print(arr.slope)      # 2.0
print(arr.intercept)  # 5.0

# Arithmetic that preserves affine structure
result = arr * 3      # Returns VTKAffineArray with slope=6.0, intercept=15.0
result = arr + 10     # Returns VTKAffineArray with slope=2.0, intercept=15.0
```

### Automatic Detection in Data Model

When accessing arrays through the data model API, constant and affine arrays
are returned directly with metadata set:

```python
arr = polydata.point_data["my_constant_array"]
print(arr.value)     # the constant value
print(arr.dataset)   # the owning dataset
```

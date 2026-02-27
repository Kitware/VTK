## Pythonic API for vtkMatrix3x3 and vtkMatrix4x4

`vtkMatrix3x3` and `vtkMatrix4x4` now support natural Python indexing and
operators:

```python
from vtkmodules.vtkCommonMath import vtkMatrix4x4

# Construct from nested lists or flat tuple
m = vtkMatrix4x4([[1,0,0,1],[0,1,0,2],[0,0,1,3],[0,0,0,1]])

# Element access
m[0, 3]           # 1.0
m[0, 3] = 5.0     # SetElement(0, 3, 5.0)

# Row access
m[1]              # (0.0, 1.0, 0.0, 2.0)
m[1] = [0,0,0,0]  # set entire row

# Negative indexing
m[-1, -1]         # last element

# Length
len(m)            # 4

# Matrix multiply
c = a @ b

# Invert
inv = ~m

# Comparison
a == b
a != b
```

The traditional C++ API (`GetElement`, `SetElement`, `Identity`, etc.) continues
to work unchanged.

## Pythonic API for implicit functions

VTK implicit functions now have a Pythonic API with constructor keyword
arguments, a callable protocol, CSG (constructive solid geometry)
operators, and informative ``repr``.

### Construction

```python
from vtkmodules.vtkCommonDataModel import vtkSphere, vtkPlane, vtkBox

s = vtkSphere(center=(0, 0, 0), radius=1.0)
p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))
b = vtkBox(bounds=(-1, 1, -1, 1, -1, 1))
```

Snake_case keyword arguments map to the corresponding ``Set`` methods.
This is provided by the Python wrapping layer for every VTK class, so it
works for all implicit functions.

### Callable protocol

```python
s(1, 2, 3)        # three scalars -> float
s([1, 2, 3])      # 3-element sequence -> float
s(Nx3_array)      # batch of points -> 1D numpy array
```

### CSG operators

```python
union        = s | p     # vtkImplicitBoolean (union)
intersection = s & p     # vtkImplicitBoolean (intersection)
difference   = s - p     # vtkImplicitBoolean (difference)
negated      = ~s        # vtkImplicitSum with weight -1.0

combined = (s & p) - vtkCylinder(radius=0.5)   # operators chain
```

The results are themselves implicit functions, so they are callable and
can be combined further.

### Repr

```python
repr(vtkSphere(center=(0, 0, 0), radius=1.0))
# "vtkSphere(center=(0.0, 0.0, 0.0), radius=1.0)"
```

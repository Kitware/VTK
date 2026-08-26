# Python reference cycles through attributes are now collected

Storing an attribute on a wrapped VTK object no longer risks leaking that object
for the lifetime of the process.  The Python wrapper's `tp_traverse` now reports
the instance dictionary to CPython's cyclic garbage collector, so a cycle that
passes through an attribute -- for example a cached helper that holds the object
it was created from -- is found and freed:

```python
import gc
from vtkmodules.vtkCommonCore import vtkPoints

class Accessor:
    def __init__(self, obj):
        self.obj = obj

points = vtkPoints()
points.accessor = Accessor(points)  # cycle: points -> __dict__ -> accessor -> points
del points
gc.collect()                        # frees both; previously freed neither
```

Previously the collector was told that the wrapper referred to nothing but its
observers, so such a cycle looked externally referenced and neither the wrapper
nor the C++ object it held was ever released.  Code that worked around this by
holding a `vtkWeakReference` remains correct.

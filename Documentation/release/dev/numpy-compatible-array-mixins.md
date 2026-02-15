## NumPy-compatible array mixins for VTK data arrays

VTK data arrays now behave like NumPy arrays in Python. The new `VTKAOSArray`
and `VTKSOAArray` mixin classes are automatically applied to all
`vtkAOSDataArrayTemplate` and `vtkSOADataArrayTemplate` instances, so arrays
obtained from VTK support arithmetic, indexing, reductions, and ufuncs without
manual conversion.

### Zero-copy NumPy integration

AOS arrays expose a single contiguous buffer as a zero-copy NumPy view. SOA
arrays expose per-component buffers as zero-copy NumPy views and perform
element-wise operations per-component, avoiding conversion to interleaved
layout:

```python
from vtkmodules.vtkFiltersSources import vtkSphereSource

sphere = vtkSphereSource()
sphere.Update()
points = sphere.GetOutput().GetPoints().GetData()

# Arithmetic works directly on VTK arrays
scaled = points * 2.0
offset = points + 1.0

# NumPy functions work transparently
import numpy as np
print(np.mean(points, axis=0))
print(np.min(points), np.max(points))
```

### SOA per-component operations

SOA arrays preserve their structure-of-arrays layout through operations,
avoiding the overhead of interleaving components:

```python
from vtkmodules.util.numpy_support import numpy_to_vtk_soa

x = np.random.rand(1000)
y = np.random.rand(1000)
z = np.random.rand(1000)
soa = numpy_to_vtk_soa([x, y, z], name="coords")

# Per-component operations — no AOS conversion
result = soa * 2.0 + 1.0
print(soa.components)        # [x_array, y_array, z_array]
print(np.sum(soa, axis=0))   # per-component sum
```

### Features

- **Arithmetic operators**: `+`, `-`, `*`, `/`, `//`, `**`, `%` and their
  reverse variants
- **Comparison operators**: `<`, `<=`, `==`, `!=`, `>=`, `>`
- **NumPy ufuncs**: element-wise operations dispatch per-component for SOA
- **NumPy array functions**: `sum`, `mean`, `min`, `max`, `std`, `var`,
  `any`, `all`, `prod`, `argmin`, `argmax`, `cumsum`, `cumprod`,
  `concatenate`, `clip`, `sort`, `where`, `isin`, `round`, `dot`
- **Indexing**: scalar, slice, boolean, and fancy indexing
- **Memory safety**: `BufferChangedEvent` observation invalidates stale views
- **Metadata propagation**: dataset and association metadata flow through
  element-wise operations

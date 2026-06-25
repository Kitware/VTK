## Rename VTKCompositeDataArray to VTKPartitionedArray

The `VTKCompositeDataArray` class in `numpy_interface.dataset_adapter` has been
extracted into its own module and renamed to `VTKPartitionedArray` in
`numpy_interface.vtk_partitioned_array`.  The new name better reflects the VTK
data model, where partitioned datasets (`vtkPartitionedDataSet`,
`vtkPartitionedDataSetCollection`) are the primary use case.

The legacy `VTKCompositeDataArray` class remains in `dataset_adapter.py` for
backward compatibility but is no longer the primary implementation.

### Local vs parallel semantics

NumPy overrides on `VTKPartitionedArray` perform **local-only** reductions:

```python
import numpy as np
# Local reduction (single process)
np.sum(partitioned_array)
```

For MPI-parallel reductions, use `numpy_interface.algorithms`:

```python
from vtkmodules.numpy_interface import algorithms as algs
# Parallel reduction (uses MPI global controller if available)
algs.sum(partitioned_array)
```

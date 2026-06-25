## VTKCompositeArray numpy-compatible Python mixin

`vtkCompositeArray` (which concatenates multiple sub-arrays into a single
implicit array) now has a Python mixin that provides numpy-compatible
operations:

- **Sub-array access**: `comp.arrays` returns the original sub-arrays with
  their own mixins (VTKAOSArray, VTKConstantArray, etc.).
- **Lazy arithmetic**: Element-wise operations (`+`, `-`, `*`, `/`, ufuncs)
  are applied per-sub-array without materializing the full composite.
- **O(n_arrays) reductions**: `sum`, `min`, `max`, `mean`, `std`, `var`,
  `any`, `all`, and `prod` operate across sub-arrays rather than iterating
  over all elements.
- **Indexing**: Scalar and slice indexing delegate to the appropriate
  sub-array via binary search on tuple offsets.
- **Materialization**: `numpy.array(comp)` and `comp.to_numpy()` concatenate
  sub-arrays into a standard numpy array.
- **Read-only**: `__setitem__` raises `TypeError` (implicit arrays are
  immutable).

New C++ accessors on `vtkCompositeArray` and `vtkCompositeImplicitBackend`:
- `GetNumberOfArrays()` — number of sub-arrays
- `GetArray(idx)` — the original sub-array at index `idx`
- `GetOffset(idx)` — cumulative tuple offset for sub-array `idx`

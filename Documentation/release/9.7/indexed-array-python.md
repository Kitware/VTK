## vtkIndexedArray Python mixin

`vtkIndexedArray` now has a numpy-compatible Python mixin (`VTKIndexedArray`)
that provides seamless numpy integration for indexed implicit arrays.

Indexed arrays provide reindexed access to a base array via an index list:
`result[i] = base_array[index_array[i]]`. They are used in HyperTree Grid
filters (threshold, contour, cell size) and the probe filter for zero-copy
subset views that save memory by storing indices rather than duplicating values.

### Lazy evaluation

The mixin avoids materializing the full array wherever possible by exploiting
the two-array structure (base array + index array):

**Operations that stay lazy (return a new `VTKIndexedArray`):**

- **Unary ufuncs** (`numpy.sqrt(arr)`, `-arr`, `abs(arr)`): the ufunc is
  applied to the base array and the result is wrapped with the same index
  mapping. No per-element gather is needed.
- **Binary ufuncs with scalars** (`arr + 5`, `arr * 2`): same strategy —
  the scalar operation is applied to the base array, indexes are reused.
- **Slice indexing** (`arr[1:4]`, `arr[::2]`): a subset of the index array
  is selected, still pointing at the same base. No values are copied.
- **Fancy integer indexing** (`arr[[0, 3, 7]]`): same as slicing — the
  index array is re-indexed.
- **Boolean mask indexing** (`arr[mask]`): the mask selects positions in
  the index array, base is unchanged.
- **Scalar element access** (`arr[0]`): direct C++ lookup through the
  implicit array, no Python-level materialization.

**Operations that materialize (return a numpy array):**

- **Reductions** (`sum`, `min`, `max`, `mean`, `std`, `var`): because the
  index array can contain duplicates and omissions, a reduction over the
  indexed array is not equivalent to a reduction over the base array.
  The selected values must be gathered before reducing.
- **Binary ufuncs with array operands** (`arr + numpy_array`): the other
  array's elements are aligned with the indexed output, not with the base,
  so the indexed array must be materialized to match.
- **Explicit materialization** (`numpy.asarray(arr)`, `arr.to_numpy()`).

### API

- `base_array` / `index_array` properties to access the underlying arrays
- `numpy.asarray()` support via efficient numpy fancy indexing
- `to_numpy()` for explicit materialization
- Arithmetic operators, comparisons, and unary operators
- Numpy ufunc and function dispatch
- Read-only enforcement (`__setitem__` raises `TypeError`)

### C++ changes

New accessors `GetBaseArray()` and `GetIndexArray()` on both
`vtkIndexedArray` and `vtkIndexedImplicitBackend` expose the original
arrays for Python interoperability. For the `vtkIdList` constructor
overload, the id list is converted to a `vtkIdTypeArray` so that it is
accessible from Python.

## Fallback NumPy-Compatible Mixin for Implicit VTK Arrays

VTK implicit arrays (`vtkImplicitArray<BackendT>`) that lack a dedicated
Python override now automatically receive a numpy-compatible interface via
the `VTKImplicitArray` fallback mixin.

Previously, implicit arrays whose backend types were not exposed to Python
wrapping resolved to bare `vtkDataArray` objects with no numpy integration.
Now, any `vtkDataArray` subclass without a more-specific override (AOS, SOA,
constant, or affine) is wrapped with `VTKImplicitArray`, which materialises
values on first access via `DeepCopy` to an AOS array and caches the result.

```python
# Implicit arrays from C++ filters work transparently with numpy
arr = some_filter.GetOutput().GetPointData().GetArray("implicit_field")
print(np.sum(arr))      # materialises once, then uses cache
print(arr + 1.0)        # arithmetic works as expected
print(arr[10:20])       # slicing works
```

The cache is automatically invalidated when the array fires a
`ModifiedEvent`, so subsequent reads after pipeline updates produce
correct results.

Additionally, `vtk_to_numpy()` now works correctly with implicit arrays.
Previously it would fail because implicit arrays do not expose the C-level
buffer protocol. The function now uses `__array__()` when available,
which handles both implicit and explicit arrays correctly on all Python
versions.

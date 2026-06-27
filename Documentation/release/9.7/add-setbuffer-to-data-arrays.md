## SetBuffer for AOS and SOA Data Array Templates

`vtkAOSDataArrayTemplate` and `vtkSOADataArrayTemplate` now provide a `SetBuffer()`
method that accepts a `vtkBuffer` (or `vtkAbstractBuffer` in Python) to replace the
array's internal storage. This enables zero-copy buffer transfers between arrays
without going through raw pointers.

### AOS Arrays

```cpp
vtkNew<vtkBuffer<double>> buffer;
buffer->Allocate(100);
// ... fill buffer ...

vtkNew<vtkDoubleArray> arr;
arr->SetNumberOfComponents(3);
arr->SetBuffer(buffer, true); // true = update MaxId from buffer size
```

### SOA Arrays

For SOA arrays, `SetBuffer()` takes a component index since each component is
stored separately:

```cpp
vtkNew<vtkBuffer<double>> buf0;
buf0->Allocate(100);

vtkNew<vtkSOADataArrayTemplate<double>> arr;
arr->SetNumberOfComponents(2);
arr->SetBuffer(0, buf0, true);  // set buffer for component 0
arr->SetBuffer(1, buf1, false); // set buffer for component 1
```

### Python

`SetBuffer()` is fully wrapped for Python using `vtkAbstractBuffer`:

```python
src = vtkFloatArray()
src.SetNumberOfTuples(100)

dst = vtkFloatArray()
dst.SetNumberOfComponents(1)
dst.SetBuffer(src.GetBuffer(), True)
```

### BufferChangedEvent

`SetBuffer()`, `SetArray()`, and `ShallowCopy()` now fire `BufferChangedEvent` on
both AOS and SOA array templates, matching the existing behavior of `Resize()` and
`InsertNextTuple()`. This ensures that external consumers (such as NumPy views) are
notified whenever the underlying buffer changes.

# vtkBuffer Python Integration and NumPy Memory Safety

## Overview

VTK now provides improved Python/NumPy interoperability through the `vtkBuffer` class
with full buffer protocol support. This enables zero-copy data sharing between VTK
arrays and NumPy, along with improved memory safety when VTK arrays reallocate.

## vtkBuffer and the Python Buffer Protocol

### Direct Buffer Access

You can now use `vtkBuffer` directly from Python with seamless NumPy integration.
The buffer protocol implementation enables zero-copy views of VTK memory:

```python
import numpy as np
from vtkmodules.vtkCommonCore import vtkBuffer

# Create a typed buffer
buf = vtkBuffer['float64']()
buf.Allocate(100)

# Create a numpy array that shares memory with the buffer (zero-copy)
arr = np.asarray(buf)

# Modifications through numpy are reflected in the VTK buffer
arr[:] = np.linspace(0, 1, 100)

# Multiple numpy arrays can share the same buffer
arr2 = np.asarray(buf)
assert np.shares_memory(arr, arr2)
```

### Supported Data Types

The buffer protocol supports all standard VTK scalar types with automatic NumPy
dtype mapping:

| VTK Type | NumPy dtype |
|----------|-------------|
| `float32` | `numpy.float32` |
| `float64` | `numpy.float64` |
| `int8` | `numpy.int8` |
| `uint8` | `numpy.uint8` |
| `int16` | `numpy.int16` |
| `uint16` | `numpy.uint16` |
| `int32` | `numpy.int32` |
| `uint32` | `numpy.uint32` |
| `int64` | `numpy.int64` |
| `uint64` | `numpy.uint64` |

### Memory Views

Standard Python `memoryview` objects also work with `vtkBuffer`:

```python
buf = vtkBuffer['int32']()
buf.Allocate(4)

m = memoryview(buf)
print(m.shape)     # (4,)
print(m.itemsize)  # 4
print(m.format)    # 'i'
```

## vtkAbstractBuffer Base Class

A new `vtkAbstractBuffer` base class provides the interface for buffer protocol
support. It defines virtual methods for type-agnostic buffer access:

- `GetVoidBuffer()` - Returns the raw buffer pointer
- `GetNumberOfElements()` - Returns the number of elements
- `GetDataType()` - Returns the VTK type identifier (e.g., `VTK_FLOAT`)
- `GetDataTypeSize()` - Returns the size in bytes of each element

This abstraction enables Python wrapping code to support any buffer type
without knowing the specific template instantiation.

## Accessing Buffers from Data Arrays

You can now access the underlying `vtkBuffer` objects from VTK data arrays,
enabling direct buffer protocol access to array memory:

```python
import numpy as np
from vtkmodules.vtkCommonCore import vtkFloatArray

# Create a VTK array
arr = vtkFloatArray()
arr.SetNumberOfComponents(3)
arr.SetNumberOfTuples(100)

# Get the underlying buffer (for AOS arrays)
buf = arr.GetBuffer()

# Create a numpy view of the raw buffer
raw_data = np.asarray(buf)
print(raw_data.shape)  # (300,) - flattened view of 100 tuples x 3 components
```

### Array Type Methods

Different array types provide appropriate buffer access methods:

- **vtkAOSDataArrayTemplate** (e.g., `vtkFloatArray`): `GetBuffer()` returns the
  single contiguous buffer
- **vtkSOADataArrayTemplate**: `GetComponentBuffer(int comp)` returns the buffer
  for a specific component
- **vtkScaledSOADataArrayTemplate**: `GetComponentBuffer(int comp)` returns the
  buffer for a specific component

## Memory Safety with BufferChangedEvent

### The Problem

When a NumPy array references VTK buffer memory and the VTK array reallocates
(e.g., due to `Resize()` or `InsertNextTuple()`), the NumPy array may point to
invalid memory, causing crashes or data corruption.

### The Solution

VTK data arrays now fire a `BufferChangedEvent` whenever they reallocate their
internal buffers. Python wrapper classes like `VTKArray` observe this event and
mark themselves as stale, raising a `RuntimeError` if accessed after the buffer
has changed.

When using `vtkBuffer` directly, you should obtain fresh buffer references after
any operation that might reallocate the array.

### Example

```python
import numpy as np
from vtkmodules.vtkCommonCore import vtkFloatArray
import vtkmodules.numpy_interface.dataset_adapter as dsa

arr = vtkFloatArray()
arr.SetNumberOfValues(10)
arr.SetValue(0, 42.0)

# Create a VTKArray wrapper (recommended for safety)
va = dsa.vtkDataArrayToVTKArray(arr)
print(va[0])  # 42.0

# Resize the VTK array - this may reallocate the buffer
arr.SetNumberOfValues(1000)

# VTKArray detects the stale buffer and raises RuntimeError
try:
    print(va[0])  # Raises RuntimeError
except RuntimeError:
    print("Buffer changed - get a fresh reference")

# Get a fresh VTKArray reference
va_new = dsa.vtkDataArrayToVTKArray(arr)
print(va_new.shape)  # (1000,)
```

## Improved numpy_support and dataset_adapter

### numpy_support Module

The `numpy_to_vtk()` function now stores NumPy array references on the buffer
rather than the data array. This ensures the NumPy memory stays valid even if
the VTK array is modified:

```python
from vtkmodules.util.numpy_support import numpy_to_vtk
import numpy as np

data = np.array([1.0, 2.0, 3.0], dtype=np.float32)
vtk_arr = numpy_to_vtk(data)

# The numpy array reference is stored on the buffer, keeping memory valid
```

### dataset_adapter Module

The `VTKArray` class in the dataset adapter now stores a reference to the
underlying buffer, ensuring memory validity throughout the VTKArray's lifetime:

```python
import vtkmodules.numpy_interface.dataset_adapter as dsa
from vtkmodules.vtkFiltersSources import vtkSphereSource

sphere = vtkSphereSource()
sphere.Update()

# Wrap the output
wrapped = dsa.WrapDataObject(sphere.GetOutput())

# Access point coordinates as VTKArray (numpy subclass)
points = wrapped.Points

# The VTKArray holds a buffer reference, ensuring memory safety
print(points.shape)
```

## Best Practices

1. **Use VTKArray for automatic safety**: The dataset adapter's `VTKArray` class
   automatically detects buffer changes and raises `RuntimeError` on stale access.

2. **Get fresh references after modifications**: If you modify a VTK array's size,
   obtain a new buffer reference and NumPy view.

3. **Use zero-copy when possible**: Creating NumPy views via `np.asarray(buf)`
   avoids data copying and provides the best performance.

4. **Observe BufferChangedEvent for custom wrappers**: If you create custom Python
   wrappers around VTK arrays, observe `BufferChangedEvent` to detect reallocation.

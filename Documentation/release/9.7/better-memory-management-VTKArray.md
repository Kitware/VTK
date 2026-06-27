# BufferChangedEvent for VTKArray Stale Detection

## Overview

VTK data arrays now fire a `BufferChangedEvent` when they reallocate their
internal buffer. The `VTKArray` class in the dataset adapter uses this event
to detect when a wrapped array becomes stale, raising a `RuntimeError` to
prevent use of invalid memory.

## BufferChangedEvent

A new event `vtkCommand::BufferChangedEvent` is invoked when data array
templates reallocate their internal buffers via `ReallocateTuples()`. This
affects:

- `vtkAOSDataArrayTemplate` (e.g., `vtkFloatArray`, `vtkIntArray`)
- `vtkSOADataArrayTemplate`
- `vtkScaledSOADataArrayTemplate`

## Stale Array Detection in Python

When you create a `VTKArray` from a VTK data array, it automatically observes
`BufferChangedEvent`. If the underlying buffer is reallocated, accessing the
`VTKArray` raises a `RuntimeError`:

```python
import vtkmodules.numpy_interface.dataset_adapter as dsa
from vtkmodules.vtkCommonCore import vtkFloatArray

arr = vtkFloatArray()
arr.SetNumberOfValues(10)
va = dsa.vtkDataArrayToVTKArray(arr)

# Resize triggers buffer reallocation
arr.SetNumberOfValues(100)

# Raises RuntimeError - the VTKArray is now stale
print(va[0])
```

The observer removes itself after firing once and is automatically cleaned up
when the `VTKArray` is garbage collected.

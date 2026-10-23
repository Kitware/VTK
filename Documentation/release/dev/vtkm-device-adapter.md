## Choose the Viskores device at run time

The new `vtkmDeviceAdapter` class selects which Viskores devices VTK's
accelerated filters run on, from C++ or Python, without restarting the
process or setting `VISKORES_DEVICE` before Viskores starts:

```python
from vtkmodules.vtkAcceleratorsVTKmCore import vtkmDeviceAdapter

vtkmDeviceAdapter.GetAvailableDevices()  # e.g. "Serial Cuda"
vtkmDeviceAdapter.ForceDevice("serial")  # run on Serial only
vtkmDeviceAdapter.DisableDevice("Cuda")  # or just stop using one device
vtkmDeviceAdapter.EnableDevice("Cuda")
vtkmDeviceAdapter.Reset()                # back to every device that can run
```

Devices are named as Viskores names them, ignoring case. A request for a
device Viskores does not know, or that this build or machine cannot run,
is refused with a warning, returns false, and leaves the selection as it
was. Like Viskores' runtime device tracker, the selection applies to the
thread that makes it; a thread that starts using Viskores later begins from
the main thread's selection.

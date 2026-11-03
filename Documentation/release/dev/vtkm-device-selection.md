## Choose the Viskores device at run time

The new `vtkmDeviceSelection` class selects which Viskores devices VTK's
accelerated filters run on, from C++ or Python, without restarting the
process or setting `VISKORES_DEVICE` before Viskores starts:

```python
from vtkmodules.vtkAcceleratorsVTKmCore import vtkmDeviceSelection

vtkmDeviceSelection.GetAvailableDevices()  # e.g. "Serial Cuda"
vtkmDeviceSelection.ForceDevice("serial")  # run on Serial only
vtkmDeviceSelection.DisableDevice("Cuda")  # or just stop using one device
vtkmDeviceSelection.EnableDevice("Cuda")
vtkmDeviceSelection.Reset()                # back to every device that can run
```

Devices are named as Viskores names them, ignoring case. A request for a
device Viskores does not know is refused with a warning, returns false, and
leaves the selection as it was. So is forcing or enabling a device this
build or machine cannot run; disabling one succeeds. Like Viskores' runtime device tracker, the selection applies to the
thread that makes it; a thread that starts using Viskores later begins from
the main thread's selection.

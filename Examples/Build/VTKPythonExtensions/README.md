## Introduction:

This directory demonstrates how to organize a local source repository
where C++ code can be compiled and linked against the VTK package available
on PyPi or
[VTK GitLab Package Registry](https://gitlab.kitware.com/vtk/vtk/-/packages).

Most of the time, you will also generate a Python wrappers for said C++ code.
You may use any existing Python wrapper generator or VTK modules.
Using VTK modules is probably the easiest way to build VTK Python Extensions.
For more information about building VTK modules from an externally built VTK,
please refer to the Build/vtkMy example.

## Build and test this example:

To build the example you need a vtk-sdk. Then call:
```sh
pip wheel .
```

It will generate a file name `vtk-pyext-<version>-<pyabi>-<platform>.whl`
It can then be installed using `pip install vtk_pyext-*`
Then imported in python:

```py
import vtkmodules.vtkCommonCore
from vtk_pyext import vtkDummy
print(vtkDummy.vtkDummy())
```

Output:
```
vtkDummy (0000017A04112080)
vtkDummy:
    Debug: Off
    Modified Time: 3
    Reference Count: 1
    Registered Events: (none)
```

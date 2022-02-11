# VTK Python wrappers now support overriding

VTK now supports overriding wrapped classes with Python
subclasses. This will enable developers to provide more
Python friendly interface for certain classes. Below is
a brief example of use:

```
@vtk.vtkPoints.override
class foo(vtk.vtkPoints):
  pass

o = vtk.vtkPoints() # o is actually an instace of foo
```

Notes:
* The Python subclasses cannot override virtual functions defined in C++.
* A VTK class has to be overridden by a python subclass. Having a C++ subclass in between is not supported.
* The override is global. It will effect all future instances of the overridden class.
* If a class is overridden in C++ through the object factory mechanism, the python override has to replace the actual class created.

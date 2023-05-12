# Python

## Native Python documentation
Python-style documentation is available for the following packages:

```{toctree}
:titlesonly:
:maxdepth: 2

python/vtkmodules/vtkmodules
```

## Doxygen-style documentation
VTK is implemented in C++ and it is made available in Python via its Python Wrappers.
Although, the VTK doxygen [documentation](http://vtk.org/doc/nightly/html) is derived from the C++ API, the corresponding Python API uses the same classes and methods.
There are however some conventions in place for how wrapping is constructed. To quickly inspect the available methods of a class you can use the `help` method:
```python
>> import vtk
help(vtk.vtkSphereSource)

Help on vtkSphereSource object:

class vtkSphereSource(vtkmodules.vtkCommonExecutionModel.vtkPolyDataAlgorithm)
 |  vtkSphereSource - create a polygonal sphere centered at the origin
 |
 |  Superclass: vtkPolyDataAlgorithm
 |
 |  vtkSphereSource creates a sphere (represented by polygons) of
 |  specified radius centered at the origin. The resolution (polygonal
 |  discretization) in both the latitude (phi) and longitude (theta)
 |  directions can be specified. It also is possible to create partial
 |  spheres by specifying maximum phi and theta angles. By default, the
 |  surface tessellation of the sphere uses triangles; however you can
 |  set LatLongTessellation to produce a tessellation using
 |  quadrilaterals.
 |
 |  @warning
 |  Resolution means the number of latitude or longitude lines for a
 |  complete sphere. If you create partial spheres the number of
 |  latitude/longitude lines may be off by one.
 |
 |  Method resolution order:
 |      vtkSphereSource
 |      vtkmodules.vtkCommonExecutionModel.vtkPolyDataAlgorithm
 |      vtkmodules.vtkCommonExecutionModel.vtkAlgorithm
 |      vtkmodules.vtkCommonCore.vtkObject
 |      vtkmodules.vtkCommonCore.vtkObjectBase
 |      builtins.object
 |
 |  Methods defined here:
 |
 |  GenerateNormalsOff(...)
 |      GenerateNormalsOff(self) -> None
 |      C++: virtual void GenerateNormalsOff()
 |
 |  GenerateNormalsOn(...)
 |      GenerateNormalsOn(self) -> None
 |      C++: virtual void GenerateNormalsOn()
 |
 |  GetCenter(...)
 |      GetCenter(self) -> (float, float, float)
 |      C++: virtual double *GetCenter()
...
```


For a more in-depth description of the Python Wrappers see the dedicated [section](../advanced/PythonWrappers.md).

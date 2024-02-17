## Added new Python API for connecting pipelines

A new (Python-only) API to connect and execute pipelines
was introduced. Pipeline objects can now be connected
with the `>>` operator. For example, the following
```
s = vtkSphereSource()
sr = vtkShrinkFilter()
sr.SetInputConnection(s.GetOutputPort())
```
can now be expressed as
```
s = vtkSphereSource()
sr = vtkShrinkFilter()
s >> sr
```
It is also possible to do the following:
```
pipeline = vtkSphereSource() >> vtkShrinkFilter()
```
Pipelines can also be executed more easily. For
example:
```
result = (vtkSphereSource() >> vtkShrinkFilter()).update().output
```
We also introduced an API to use filters in a more functional way:
```
result = vtkShrinkFilter(shrink_factor=0.5)(input_data)
```

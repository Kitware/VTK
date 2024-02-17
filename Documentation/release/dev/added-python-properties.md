## Added Python properties

We have made several significant changes to VTK's Python
wrappers to support a more Pythonic API. One change
is automatic wrapping of Set/Get methods into Python
properties and adding support for setting these properties
in the objects' constructors. The following
```
obj = vtkFoo()
obj.SetBarValue(12)
val = obj.GetBarValue()
```
can now be expressed as
```
obj = vtkFoo()
obj.bar_value = 12
val = obj.bar_value
```
or
```
obj = vtkFoo(bar_value=12)
val = obj.bar_value
```

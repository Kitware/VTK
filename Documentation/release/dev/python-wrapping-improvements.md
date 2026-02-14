## Python Wrapping Improvements

### Bracket Notation for vtkAOSDataArrayTemplate

`vtkAOSDataArrayTemplate` now supports bracket notation in Python, matching the
existing syntax for `vtkSOADataArrayTemplate` and `vtkBuffer`:

```python
from vtkmodules.vtkCommonCore import vtkAOSDataArrayTemplate

arr = vtkAOSDataArrayTemplate['float64']()
arr.SetNumberOfComponents(3)
arr.SetNumberOfTuples(10)
```

Use `vtkAOSDataArrayTemplate.keys()` to list all available type strings.

### Multiple Positional Arguments in Constructors

VTK Python constructors now pass all positional arguments through to `__init__`,
enabling Python override classes to define rich constructors. Previously, only a
single SWIG pointer string argument was accepted.

```python
from vtkmodules.vtkCommonCore import vtkCollection

class MyCollection(vtkCollection):
    def __init__(self, size=0, **kwargs):
        self.size = size

vtkCollection.override(MyCollection)
c = vtkCollection(42)  # size=42
```

### Fix override() for Templated Classes

`override()` now works correctly on templated VTK classes (e.g.,
`vtkAOSDataArrayTemplate`, `vtkSOADataArrayTemplate`). Previously, the Python-to-C++
class name translation was missing, causing overrides to silently fail on these types.

### Override Docstring Propagation

When a Python override class is registered via `override()`, its docstring is now
copied to the base VTK type so that `help()` displays the Python documentation
directly.

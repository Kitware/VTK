## Pythonic API for vtkStringArray

`vtkStringArray` now supports a Python list-like interface:

```python
from vtkmodules.vtkCommonCore import vtkStringArray

arr = vtkStringArray(["hello", "world"])  # construct from iterable

arr = vtkStringArray()
arr.append("hello")
arr.extend(["a", "b", "c"])
len(arr)          # 4
arr[0]            # "hello"
arr[-1]           # "c"
arr[1] = "world"  # SetValue
"hello" in arr    # True
list(arr)         # ["hello", "world", "b", "c"]
arr.clear()       # Initialize
```

The traditional VTK API (`GetValue`, `SetValue`, `InsertNextValue`, etc.)
continues to work as before.

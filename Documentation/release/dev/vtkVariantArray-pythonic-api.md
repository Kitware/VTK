## Pythonic list-like API for vtkVariantArray

`vtkVariantArray` now supports a Pythonic list-like interface. Values are
automatically extracted from `vtkVariant` to native Python types (int, float,
str) using `vtkVariantExtract`.

```python
from vtkmodules.vtkCommonCore import vtkVariantArray

arr = vtkVariantArray([1, "hello", 3.14])
arr.append(42)
arr[0]           # 1 (int)
arr[1]           # "hello" (str)
arr[-1]          # 42 (int)
arr[1] = "world"
"world" in arr   # True
list(arr)        # [1, 'world', 3.14, 42]
len(arr)         # 4
arr.clear()
```

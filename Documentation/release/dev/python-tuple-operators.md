## Python operators for vtkTuple-derived types and scaled vtkColor conversions

`vtkTuple`-derived special types (`vtkVector*`, `vtkColor*`, `vtkQuaternion*`)
now support Python arithmetic, equality, and an informative ``repr``.

```python
from vtkmodules.vtkCommonDataModel import vtkVector3d, vtkColor3d, vtkColor3ub

repr(vtkVector3d(1, 2, 3))                     # 'vtkVector3d(1.0, 2.0, 3.0)'
vtkVector3d(1, 2, 3) == vtkVector3d(1, 2, 3)   # True
vtkVector3d(1, 2, 3) + vtkVector3d(4, 5, 6)    # vtkVector3d(5.0, 7.0, 9.0)
2.0 * vtkVector3d(1, 2, 3)                     # vtkVector3d(2.0, 4.0, 6.0)
```

Element-wise ``+``, ``-``, ``*``, ``/`` and unary ``-`` are supported, with
both element-wise and scalar operands. Equality and ``repr`` use the sequence
protocol, so they work for both concrete types and template instantiations.

The concrete `vtkColor*` types also gain scaled conversion methods so callers
no longer have to write the `× 255` / clamp / round arithmetic themselves.

```python
vtkColor3ub(255, 128, 0).ToFloat()             # vtkColor3f(1.0, 0.502, 0.0)
vtkColor3d(1.0, 0.5, 0.0).ToUnsignedChar()     # vtkColor3ub(255, 128, 0)
```

Conversions between `[0, 255]` integer and `[0, 1]` floating-point
representations clamp and round; same-precision conversions are plain casts.

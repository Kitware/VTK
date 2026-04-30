## Pythonic API for vtkMatrix3x3 and vtkMatrix4x4

`vtkMatrix3x3` and `vtkMatrix4x4` now expose a Pythonic API that
replaces the verbose `GetElement` / `SetElement` / `Multiply3x3` /
`Invert` patterns with natural Python indexing, slicing, and
operators. The new behavior is enabled automatically when
`vtkCommonMath` is loaded — no extra import is needed.

### Construction from data

The constructors accept a nested sequence (list of rows) or a flat
sequence of `n*n` elements, in addition to the existing no-arg
constructor:

```python
from vtkmodules.vtkCommonMath import vtkMatrix4x4

# Nested
m = vtkMatrix4x4([[1, 0, 0, 1],
                  [0, 1, 0, 2],
                  [0, 0, 1, 3],
                  [0, 0, 0, 1]])

# Flat (16 elements, row-major)
m = vtkMatrix4x4((1, 0, 0, 1, 0, 1, 0, 2, 0, 0, 1, 3, 0, 0, 0, 1))

# Empty (identity), as before
m = vtkMatrix4x4()
```

### Element, row, and 2-D slice indexing

Single-element access, full-row access, and arbitrary 2-D slicing all
work. Slices and rows are returned as numpy arrays so they can flow
straight into numpy code; scalars come back as plain floats. Negative
indices count from the end:

```python
m[0, 3]              # 1.0  — single element
m[0, 3] = 5.0        # SetElement(0, 3, 5.0)

m[1]                 # array([0.0, 1.0, 0.0, 2.0])  — full row
m[1] = [0, 0, 0, 0]  # set entire row

m[-1, -1]            # last element via negative index

m[0:2]               # 2x4 array of the first two rows
m[:, 3]              # length-4 column vector (translation column)
m[1:3, 1:3]          # 2x2 sub-matrix
m[1:3, 1:3] = np.eye(2)  # assign a 2-D block
```

Assignment accepts any value broadcastable to the targeted shape:
scalar, 1-D, or 2-D. Mismatched shapes raise `ValueError` with the
expected vs actual shape.

### Operators

```python
len(m)               # 3 or 4 — matrix dimension

a == b               # element-wise equality
a != b

c = a @ b            # matrix multiply (Multiply3x3 / Multiply4x4
                     #   under the hood; new matrix returned)

inv = ~m             # in-place-safe Invert(m, result), returns a new matrix
```

`==` and `@` return `NotImplemented` for foreign types so Python's
fallback machinery runs (giving canonical TypeErrors instead of
ours).

### Informative `repr`

`repr(m)` produces a copy-paste-able expression that reconstructs the
matrix:

```python
>>> vtkMatrix4x4([[1, 0, 0, 1], [0, 1, 0, 2], [0, 0, 1, 3], [0, 0, 0, 1]])
vtkMatrix4x4([[1.0, 0.0, 0.0, 1.0], [0.0, 1.0, 0.0, 2.0], [0.0, 0.0, 1.0, 3.0], [0.0, 0.0, 0.0, 1.0]])
```

### Backwards compatibility

The classic API (`GetElement`, `SetElement`, `Multiply3x3`, `Multiply4x4`,
`Invert`, `Identity`, `DeepCopy`, …) continues to work unchanged. The
Pythonic features are additive and are installed via the standard
`@vtkClassName.override` mechanism.

## numpy type mapping now follows numpy's underlying C type

`vtkmodules.util.numpy_support` and the Python template subscript operator now
map numpy dtypes to VTK types by the dtype's underlying **C type** (its
`dtype.char`) rather than by a hardwired table of numpy scalar types. numpy
identifies `int64` as whichever C type is 64 bits wide on the platform, so the
mapping now agrees with numpy's own notion of the type.

This is a deliberate correctness change, but it is observable on LP64 platforms
(Linux and macOS) and it changes which VTK array class you get back. Read the
migration section below if you have code that branches on VTK array types.

### What changed on LP64

On LP64 platforms C `long` is 64 bits, and numpy's `int64` *is* C `long`
(`numpy.dtype(numpy.int64).char == 'l'`). numpy's `longlong` is a separate type
with char `'q'`. Previously the mapping ignored this and sent `int64` to
`VTK_LONG_LONG` unconditionally.

| Input dtype | Before (all platforms) | Now, LP64 (Linux/macOS) | Now, LLP64 (Windows) |
| --- | --- | --- | --- |
| `numpy.int64` | `VTK_LONG_LONG` (16) | `VTK_LONG` (8) | `VTK_LONG_LONG` (16) |
| `numpy.uint64` | `VTK_UNSIGNED_LONG_LONG` (17) | `VTK_UNSIGNED_LONG` (9) | `VTK_UNSIGNED_LONG_LONG` (17) |
| `numpy.longlong` | `VTK_LONG_LONG` (16) | `VTK_LONG_LONG` (16) | `VTK_LONG_LONG` (16) |

Concretely, on Linux and macOS:

```python
>>> numpy_to_vtk(numpy.zeros(10, dtype=numpy.int64))
# before: vtkTypeInt64Array
# now:    vtkLongArray
```

Note the previous class was `vtkTypeInt64Array`, **not** `vtkLongLongArray`:
`VTK_TYPE_INT64` and `VTK_LONG_LONG` are the same type code, and
`vtkDataArray::CreateDataArray` instantiates `vtkTypeInt64Array` for it.

Windows behavior is unchanged, and `numpy.longlong` still yields a
`vtkTypeInt64Array` on every platform.

### What this breaks

The data is unaffected — both types are 64 bits wide, and arrays round-trip back
to the same numpy dtype without loss. Only **type identity** changed, so the
checks that break are the ones that test for a class or a type code:

```python
arr = numpy_to_vtk(numpy.zeros(10, dtype=numpy.int64))

isinstance(arr, vtkTypeInt64Array)      # was True, now False on LP64
arr.GetDataType() == VTK_TYPE_INT64     # was True, now False on LP64
arr.GetDataType() == VTK_LONG_LONG      # was True, now False on LP64
```

`vtkTypeInt64Array` derives from `vtkAOSDataArrayTemplate<long long>` while
`vtkLongArray` derives from `vtkAOSDataArrayTemplate<long>`, and there is no
subclass relationship between them. An `isinstance` check against either one
will not accept the other:

```python
issubclass(vtkTypeInt64Array, vtkAOSDataArrayTemplate[numpy.int64])   # False
issubclass(vtkLongArray,      vtkAOSDataArrayTemplate[numpy.int64])   # True
```

### Migrating

Prefer width-based checks over class or type-code identity, since they are
correct on every platform and under either convention:

```python
# instead of: isinstance(arr, vtkTypeInt64Array)
arr.GetDataTypeSize() == 8 and arr.GetDataType() in (VTK_LONG, VTK_LONG_LONG)

# or accept either class explicitly
isinstance(arr, (vtkTypeInt64Array, vtkLongArray))

# or go through numpy, where the two compare equal
vtk_to_numpy(arr).dtype == numpy.int64
```

If you need a `vtkTypeInt64Array` specifically, ask for it by VTK type rather
than by numpy dtype, e.g. `create_vtk_array(VTK_TYPE_INT64)`, or build from
`numpy.longlong` instead of `numpy.int64`.

### Template subscripting follows the same rule

Indexing a wrapped template by a numpy scalar type and by the equivalent
`numpy.dtype` instance now resolve to the same instantiation. Previously the two
spellings could disagree on LP64, returning incompatible classes:

```python
vtkAOSDataArrayTemplate[numpy.int64] is vtkAOSDataArrayTemplate[numpy.dtype('int64')]
# now True; could be False before

vtkAOSDataArrayTemplate[numpy.int64]     # -> vtkAOSDataArrayTemplate<long>      on LP64
vtkAOSDataArrayTemplate[numpy.longlong]  # -> vtkAOSDataArrayTemplate<long long>
```

This keeps template subscripting consistent with `numpy_to_vtk`: the class that
`numpy_to_vtk` returns for a given dtype is always an instance of the template
that same dtype selects.

### A note on reading numpy dtypes at the prompt

numpy gives both `long` and `longlong` the `name` `"int64"` on LP64, and dtype
equality compares by kind and width, so the two are hard to tell apart by eye:

```python
>>> numpy.dtype(numpy.int64).name, numpy.dtype(numpy.longlong).name
('int64', 'int64')
>>> numpy.dtype(numpy.int64) == numpy.dtype(numpy.longlong)
True
>>> numpy.dtype(numpy.int64).char, numpy.dtype(numpy.longlong).char
('l', 'q')
```

So `repr(arr.dtype)` prints `dtype('int64')` for both, and
`vtkTypeInt64Array().to_numpy()` displays as `int64` even though it is really
`longlong`. Use `dtype.char`, `dtype.type`, or an `is` comparison to tell them
apart. The conversions themselves are consistent in both directions:

| VTK array | `to_numpy()` char | `to_numpy()` scalar type | displays as |
| --- | --- | --- | --- |
| `vtkLongArray` | `'l'` | `numpy.int64` | `dtype('int64')` |
| `vtkTypeInt64Array` | `'q'` | `numpy.longlong` | `dtype('int64')` |

### Removed constants

The following module-level names are gone, along with the runtime size probing
they were derived from:

- `VTK_ID_TYPE_SIZE`, `ID_TYPE_CODE`
- `VTK_LONG_TYPE_SIZE`, `LONG_TYPE_CODE`, `ULONG_TYPE_CODE`

Use `get_numpy_array_type()` instead, which resolves the same information from
the type map:

```python
from vtkmodules.util.numpy_support import get_numpy_array_type
from vtkmodules.vtkCommonCore import VTK_ID_TYPE, VTK_LONG, VTK_UNSIGNED_LONG

id_type_code = get_numpy_array_type(VTK_ID_TYPE)           # was ID_TYPE_CODE
long_type_code = get_numpy_array_type(VTK_LONG)            # was LONG_TYPE_CODE
ulong_type_code = get_numpy_array_type(VTK_UNSIGNED_LONG)  # was ULONG_TYPE_CODE
```

Sizes are available from the dtype, e.g.
`numpy.dtype(get_numpy_array_type(VTK_ID_TYPE)).itemsize` in place of
`VTK_ID_TYPE_SIZE`.

Note that `get_numpy_array_type(VTK_ID_TYPE)` resolves to `numpy.longlong`,
whereas the old `ID_TYPE_CODE` was `numpy.int64`. Per the section above the two
compare equal as dtypes, have the same width, and interoperate in arithmetic,
but they are distinct scalar types on LP64. Code that compared the constant by
identity, or keyed off its `char`, should use the dtype comparison instead.

### `get_vtk_to_numpy_typemap()` returns a copy

`get_vtk_to_numpy_typemap()` again returns a copy of the VTK-to-numpy mapping
rather than the module's own dictionary, restoring its earlier behavior.
Mutating the returned dictionary no longer perturbs conversions elsewhere in the
process.

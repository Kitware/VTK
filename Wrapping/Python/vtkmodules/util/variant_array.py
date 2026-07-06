"""Pythonic interface for vtkVariantArray.

Makes vtkVariantArray behave like a Python list of heterogeneous values::

    arr = vtkVariantArray()
    arr.append(42)
    arr.append("hello")
    arr.append(3.14)
    len(arr)                      # 3
    arr[0]                        # 42
    arr[-1]                       # 3.14
    arr[1] = "world"              # SetValue
    "world" in arr                # True
    list(arr)                     # [42, 'world', 3.14]
    repr(arr)                     # vtkVariantArray([42, 'world', 3.14])

Values are automatically extracted from vtkVariant to native Python types
using vtkVariantExtract.
"""

from vtkmodules.vtkCommonCore import vtkVariantArray

_extract = None

def _get_extract():
    global _extract
    if _extract is None:
        from vtkmodules.util.vtkVariant import vtkVariantExtract
        _extract = vtkVariantExtract
    return _extract


class _VariantArrayMixin:
    def __init__(self, *args, **kwargs):
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip mixin init to avoid clobbering state.
        if args and isinstance(args[0], str):
            return
        super().__init__(**kwargs)
        if args:
            self._init_from_data(args[0])

    def _init_from_data(self, data):
        """Populate the array from an iterable of values."""
        for item in data:
            self.InsertNextValue(item)

    # ---- sequence protocol --------------------------------------------------
    def __len__(self):
        return self.GetNumberOfValues()

    def __getitem__(self, index):
        n = self.GetNumberOfValues()
        if index < 0:
            index += n
        if index < 0 or index >= n:
            raise IndexError(
                "vtkVariantArray index %d out of range for size %d" % (index, n))
        return _get_extract()(self.GetValue(index))

    def __setitem__(self, index, value):
        n = self.GetNumberOfValues()
        if index < 0:
            index += n
        if index < 0 or index >= n:
            raise IndexError(
                "vtkVariantArray index %d out of range for size %d" % (index, n))
        self.SetValue(index, value)

    def __contains__(self, value):
        return self.LookupValue(value) >= 0

    def __iter__(self):
        for i in range(self.GetNumberOfValues()):
            yield _get_extract()(self.GetValue(i))

    # ---- repr ---------------------------------------------------------------
    def __repr__(self):
        n = self.GetNumberOfValues()
        items = [_get_extract()(self.GetValue(i)) for i in range(min(10, n))]
        if n <= 10:
            return "vtkVariantArray(%r)" % items
        return "vtkVariantArray(%r, ... %d total)" % (items, n)

    # ---- list-like mutators -------------------------------------------------
    def append(self, value):
        """Append a value to the end of the array."""
        self.InsertNextValue(value)

    def extend(self, iterable):
        """Extend the array with values from an iterable."""
        for v in iterable:
            self.InsertNextValue(v)

    def clear(self):
        """Remove all values from the array."""
        self.Initialize()


@vtkVariantArray.override
class VariantArray(_VariantArrayMixin, vtkVariantArray):
    pass

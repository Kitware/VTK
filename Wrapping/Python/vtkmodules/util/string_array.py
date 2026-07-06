"""Pythonic interface for vtkStringArray.

Makes vtkStringArray behave like a Python list of strings::

    arr = vtkStringArray()
    arr.append("hello")           # InsertNextValue
    arr.extend(["a", "b", "c"])   # InsertNextValue loop
    len(arr)                      # 4
    arr[0]                        # "hello"
    arr[-1]                       # "c"
    arr[1] = "world"              # SetValue
    "hello" in arr                # True
    list(arr)                     # ["hello", "world", "b", "c"]
    repr(arr)                     # vtkStringArray(['hello', 'world', 'b', 'c'])
"""

from vtkmodules.vtkCommonCore import vtkStringArray


class _StringArrayMixin:
    def __init__(self, *args, **kwargs):
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip mixin init to avoid clobbering state.
        if args and isinstance(args[0], str):
            return
        super().__init__(**kwargs)
        if args:
            self._init_from_data(args[0])

    def _init_from_data(self, data):
        """Populate the array from an iterable of strings."""
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
                "vtkStringArray index %d out of range for size %d" % (index, n))
        return self.GetValue(index)

    def __setitem__(self, index, value):
        n = self.GetNumberOfValues()
        if index < 0:
            index += n
        if index < 0 or index >= n:
            raise IndexError(
                "vtkStringArray index %d out of range for size %d" % (index, n))
        self.SetValue(index, value)

    def __contains__(self, value):
        return self.LookupValue(value) >= 0

    def __iter__(self):
        for i in range(self.GetNumberOfValues()):
            yield self.GetValue(i)

    # ---- repr ---------------------------------------------------------------
    def __repr__(self):
        n = self.GetNumberOfValues()
        items = [self.GetValue(i) for i in range(min(10, n))]
        if n <= 10:
            return "vtkStringArray(%r)" % items
        return "vtkStringArray(%r, ... %d total)" % (items, n)

    # ---- list-like mutators -------------------------------------------------
    def append(self, value):
        """Append a string to the end of the array."""
        self.InsertNextValue(value)

    def extend(self, iterable):
        """Extend the array with strings from an iterable."""
        for v in iterable:
            self.InsertNextValue(v)

    def clear(self):
        """Remove all strings from the array."""
        self.Initialize()


@vtkStringArray.override
class StringArray(_StringArrayMixin, vtkStringArray):
    pass

"""Dictionary interface for vtkDataArraySelection.

Makes vtkDataArraySelection behave like a Python dictionary (str -> bool)::

    sel = reader.GetPointDataArraySelection()
    sel["Pressure"]              # True/False
    sel["Pressure"] = True       # EnableArray
    sel["Pressure"] = False      # DisableArray
    "Pressure" in sel            # True
    len(sel)                     # number of arrays
    del sel["Pressure"]          # RemoveArrayByName
    for name in sel: ...         # iterate array names
"""

from vtkmodules.vtkCommonCore import vtkDataArraySelection


class _DataArraySelectionMixin:
    def __getitem__(self, name):
        if not self.ArrayExists(name):
            raise KeyError(name)
        return bool(self.GetArraySetting(name))

    def __setitem__(self, name, value):
        self.SetArraySetting(name, int(bool(value)))

    def __delitem__(self, name):
        if not self.ArrayExists(name):
            raise KeyError(name)
        self.RemoveArrayByName(name)

    def __contains__(self, name):
        return bool(self.ArrayExists(name))

    def __len__(self):
        return self.GetNumberOfArrays()

    def __iter__(self):
        return iter(self.keys())

    def keys(self):
        return [self.GetArrayName(i) for i in range(self.GetNumberOfArrays())]

    def values(self):
        return [bool(self.GetArraySetting(i)) for i in range(self.GetNumberOfArrays())]

    def items(self):
        return [
            (self.GetArrayName(i), bool(self.GetArraySetting(i)))
            for i in range(self.GetNumberOfArrays())
        ]

    def __repr__(self):
        n = self.GetNumberOfArrays()
        enabled = sum(1 for i in range(n) if self.GetArraySetting(i))
        return f"vtkDataArraySelection({n} arrays, {enabled} enabled)"


@vtkDataArraySelection.override
class DataArraySelection(_DataArraySelectionMixin, vtkDataArraySelection):
    pass

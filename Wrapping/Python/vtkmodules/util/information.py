"""Dictionary interface for vtkInformation.

Makes vtkInformation behave like a Python dictionary (key -> value)::

    from vtkmodules.vtkCommonDataModel import vtkDataObject
    info = vtkInformation()
    info[vtkDataObject.FIELD_NAME()] = "Pressure"
    info[vtkDataObject.FIELD_NAME()]           # "Pressure"
    vtkDataObject.FIELD_NAME() in info         # True
    del info[vtkDataObject.FIELD_NAME()]       # Remove entry
    len(info)                                  # entry count
    for key in info: ...                       # iterate keys
    info.keys(), info.values(), info.items()   # dict views
"""

from vtkmodules.vtkCommonCore import (
    vtkInformation,
    vtkInformationIterator,
    vtkInformationDataObjectKey,
    vtkInformationDoubleKey,
    vtkInformationDoubleVectorKey,
    vtkInformationIdTypeKey,
    vtkInformationInformationKey,
    vtkInformationInformationVectorKey,
    vtkInformationIntegerKey,
    vtkInformationIntegerPointerKey,
    vtkInformationIntegerVectorKey,
    vtkInformationKeyVectorKey,
    vtkInformationObjectBaseKey,
    vtkInformationObjectBaseVectorKey,
    vtkInformationRequestKey,
    vtkInformationStringKey,
    vtkInformationStringVectorKey,
    vtkInformationUnsignedLongKey,
    vtkInformationVariantKey,
    vtkInformationVariantVectorKey,
)

# Scalar keys: Get(key) -> value, Set(key, value)
_SCALAR_KEY_TYPES = (
    vtkInformationIntegerKey,
    vtkInformationIdTypeKey,
    vtkInformationDoubleKey,
    vtkInformationUnsignedLongKey,
    vtkInformationStringKey,
    vtkInformationVariantKey,
)

# Vector keys: Get(key, i), Length(key), Append(key, v)
_VECTOR_KEY_TYPES = (
    vtkInformationIntegerVectorKey,
    vtkInformationDoubleVectorKey,
    vtkInformationStringVectorKey,
    vtkInformationVariantVectorKey,
)

# Object keys: Get(key) -> vtkObject, Set(key, obj)
_OBJECT_KEY_TYPES = (
    vtkInformationDataObjectKey,
    vtkInformationObjectBaseKey,
    vtkInformationInformationKey,
    vtkInformationInformationVectorKey,
)

# Object vector keys: Get(key, i), Length(key), Append(key, v)
_OBJECT_VECTOR_KEY_TYPES = (
    vtkInformationObjectBaseVectorKey,
    vtkInformationKeyVectorKey,
)


class _InformationMixin:
    def __getitem__(self, key):
        if not self.Has(key):
            raise KeyError(key)
        if isinstance(key, _SCALAR_KEY_TYPES):
            return self.Get(key)
        if isinstance(key, _VECTOR_KEY_TYPES):
            n = self.Length(key)
            return tuple(self.Get(key, i) for i in range(n))
        if isinstance(key, _OBJECT_KEY_TYPES):
            return self.Get(key)
        if isinstance(key, _OBJECT_VECTOR_KEY_TYPES):
            n = self.Length(key)
            return tuple(self.Get(key, i) for i in range(n))
        if isinstance(key, vtkInformationRequestKey):
            return True
        if isinstance(key, vtkInformationIntegerPointerKey):
            n = self.Length(key)
            return tuple(self.Get(key, i) for i in range(n))
        raise TypeError("Unsupported key type: %s" % type(key).__name__)

    def __setitem__(self, key, value):
        if isinstance(key, _SCALAR_KEY_TYPES):
            self.Set(key, value)
        elif isinstance(key, (vtkInformationIntegerVectorKey,
                              vtkInformationDoubleVectorKey,
                              vtkInformationVariantVectorKey)):
            self.Set(key, tuple(value), len(value))
        elif isinstance(key, vtkInformationStringVectorKey):
            self.Remove(key)
            for v in value:
                self.Append(key, v)
        elif isinstance(key, _OBJECT_KEY_TYPES):
            self.Set(key, value)
        elif isinstance(key, _OBJECT_VECTOR_KEY_TYPES):
            self.Remove(key)
            for v in value:
                self.Append(key, v)
        elif isinstance(key, vtkInformationRequestKey):
            if value:
                self.Set(key)
            else:
                self.Remove(key)
        elif isinstance(key, vtkInformationIntegerPointerKey):
            self.Set(key, tuple(value), len(value))
        else:
            raise TypeError("Unsupported key type: %s" % type(key).__name__)

    def __delitem__(self, key):
        if not self.Has(key):
            raise KeyError(key)
        self.Remove(key)

    def __contains__(self, key):
        return bool(self.Has(key))

    def __len__(self):
        return self.GetNumberOfKeys()

    def __iter__(self):
        it = vtkInformationIterator()
        it.SetInformation(self)
        it.InitTraversal()
        while not it.IsDoneWithTraversal():
            yield it.GetCurrentKey()
            it.GoToNextItem()

    def keys(self):
        return list(self)

    def values(self):
        return [self[k] for k in self]

    def items(self):
        return [(k, self[k]) for k in self]

    def get(self, key, default=None):
        if self.Has(key):
            return self[key]
        return default

    def update(self, other):
        """Append all entries from another vtkInformation."""
        self.Append(other)

    def clear(self):
        self.Clear()

    def __repr__(self):
        entries = []
        for key in self:
            name = key.GetName()
            try:
                val = self[key]
                entries.append("%s: %r" % (name, val))
            except Exception:
                entries.append(name)
            if len(entries) >= 8:
                entries.append("...")
                break
        return "vtkInformation({%s})" % ", ".join(entries)


@vtkInformation.override
class Information(_InformationMixin, vtkInformation):
    pass

"""Pythonic interfaces for vtkInformation and vtkInformationVector.

Makes vtkInformation behave like a Python dictionary (key -> value)::

    from vtkmodules.vtkCommonDataModel import vtkDataObject
    info = vtkInformation()
    info[vtkDataObject.FIELD_NAME()] = "Pressure"
    info[vtkDataObject.FIELD_NAME()]           # "Pressure"
    vtkDataObject.FIELD_NAME() in info         # True
    del info[vtkDataObject.FIELD_NAME()]       # Remove entry
    len(info)                                  # entry count
    for key in info: ...                       # iterate vtkInformationKey objects
    info.keys()                                # list of vtkInformationKey objects
    info.string_keys()                         # list of key name strings
    info.values(), info.items()                # dict-like views

Makes vtkInformationVector behave like a Python list::

    vec = vtkInformationVector()
    vec.append(vtkInformation())
    len(vec)          # 1
    vec[0]            # vtkInformation
    for info in vec:  # iterate
"""

from vtkmodules.vtkCommonCore import (
    vtkInformation,
    vtkInformationVector,
    vtkInformationIterator,
    vtkInformationKeyLookup,
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


def _resolve_key(name_or_key):
    """Resolve a string key name (case-insensitive) to its vtkInformationKey object."""
    if not isinstance(name_or_key, str):
        return name_or_key
    key = vtkInformationKeyLookup.FindByName(name_or_key.upper())
    if key is None:
        raise KeyError(name_or_key)
    return key


class _InformationMixin:
    def __getitem__(self, key):
        key = _resolve_key(key)
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
        key = _resolve_key(key)
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
        key = _resolve_key(key)
        if not self.Has(key):
            raise KeyError(key)
        self.Remove(key)

    def __contains__(self, key):
        # __contains__ must answer True/False, never raise: an unknown key
        # name simply is not present.
        try:
            key = _resolve_key(key)
        except KeyError:
            return False
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
        """Return the list of vtkInformationKey objects in this information."""
        return list(self)

    def values(self):
        return [self[k] for k in self]

    def items(self):
        """Return [(key_object, value), ...] for all entries."""
        return [(k, self[k]) for k in self]

    def string_keys(self):
        """Return the names of the keys in this information as strings."""
        return [k.GetName() for k in self]

    def get(self, key, default=None):
        key = _resolve_key(key)
        if self.Has(key):
            return self[key]
        return default

    def update(self, other):
        """Merge entries from another vtkInformation or a Python mapping."""
        if isinstance(other, vtkInformation):
            self.Append(other)
            return
        if hasattr(other, "items"):
            for k, v in other.items():
                self[k] = v
            return
        for k, v in other:
            self[k] = v

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


class _InformationVectorMixin:
    def __len__(self):
        return self.GetNumberOfInformationObjects()

    def __getitem__(self, index):
        if isinstance(index, slice):
            return [self[i] for i in range(*index.indices(len(self)))]
        if index < 0:
            index += len(self)
        if not 0 <= index < len(self):
            raise IndexError("index %d out of range" % index)
        return self.GetInformationObject(index)

    def __setitem__(self, index, info):
        if index < 0:
            index += len(self)
        if not 0 <= index < len(self):
            raise IndexError("index %d out of range" % index)
        self.SetInformationObject(index, info)

    def __delitem__(self, index):
        if isinstance(index, slice):
            raise TypeError("vtkInformationVector does not support slice deletion")
        if index < 0:
            index += len(self)
        if not 0 <= index < len(self):
            raise IndexError("index %d out of range" % index)
        self.Remove(index)

    def __iter__(self):
        for i in range(len(self)):
            yield self.GetInformationObject(i)

    def __contains__(self, info):
        for i in range(len(self)):
            if self.GetInformationObject(i) == info:
                return True
        return False

    def append(self, info):
        self.Append(info)

    def __repr__(self):
        n = len(self)
        items = [repr(self.GetInformationObject(i)) for i in range(min(n, 8))]
        if n > 8:
            items.append("...")
        return "vtkInformationVector([%s])" % ", ".join(items)


@vtkInformationVector.override
class InformationVector(_InformationVectorMixin, vtkInformationVector):
    pass

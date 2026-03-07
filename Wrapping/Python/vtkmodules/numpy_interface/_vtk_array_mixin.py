"""Shared base mixin and utilities for VTK data array Python wrappers.

Provides:
- ``VTKDataArrayMixin`` — base class with metadata management, shape properties,
  reduction method stubs, and shape/layout methods shared across the VTK
  array mixins (AOS, SOA).
- ``make_override_registry()`` — create an ``__array_function__`` override dict
  and decorator pair.
- ``register_template_overrides()`` — register a mixin as override for all
  dtype instantiations of a VTK template class.
"""
import numpy

from ..util import numpy_support


class VTKDataArrayMixin:
    """Shared base for VTK data array Python mixins (AOS, SOA).

    Subclasses must provide ``__array__()``, ``GetNumberOfTuples()``,
    ``GetNumberOfComponents()``, and ``GetDataType()`` (inherited from the
    VTK base class in the MRO).
    """

    # ---- metadata management ------------------------------------------------
    def _set_dataset(self, dataset):
        """Store a weak reference to the owning dataset."""
        if dataset is not None:
            from ..vtkCommonCore import vtkWeakReference
            self._dataset = vtkWeakReference()
            self._dataset.Set(dataset)
        else:
            self._dataset = None

    @property
    def dataset(self):
        """Return the owning dataset (dereferenced from weak ref)."""
        if self._dataset is not None:
            return self._dataset.Get()
        return None

    @dataset.setter
    def dataset(self, value):
        self._set_dataset(value)

    @property
    def association(self):
        """Return the association type (POINT, CELL, etc.)."""
        return self._association

    @association.setter
    def association(self, value):
        self._association = value

    # ---- _wrap_result (AOS — NOT SOA) ----------------------------------------
    def _wrap_result(self, result):
        """Wrap a numpy result as VTKAOSArray preserving metadata.

        Only wraps if result is an ndarray with same number of tuples.
        Returns plain ndarray/scalar otherwise.
        """
        if not isinstance(result, numpy.ndarray) or result.ndim == 0:
            return result
        if result.shape[0] != self.GetNumberOfTuples():
            return result
        # VTK has no bool type; use int8 view (binary compatible)
        vtk_input = result.view(numpy.int8) if result.dtype == numpy.bool_ else result
        vtk_arr = numpy_support.numpy_to_vtk(vtk_input)
        vtk_arr._dataset = self._dataset
        vtk_arr._association = self._association
        return vtk_arr

    # ---- shape properties ---------------------------------------------------
    @property
    def shape(self):
        nc = self.GetNumberOfComponents()
        nt = self.GetNumberOfTuples()
        return (nt,) if nc == 1 else (nt, nc)

    @property
    def ndim(self):
        return len(self.shape)

    @property
    def size(self):
        return self.GetNumberOfTuples() * self.GetNumberOfComponents()

    @property
    def T(self):
        return self.__array__().T

    # ---- hash (needed since all mixins define __eq__) -----------------------
    __hash__ = None

    # ---- __len__ ------------------------------------------------------------
    def __len__(self):
        return self.GetNumberOfTuples()

    # ---- reduction methods (delegate to numpy free functions) ----------------
    def sum(self, axis=None, **kwargs):
        return numpy.sum(self, axis=axis, **kwargs)

    def mean(self, axis=None, **kwargs):
        return numpy.mean(self, axis=axis, **kwargs)

    def min(self, axis=None, **kwargs):
        return numpy.min(self, axis=axis, **kwargs)

    def max(self, axis=None, **kwargs):
        return numpy.max(self, axis=axis, **kwargs)

    def std(self, axis=None, **kwargs):
        return numpy.std(self, axis=axis, **kwargs)

    def var(self, axis=None, **kwargs):
        return numpy.var(self, axis=axis, **kwargs)

    def any(self, axis=None, **kwargs):
        return numpy.any(self, axis=axis, **kwargs)

    def all(self, axis=None, **kwargs):
        return numpy.all(self, axis=axis, **kwargs)

    def prod(self, axis=None, **kwargs):
        return numpy.prod(self, axis=axis, **kwargs)

    def argmin(self, axis=None, **kwargs):
        return numpy.argmin(self, axis=axis, **kwargs)

    def argmax(self, axis=None, **kwargs):
        return numpy.argmax(self, axis=axis, **kwargs)

    def cumsum(self, axis=None, **kwargs):
        return numpy.cumsum(self, axis=axis, **kwargs)

    def cumprod(self, axis=None, **kwargs):
        return numpy.cumprod(self, axis=axis, **kwargs)

    # ---- shape / layout methods (materialize via __array__) -----------------
    def reshape(self, *args, **kwargs):
        return self.__array__().reshape(*args, **kwargs)

    def flatten(self, order='C'):
        return self.__array__().flatten(order=order)

    def ravel(self, order='C'):
        return self.__array__().ravel(order=order)

    def copy(self, order='C'):
        return self.__array__().copy(order=order)

    def squeeze(self, axis=None):
        return self.__array__().squeeze(axis=axis)

    def transpose(self, *axes):
        return self.__array__().transpose(*axes)

    def tolist(self):
        return self.__array__().tolist()

    def clip(self, a_min=None, a_max=None, **kwargs):
        return numpy.clip(self.__array__(), a_min, a_max, **kwargs)

    def round(self, decimals=0, **kwargs):
        return numpy.round(self.__array__(), decimals=decimals, **kwargs)

    def sort(self, axis=0, **kwargs):
        return numpy.sort(self.__array__(), axis=axis, **kwargs)

    def dot(self, other):
        return numpy.dot(self, other)

    def astype(self, dtype):
        return self.__array__().astype(dtype)


def make_override_registry():
    """Create an override dict + decorator for ``__array_function__`` dispatch.

    Returns ``(registry_dict, register_decorator)``.

    Usage::

        MY_OVERRIDE, _override_mine = make_override_registry()

        @_override_mine(numpy.sum)
        def _my_sum(a, axis=None, **kw): ...
    """
    registry = {}

    def register(numpy_function):
        def decorator(func):
            registry[numpy_function] = func
            return func
        return decorator

    return registry, register


def register_template_overrides(mixin_cls, template_cls, name_prefix,
                                concrete_classes=None):
    """Register a mixin as override for all dtype instantiations of a VTK template.

    Parameters
    ----------
    mixin_cls : type
        The Python mixin class (e.g. VTKAOSArray).
    template_cls : VTK template class
        The VTK template (e.g. vtkAOSDataArrayTemplate).
    name_prefix : str
        Prefix for the generated class names (e.g. 'VTKAOSArray').
    concrete_classes : list of type, optional
        Additional concrete VTK classes to override (e.g. vtkFloatArray).
    """
    _dtype_strings = [
        'float32', 'float64',
        'int8', 'int16', 'int32', 'int64',
        'uint8', 'uint16', 'uint32', 'uint64',
    ]
    for dt in _dtype_strings:
        base = template_cls[dt]
        cls = type(f'{name_prefix}_{dt}', (mixin_cls, base),
                  {'__doc__': mixin_cls.__doc__})
        base.override(cls)

    if concrete_classes:
        for base in concrete_classes:
            cls = type(f'{name_prefix}_{base.__name__}', (mixin_cls, base),
                      {'__doc__': mixin_cls.__doc__})
            base.override(cls)

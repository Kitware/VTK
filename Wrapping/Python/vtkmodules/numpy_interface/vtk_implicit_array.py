"""VTKImplicitArray — fallback numpy-compatible mixin for implicit VTK arrays.

VTK implicit arrays (vtkImplicitArray<BackendT>) compute values on the fly
from a backend functor rather than storing them in memory.  Many backends are
defined locally in C++ implementation files and are never exposed to the
Python wrapping system.  When such an array crosses from C++ to Python, VTK's
``FindNearestBaseClass`` resolves it to ``vtkDataArray`` — the nearest wrapped
ancestor.

This module registers a fallback mixin on ``vtkDataArray`` so that *any*
data array type that does not have a more-specific override (AOS, SOA, or a
dedicated implicit-array override) still gets a working numpy interface.

The mixin materialises values by deep-copying to an AOS array on first access
and caches the result.  A ``ModifiedEvent`` observer invalidates the cache so
that re-reads after the underlying data changes produce correct results.

Notes
-----
Because AOS overrides are registered on concrete types (``vtkFloatArray``,
``vtkAOSDataArrayTemplate[float32]``, etc.) and SOA overrides on
``vtkSOADataArrayTemplate[...]``, those always take precedence over this
``vtkDataArray``-level fallback.
"""
import numpy

from ..vtkCommonCore import vtkCommand
from ..util import numpy_support
from ._vtk_array_mixin import VTKDataArrayMixin

_UNINITIALIZED = object()


class VTKImplicitArray(VTKDataArrayMixin):
    """Fallback numpy-compatible mixin for implicit/unknown VTK data arrays.

    Materialises the array to a contiguous AOS copy (via ``DeepCopy``) on
    first numpy access and caches the result.  The cache is invalidated
    when VTK fires ``ModifiedEvent`` on the array.

    This mixin is automatically applied to any ``vtkDataArray`` subclass
    that does not already have a more specific override (AOS or SOA).

    Properties
    ----------
    shape : tuple of int
        ``(ntuples,)`` for 1-component, ``(ntuples, ncomps)`` otherwise.
    dtype : numpy.dtype
        The element data type.
    dataset : vtkDataSet or None
        The owning VTK dataset, when the array is attached to one.
    association : int or None
        The attribute association (POINT, CELL, etc.).

    Examples
    --------
    Implicit arrays from VTK are automatically wrapped::

        implicit_arr = some_filter.GetOutput().GetPointData().GetArray(0)
        print(np.sum(implicit_arr))   # materialises once, then cached

    See Also
    --------
    VTKAOSArray : Mixin for array-of-structures VTK arrays (zero-copy).
    VTKSOAArray : Mixin for structure-of-arrays VTK arrays (zero-copy).
    """

    # ---- construction -------------------------------------------------------
    def __init__(self, *args, **kwargs):
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip mixin init to avoid clobbering state.
        if args and isinstance(args[0], str):
            return
        super().__init__(**kwargs)
        self._array_cache = _UNINITIALIZED
        self._observer_id = None
        self._dataset = None
        self._association = None

    # ---- lazy materialisation -----------------------------------------------
    def _materialise(self):
        """Deep-copy this array into a contiguous AOS numpy array."""
        nt = self.GetNumberOfTuples()
        nc = self.GetNumberOfComponents()
        dt = numpy_support.get_numpy_array_type(self.GetDataType())
        if nt == 0:
            return numpy.empty((0,) if nc == 1 else (0, nc), dtype=dt)

        # Create an explicit AOS array and deep-copy into it.
        # DeepCopy handles the implicit -> AOS conversion on the C++ side.
        from ..vtkCommonCore import vtkAOSDataArrayTemplate
        target = vtkAOSDataArrayTemplate[dt]()
        target.DeepCopy(self)
        arr = numpy_support.vtk_to_numpy(target)
        # Keep target alive so the numpy view remains valid.
        # Store on self since numpy arrays don't support arbitrary attrs.
        self._materialised_vtk_ref = target
        return arr

    @property
    def _array_view(self):
        if self._array_cache is _UNINITIALIZED:
            self._array_cache = self._materialise()
            self._setup_observer()
        return self._array_cache

    def _setup_observer(self):
        """Invalidate the cached numpy array when the VTK array is modified."""
        if self._observer_id is not None:
            return
        observer_id_holder = [None]

        def on_modified(vtk_obj, event):
            vtk_obj._array_cache = _UNINITIALIZED
            vtk_obj._observer_id = None
            if observer_id_holder[0] is not None:
                vtk_obj.RemoveObserver(observer_id_holder[0])
                observer_id_holder[0] = None

        observer_id = self.AddObserver(vtkCommand.ModifiedEvent, on_modified)
        observer_id_holder[0] = observer_id
        self._observer_id = observer_id_holder

    # ---- numpy-like properties ----------------------------------------------
    @property
    def dtype(self):
        view = self._array_view
        if view is not None and hasattr(view, 'dtype'):
            return view.dtype
        return numpy_support.get_numpy_array_type(self.GetDataType())

    @property
    def nbytes(self):
        return self.GetNumberOfTuples() * self.GetNumberOfComponents() * self.GetDataTypeSize()

    # ---- buffer protocol (PEP 3118) -----------------------------------------
    def __buffer__(self, flags):
        """Expose the materialised AOS copy via the buffer protocol.

        This allows ``numpy.frombuffer(implicit_arr)`` and
        ``vtk_to_numpy(implicit_arr)`` to work transparently.
        """
        return memoryview(self._array_view)

    # ---- numpy protocol -----------------------------------------------------
    def __array__(self, dtype=None, copy=None):
        arr = self._array_view
        if arr is None:
            nc = self.GetNumberOfComponents()
            dt = dtype or numpy_support.get_numpy_array_type(self.GetDataType())
            return numpy.empty((0,) if nc == 1 else (0, nc), dtype=dt)
        if copy:
            result = arr.copy()
        else:
            result = arr
        if dtype is not None and result.dtype != numpy.dtype(dtype):
            result = result.astype(dtype)
        return result

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        out = kwargs.get('out', None)
        if out is not None:
            return NotImplemented

        converted = [inp.__array__() if isinstance(inp, VTKImplicitArray) else inp
                     for inp in inputs]
        result = getattr(ufunc, method)(*converted, **kwargs)
        if method == '__call__':
            return self._wrap_result(result)
        return result

    def __array_function__(self, func, types, args, kwargs):
        def _convert(arg):
            if isinstance(arg, VTKImplicitArray):
                return arg.__array__()
            if isinstance(arg, (list, tuple)):
                return type(arg)(_convert(x) for x in arg)
            return arg
        converted_args = tuple(_convert(a) for a in args)
        converted_kwargs = {k: _convert(v) for k, v in kwargs.items()}
        return func(*converted_args, **converted_kwargs)

    # ---- indexing -----------------------------------------------------------
    def __getitem__(self, index):
        return self.__array__()[index]

    def __setitem__(self, index, value):
        # Implicit arrays are conceptually read-only, but if someone
        # has materialised a copy we can write into that cache.
        view = self._array_view
        if view is not None and view.size > 0:
            view[index] = value
        else:
            raise IndexError("Cannot set items on an empty implicit array")

    # ---- arithmetic operators -----------------------------------------------
    def _binop(self, other, op):
        a = self.__array__()
        b = other.__array__() if isinstance(other, VTKImplicitArray) else numpy.asarray(other)
        return self._wrap_result(op(a, b))

    def _rbinop(self, other, op):
        a = self.__array__()
        b = numpy.asarray(other)
        return self._wrap_result(op(b, a))

    def __add__(self, other):       return self._binop(other, numpy.add)
    def __radd__(self, other):      return self._rbinop(other, numpy.add)
    def __sub__(self, other):       return self._binop(other, numpy.subtract)
    def __rsub__(self, other):      return self._rbinop(other, numpy.subtract)
    def __mul__(self, other):       return self._binop(other, numpy.multiply)
    def __rmul__(self, other):      return self._rbinop(other, numpy.multiply)
    def __truediv__(self, other):   return self._binop(other, numpy.true_divide)
    def __rtruediv__(self, other):  return self._rbinop(other, numpy.true_divide)
    def __floordiv__(self, other):  return self._binop(other, numpy.floor_divide)
    def __rfloordiv__(self, other): return self._rbinop(other, numpy.floor_divide)
    def __pow__(self, other):       return self._binop(other, numpy.power)
    def __rpow__(self, other):      return self._rbinop(other, numpy.power)
    def __mod__(self, other):       return self._binop(other, numpy.mod)
    def __rmod__(self, other):      return self._rbinop(other, numpy.mod)

    # comparison
    def __lt__(self, other):  return self._binop(other, numpy.less)
    def __le__(self, other):  return self._binop(other, numpy.less_equal)
    def __eq__(self, other):  return self._binop(other, numpy.equal)
    def __ne__(self, other):  return self._binop(other, numpy.not_equal)
    def __ge__(self, other):  return self._binop(other, numpy.greater_equal)
    def __gt__(self, other):  return self._binop(other, numpy.greater)

    # unary
    def __neg__(self):
        return self._wrap_result(-self.__array__())

    def __abs__(self):
        return self._wrap_result(numpy.abs(self.__array__()))

    # ---- utilities ----------------------------------------------------------
    def __iter__(self):
        arr = self.__array__()
        for i in range(len(arr)):
            yield arr[i]

    def __repr__(self):
        return f"VTKImplicitArray(shape={self.shape}, dtype={self.dtype})"

    def __str__(self):
        return str(self.__array__())


# ---- Register as fallback override on vtkDataArray --------------------------
def _register_implicit_override():
    """Register VTKImplicitArray as override on vtkDataArray.

    This is a fallback: any array type that already has a more-specific
    override (AOS on vtkFloatArray, SOA on vtkSOADataArrayTemplate, etc.)
    is unaffected.  Only types whose nearest wrapped ancestor is
    vtkDataArray will use this mixin.
    """
    from vtkmodules.vtkCommonCore import vtkDataArray

    cls = type('VTKImplicitArray_vtkDataArray',
               (VTKImplicitArray, vtkDataArray),
               {'__doc__': VTKImplicitArray.__doc__})
    vtkDataArray.override(cls)

_register_implicit_override()

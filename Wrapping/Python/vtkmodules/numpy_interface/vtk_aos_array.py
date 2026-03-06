"""VTKAOSArray — numpy-compatible mixin for vtkAOSDataArrayTemplate.

This module provides the VTKAOSArray mixin class and registers overrides
for all vtkAOSDataArrayTemplate instantiations and concrete AOS subclasses
(vtkFloatArray, vtkDoubleArray, etc.) so that AOS arrays coming from VTK
automatically have numpy-compatible operations.  The single contiguous
buffer is exposed as a zero-copy numpy array view.
"""
import numpy

from ..vtkCommonCore import vtkCommand
from ..util import numpy_support
from ._vtk_array_mixin import VTKDataArrayMixin, register_template_overrides

_UNINITIALIZED = object()


def _reshape_for_broadcast(a, b):
    """Reshape for VTK-style broadcasting between arrays of different ndim.

    When a 1-D array of length *n* is combined with a 2-D array of shape
    ``(n, k)``, numpy's standard broadcasting would fail because it aligns
    shapes from the right: ``(n,)`` vs ``(n, k)`` becomes ``(1, n)`` vs
    ``(n, k)``.  VTK arrays expect per-tuple scalars to broadcast across
    components, so we reshape ``(n,)`` to ``(n, 1)`` instead.

    When the 1-D length does **not** match the leading dimension of the
    other operand (e.g. a reduction result of shape ``(k,)`` against
    ``(n, k)``), we leave the shapes alone and let numpy handle it with
    its standard right-aligned broadcasting rules.
    """
    a = numpy.asarray(a) if not isinstance(a, numpy.ndarray) else a
    b = numpy.asarray(b) if not isinstance(b, numpy.ndarray) else b
    if a.ndim == b.ndim:
        return a, b
    if a.ndim < b.ndim:
        # Only reshape if the 1-D length matches the leading dimension
        if a.ndim == 1 and a.shape[0] == b.shape[0]:
            a = a.reshape(a.shape + (1,) * (b.ndim - a.ndim))
    else:
        if b.ndim == 1 and b.shape[0] == a.shape[0]:
            b = b.reshape(b.shape + (1,) * (a.ndim - b.ndim))
    return a, b


def _aos_to_ndarray(x):
    """Convert a VTKAOSArray to its cached ndarray view."""
    if isinstance(x, VTKAOSArray):
        return x.__array__()
    return numpy.asarray(x)


def _aos_to_ndarray_arg(arg):
    """Convert VTKAOSArray to ndarray, recursing into lists/tuples."""
    if isinstance(arg, VTKAOSArray):
        return arg.__array__()
    if isinstance(arg, (list, tuple)):
        return type(arg)(_aos_to_ndarray_arg(x) for x in arg)
    return arg


class VTKAOSArray(VTKDataArrayMixin):
    """A numpy-compatible view of VTK's array-of-structures data arrays.

    VTK stores most numeric data in AOS (array-of-structures) layout:
    all components of a tuple are contiguous in memory.  This mixin
    exposes the underlying buffer as a **zero-copy numpy array view**,
    so that indexing, slicing, and arithmetic work without copying data.

    This mixin is automatically applied to ``vtkAOSDataArrayTemplate``
    instantiations and all concrete AOS subclasses (``vtkFloatArray``,
    ``vtkDoubleArray``, ``vtkIntArray``, etc.).  Any AOS array that
    crosses from C++ to Python is already a ``VTKAOSArray`` instance.

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

    Zero-Copy Access
    ----------------
    The internal VTK buffer is shared with numpy — no data is copied
    when you read array values::

        arr = polydata.point_data["Normals"]
        normals = np.asarray(arr)   # zero-copy view
        normals[:, 2] = 0           # modifies VTK data in place

    Memory Safety
    -------------
    The mixin observes ``BufferChangedEvent`` from VTK.  If the
    underlying buffer is reallocated (e.g. by ``SetNumberOfTuples``),
    the cached numpy view is automatically invalidated to prevent
    use-after-free.

    Arithmetic
    ----------
    Element-wise operations (``+``, ``-``, ``*``, ``/``, comparisons,
    ufuncs) produce new VTK-backed AOS arrays with metadata (``dataset``,
    ``association``) propagated.  Reductions (``sum``, ``mean``, etc.)
    return plain scalars or numpy arrays.

    NumPy Integration
    -----------------
    - ``numpy.asarray(arr)`` returns the zero-copy view directly.
    - All numpy ufuncs and ``__array_function__`` calls are supported.
    - The array implements the buffer protocol (PEP 3118).

    Examples
    --------
    Arrays from VTK are already the right type::

        arr = polydata.point_data["velocity"]
        print(arr.shape)           # (n, 3)
        print(np.sum(arr, axis=0)) # per-component sum

    Create from numpy::

        from vtkmodules.util.numpy_support import numpy_to_vtk
        vtk_arr = numpy_to_vtk(my_array)

    See Also
    --------
    VTKSOAArray : Mixin for structure-of-arrays VTK arrays.
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
        self._buffer_ref = None
        self._dataset = None
        self._association = None
        if args:
            self._init_from_data(args[0])

    def _init_from_data(self, data):
        """Populate this array from an iterable (list, tuple, numpy array, etc.).

        The data is converted to match this array's VTK data type.  1-D and
        2-D inputs are supported; for 2-D inputs the second dimension sets the
        number of components.
        """
        z = numpy.ascontiguousarray(data)
        if z.ndim > 2:
            raise ValueError("Only 1D and 2D arrays are supported")
        if z.size == 0:
            if z.ndim == 2:
                self.SetNumberOfComponents(z.shape[1])
            return
        # Convert to match this array's element type
        arr_dtype = numpy_support.get_numpy_array_type(self.GetDataType())
        if z.dtype != numpy.dtype(arr_dtype):
            z = numpy.ascontiguousarray(z.astype(arr_dtype))
        if z.ndim == 1:
            self.SetNumberOfComponents(1)
        else:
            self.SetNumberOfComponents(z.shape[1])
        z_flat = numpy.ravel(z)
        self.SetVoidArray(z_flat, len(z_flat), 1)
        self.GetBuffer()._numpy_reference = z_flat

    # ---- lazy array view initialization -------------------------------------
    @property
    def _array_view(self):
        if self._array_cache is _UNINITIALIZED:
            self._init_array_view()
        if self._array_cache is _UNINITIALIZED:
            return None
        return self._array_cache

    @_array_view.setter
    def _array_view(self, value):
        self._array_cache = value

    def _init_array_view(self):
        """Extract a zero-copy numpy array view from the VTK buffer."""
        if self.GetNumberOfTuples() == 0:
            # Don't cache — array may be populated later
            return
        self._array_cache = numpy_support.vtk_to_numpy(self)
        self._setup_observer()

    def _setup_observer(self):
        """Set up BufferChangedEvent observer for memory safety."""
        if self._observer_id is not None:
            return
        observer_id_holder = [None]

        def on_buffer_changed(vtk_obj, event):
            vtk_obj._array_cache = _UNINITIALIZED  # invalidate
            vtk_obj._observer_id = None
            if observer_id_holder[0] is not None:
                vtk_obj.RemoveObserver(observer_id_holder[0])
                observer_id_holder[0] = None

        observer_id = self.AddObserver(
            vtkCommand.BufferChangedEvent, on_buffer_changed)
        observer_id_holder[0] = observer_id
        self._observer_id = observer_id_holder

        # Store reference to buffer for memory safety
        buf = self.GetBuffer()
        if buf is not None:
            self._buffer_ref = buf

    # ---- numpy-like properties ----------------------------------------------
    @property
    def dtype(self):
        view = self._array_view
        if view is not None:
            return view.dtype
        return numpy_support.get_numpy_array_type(self.GetDataType())

    @property
    def nbytes(self):
        return self.GetNumberOfTuples() * self.GetNumberOfComponents() * self.GetDataTypeSize()

    # ---- numpy protocol -----------------------------------------------------
    def to_numpy(self, dtype=None):
        """Return the array as a numpy ndarray."""
        return self.__array__(dtype=dtype)

    def __array__(self, dtype=None, copy=None):
        view = self._array_view
        if view is None:
            nt = self.GetNumberOfTuples()
            if nt == 0:
                nc = self.GetNumberOfComponents()
                dt = dtype or numpy_support.get_numpy_array_type(self.GetDataType())
                if nc == 1:
                    return numpy.empty((0,), dtype=dt)
                return numpy.empty((0, nc), dtype=dt)
            # Force init
            self._init_array_view()
            view = self._array_cache
            if view is _UNINITIALIZED:
                return numpy.empty((0,), dtype=dtype or numpy.float64)

        if copy:
            result = view.copy()
        else:
            result = view
        if dtype is not None and result.dtype != numpy.dtype(dtype):
            result = result.astype(dtype)
        return result

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        out = kwargs.get('out', None)
        if out is not None:
            return NotImplemented

        converted = [_aos_to_ndarray(inp) if isinstance(inp, VTKAOSArray) else inp
                     for inp in inputs]
        result = getattr(ufunc, method)(*converted, **kwargs)
        if method == '__call__':
            return self._wrap_result(result)
        return result

    def __array_function__(self, func, types, args, kwargs):
        # Universal fallback: convert all VTKAOSArray args to ndarray and re-call.
        converted_args = tuple(_aos_to_ndarray_arg(a) for a in args)
        converted_kwargs = {k: _aos_to_ndarray_arg(v) for k, v in kwargs.items()}
        return func(*converted_args, **converted_kwargs)

    # ---- indexing -----------------------------------------------------------
    def __getitem__(self, index):
        return self.__array__()[index]

    def __setitem__(self, index, value):
        view = self._array_view
        if view is not None:
            view[index] = value
        else:
            # Fallback for empty or uninitialised arrays
            raise IndexError("Cannot set items on empty VTKAOSArray")

    # ---- arithmetic operators -----------------------------------------------
    def _binop(self, other, op):
        a = self.__array__()
        b = _aos_to_ndarray(other)
        a, b = _reshape_for_broadcast(a, b)
        return self._wrap_result(op(a, b))

    def _rbinop(self, other, op):
        a = self.__array__()
        b = _aos_to_ndarray(other)
        b, a = _reshape_for_broadcast(b, a)
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
        return f"VTKAOSArray(shape={self.shape}, dtype={self.dtype})"

    def __str__(self):
        return str(self.__array__())

    # ---- static helper to build from numpy array ----------------------------
    @staticmethod
    def _from_array(narray):
        """Create a VTK-backed AOS array from a numpy array.

        Returns a VTKAOSArray-overridden VTK array with the numpy data.
        The VTK array holds a reference to the numpy array to prevent GC.
        """
        vtk_array = numpy_support.numpy_to_vtk(narray)
        return vtk_array


# ---- Register overrides for all AOS template types and concrete classes -----
def _register_aos_overrides():
    """Register VTKAOSArray mixin as override for all AOS template types."""
    from vtkmodules.vtkCommonCore import (
        vtkAOSDataArrayTemplate,
        vtkFloatArray, vtkDoubleArray,
        vtkIntArray, vtkUnsignedIntArray,
        vtkShortArray, vtkUnsignedShortArray,
        vtkSignedCharArray, vtkUnsignedCharArray,
        vtkLongArray, vtkUnsignedLongArray,
        vtkLongLongArray, vtkUnsignedLongLongArray,
        vtkIdTypeArray,
        vtkTypeFloat32Array, vtkTypeFloat64Array,
        vtkTypeInt8Array, vtkTypeInt16Array,
        vtkTypeInt32Array, vtkTypeInt64Array,
        vtkTypeUInt8Array, vtkTypeUInt16Array,
        vtkTypeUInt32Array, vtkTypeUInt64Array,
    )

    _concrete_classes = [
        vtkFloatArray, vtkDoubleArray,
        vtkIntArray, vtkUnsignedIntArray,
        vtkShortArray, vtkUnsignedShortArray,
        vtkSignedCharArray, vtkUnsignedCharArray,
        vtkLongArray, vtkUnsignedLongArray,
        vtkLongLongArray, vtkUnsignedLongLongArray,
        vtkIdTypeArray,
        vtkTypeFloat32Array, vtkTypeFloat64Array,
        vtkTypeInt8Array, vtkTypeInt16Array,
        vtkTypeInt32Array, vtkTypeInt64Array,
        vtkTypeUInt8Array, vtkTypeUInt16Array,
        vtkTypeUInt32Array, vtkTypeUInt64Array,
    ]

    register_template_overrides(
        VTKAOSArray, vtkAOSDataArrayTemplate, 'VTKAOSArray',
        concrete_classes=_concrete_classes)

_register_aos_overrides()

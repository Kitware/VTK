"""VTKConstantArray — numpy-compatible mixin for vtkConstantArray.

This module provides the VTKConstantArray mixin class and registers
overrides for all vtkConstantArray instantiations so that constant arrays
coming from VTK automatically have numpy-compatible operations.

Arithmetic between constant arrays produces new constant arrays; mixed
operations with real arrays return VTKAOSArray results.  The constant
value is never materialized into a full array unless explicitly requested.
"""
import warnings

import numpy

from ..util import numpy_support
from ._vtk_array_mixin import (
    VTKDataArrayMixin, make_override_registry, register_template_overrides,
)


# Registry for __array_function__ overrides
_CONSTANT_OVERRIDE, _override_constant_numpy = make_override_registry()


class VTKConstantArray(VTKDataArrayMixin):
    """A memory-efficient array where every element has the same value.

    ``vtkConstantArray`` stores a single scalar value and a shape, using
    O(1) memory regardless of the number of elements.  This Python
    mixin adds a numpy-compatible interface so that constant arrays
    can be used in arithmetic expressions, numpy functions, and VTK
    pipelines without ever materializing the full array into memory.

    Parameters
    ----------
    shape : int or tuple of int, optional
        Number of tuples (single int or 1-tuple), or ``(ntuples, ncomps)``
        for a multi-component array.  When omitted, creates an empty array
        with zero tuples that can be resized later with the C++ API.
    value : scalar, optional
        The constant value for every element.  Default is ``0``.

    Construction
    ------------
    Create with bracket notation and positional arguments::

        from vtkmodules.vtkCommonCore import vtkConstantArray

        # 1-component, 1 million tuples, all 5.0
        a = vtkConstantArray[numpy.float64](1000000, 5.0)

        # 3-component, 100 tuples, all 5.0
        b = vtkConstantArray[numpy.float64]((100, 3), 5.0)

    Available dtypes: ``'float32'``, ``'float64'``, ``'int8'``, ``'int16'``,
    ``'int32'``, ``'int64'``, ``'uint8'``, ``'uint16'``, ``'uint32'``,
    ``'uint64'``.

    Properties
    ----------
    value : scalar
        The constant value stored by the array.
    shape : tuple of int
        ``(ntuples,)`` for 1-component, ``(ntuples, ncomps)`` otherwise.
    dtype : numpy.dtype
        The element data type.
    dataset : vtkDataSet or None
        The owning VTK dataset, when the array is attached to one.
    association : int or None
        The attribute association (POINT, CELL, etc.).

    Arithmetic
    ----------
    All element-wise operations (``+``, ``-``, ``*``, ``/``, ``**``,
    comparisons, unary ``-``, ``abs``, and numpy ufuncs) operate on the
    scalar constant value without allocating a full array:

    - **Constant + constant** returns a new ``VTKConstantArray``.
    - **Constant + regular array** substitutes the scalar and returns
      a ``VTKAOSArray`` with metadata (dataset, association) propagated.

    Reductions (``sum``, ``mean``, ``min``, ``max``, ``std``, ``var``,
    ``prod``, ``any``, ``all``) are computed in O(1) from the constant
    value and array shape.

    NumPy Integration
    -----------------
    Constant arrays implement the numpy array protocol:

    - ``numpy.asarray(arr)`` materializes the full array via
      ``numpy.full()``.
    - ``numpy.sum(arr)``, ``numpy.mean(arr)``, etc. are intercepted
      and computed in O(1).
    - Ufuncs (``numpy.sin``, ``numpy.sqrt``, etc.) substitute the
      scalar value.
    - Unoptimized numpy functions trigger a warning and fall back to
      materialization.

    Indexing
    --------
    Scalar indexing returns the constant value.  Slices and fancy
    indexing return a new ``VTKConstantArray`` of the appropriate shape,
    still using O(1) memory.  The array is read-only; ``__setitem__``
    raises ``TypeError``.

    Dataset Integration
    -------------------
    Constant arrays retrieved from a VTK dataset (via ``data_model``)
    automatically carry ``dataset`` and ``association`` metadata::

        arr = polydata.point_data["my_constant_array"]
        print(arr.value)       # the constant
        print(arr.dataset)     # the owning vtkPolyData

    See Also
    --------
    vtkConstantArray : The underlying C++ implicit array class.
    VTKAffineArray : Similar lazy evaluation for affine (slope*i + intercept)
        arrays.
    VTKAOSArray : Mixin for regular (array-of-structures) VTK arrays.
    """

    # ---- construction -------------------------------------------------------
    def __init__(self, shape=None, value=0, **kwargs):
        """Create a new constant array.

        Parameters
        ----------
        shape : int or tuple of int, optional
            Number of tuples, or ``(ntuples, ncomps)`` tuple.
        value : scalar, optional
            The constant value (default ``0``).
        **kwargs
            Forwarded to the VTK base-class initializer (e.g. property
            setters like ``name="MyArray"``).
        """
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip mixin init to avoid clobbering state.
        if isinstance(shape, str):
            return
        # Wrapping a pre-existing C++ object (e.g. from GetCellTypes()):
        # __init__ is called with no args.  If the backend is already
        # constructed by C++, skip to avoid clobbering its value.
        if shape is None and not kwargs and self.IsBackendConstructed():
            return
        super().__init__(**kwargs)
        self._dataset = None
        self._association = None
        if shape is not None:
            if isinstance(shape, (tuple, list)):
                if len(shape) == 1:
                    ntuples = shape[0]
                    ncomps = 1
                else:
                    ntuples, ncomps = shape
            else:
                ntuples = shape
                ncomps = 1
            self.SetNumberOfComponents(ncomps)
            self.SetNumberOfTuples(ntuples)
        # Always construct the backend so that GetConstantValue()
        # never dereferences a null pointer.
        self.ConstructBackend(value)

    # ---- factory helpers ----------------------------------------------------
    def _new_like(self, value, dtype=None):
        """Create a new VTKConstantArray of same shape with given value."""
        from ..vtkCommonCore import vtkConstantArray

        dt = numpy.dtype(dtype) if dtype is not None else self.dtype
        vtk_arr = vtkConstantArray[dt.type](self.shape, dt.type(value))
        vtk_arr._dataset = self._dataset
        vtk_arr._association = self._association
        return vtk_arr

    # ---- core properties ----------------------------------------------------
    @property
    def value(self):
        """The constant scalar value."""
        if not self.IsBackendConstructed():
            return None
        return self.GetConstantValue()

    @property
    def dtype(self):
        return numpy.dtype(
            numpy_support.get_numpy_array_type(self.GetDataType()))

    @property
    def nbytes(self):
        return self.size * self.dtype.itemsize

    # ---- numpy protocol -----------------------------------------------------
    def to_numpy(self, dtype=None):
        """Return the full materialized array as a numpy ndarray."""
        return self.__array__(dtype=dtype)

    def __array__(self, dtype=None, copy=None):
        """Materialize the full array when numpy needs it explicitly."""
        dt = dtype or self.dtype
        if not self.IsBackendConstructed():
            return numpy.empty(self.shape, dtype=dt)
        return numpy.full(self.shape, self.GetConstantValue(), dtype=dt)

    def __buffer__(self, flags):
        """Override C-level buffer protocol to materialize via numpy.full."""
        return memoryview(self.__array__())

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        """Handle numpy ufuncs by substituting the scalar constant.

        When all operands are VTKConstantArrays or plain scalars,
        the result is wrapped back into a VTKConstantArray of the
        same shape.  When mixed with a real numpy array, the constant
        is passed as a scalar and the result is wrapped as VTKAOSArray.
        """
        if method != '__call__':
            return NotImplemented

        out = kwargs.get('out', None)
        if out is not None:
            return NotImplemented

        has_real_array = any(
            isinstance(inp, numpy.ndarray) for inp in inputs
        )

        new_inputs = []
        first_const = None
        for inp in inputs:
            if isinstance(inp, VTKConstantArray):
                if not inp.IsBackendConstructed():
                    return NotImplemented
                if first_const is None:
                    first_const = inp
                new_inputs.append(inp.GetConstantValue())
            else:
                new_inputs.append(inp)

        result = ufunc(*new_inputs, **kwargs)

        # All constant or scalar -> wrap as new constant array
        if not has_real_array and first_const is not None:
            scalar = (result.item() if isinstance(result, numpy.ndarray)
                      and result.ndim == 0 else result)
            if numpy.isscalar(scalar):
                result_dtype = numpy.result_type(scalar)
                try:
                    return first_const._new_like(scalar, result_dtype)
                except (KeyError, TypeError):
                    return numpy.full(
                        first_const.shape, scalar, dtype=result_dtype)

        # Mixed with real array -> wrap as VTKAOSArray
        return self._wrap_result(result)

    def __array_function__(self, func, types, args, kwargs):
        """Dispatch numpy functions with O(1) overrides where possible."""
        if func in _CONSTANT_OVERRIDE:
            return _CONSTANT_OVERRIDE[func](*args, **kwargs)

        warnings.warn(
            f"numpy.{func.__name__}() is not optimized for "
            f"VTKConstantArray; the full array will be materialized.",
            stacklevel=2,
        )

        def convert(arg):
            if isinstance(arg, VTKConstantArray):
                return numpy.asarray(arg)
            elif isinstance(arg, (list, tuple)):
                return type(arg)(convert(a) for a in arg)
            return arg

        new_args = [convert(arg) for arg in args]
        return func(*new_args, **kwargs)

    # ---- indexing -----------------------------------------------------------
    def __getitem__(self, key):
        """Index into the virtual constant array.

        Returns a scalar for element access, or a new
        VTKConstantArray for slices / fancy indexing.
        """
        # Use a zero-strided array to compute the result shape without
        # allocating memory proportional to self.shape.
        dummy = numpy.lib.stride_tricks.as_strided(
            numpy.zeros(1, dtype=self.dtype),
            shape=self.shape,
            strides=(0,) * self.ndim,
        )
        sliced = dummy[key]
        if not self.IsBackendConstructed():
            raise RuntimeError(
                "Cannot index a VTKConstantArray whose backend has not "
                "been constructed. Provide shape and value arguments.")
        if (numpy.isscalar(sliced)
                or (isinstance(sliced, numpy.ndarray) and sliced.ndim == 0)):
            return self.dtype.type(self.GetConstantValue())

        from ..vtkCommonCore import vtkConstantArray
        return vtkConstantArray[self.dtype.type](
            sliced.shape, self.dtype.type(self.GetConstantValue()))

    def __setitem__(self, key, value):
        raise TypeError("VTKConstantArray is read-only")

    # ---- arithmetic operators -----------------------------------------------
    def __add__(self, other):       return numpy.add(self, other)
    def __radd__(self, other):      return numpy.add(other, self)
    def __sub__(self, other):       return numpy.subtract(self, other)
    def __rsub__(self, other):      return numpy.subtract(other, self)
    def __mul__(self, other):       return numpy.multiply(self, other)
    def __rmul__(self, other):      return numpy.multiply(other, self)
    def __truediv__(self, other):   return numpy.true_divide(self, other)
    def __rtruediv__(self, other):  return numpy.true_divide(other, self)
    def __floordiv__(self, other):  return numpy.floor_divide(self, other)
    def __rfloordiv__(self, other): return numpy.floor_divide(other, self)
    def __pow__(self, other):       return numpy.power(self, other)
    def __rpow__(self, other):      return numpy.power(other, self)
    def __mod__(self, other):       return numpy.mod(self, other)
    def __rmod__(self, other):      return numpy.mod(other, self)

    # comparison
    def __lt__(self, other):  return numpy.less(self, other)
    def __le__(self, other):  return numpy.less_equal(self, other)
    def __eq__(self, other):  return numpy.equal(self, other)
    def __ne__(self, other):  return numpy.not_equal(self, other)
    def __ge__(self, other):  return numpy.greater_equal(self, other)
    def __gt__(self, other):  return numpy.greater(self, other)

    # unary
    def __neg__(self):   return numpy.negative(self)
    def __pos__(self):   return numpy.positive(self)
    def __abs__(self):   return numpy.absolute(self)

    # ---- O(1) reduction overrides -------------------------------------------

    def _require_backend(self):
        """Raise if backend is not constructed."""
        if not self.IsBackendConstructed():
            raise RuntimeError(
                "Cannot operate on a VTKConstantArray whose backend has not "
                "been constructed. Provide shape and value arguments.")

    def _axis_full(self, axis, value):
        """Build a result array of the correct shape for a reduction."""
        nt = self.GetNumberOfTuples()
        nc = self.GetNumberOfComponents()
        if nc == 1:
            return value
        if axis == 0:
            return numpy.full((nc,), value)
        if axis == 1:
            return numpy.full((nt,), value)
        return None

    def sum(self, axis=None, **kwargs):
        self._require_backend()
        v = self.GetConstantValue()
        nt = self.GetNumberOfTuples()
        nc = self.GetNumberOfComponents()
        if axis is None:
            return self.dtype.type(v * self.size)
        result = self._axis_full(
            axis, self.dtype.type(v * nt) if axis == 0 else self.dtype.type(v * nc))
        if result is not None:
            return result
        return numpy.asarray(self).sum(axis=axis, **kwargs)

    def mean(self, axis=None, **kwargs):
        self._require_backend()
        v = self.dtype.type(self.GetConstantValue())
        if axis is None:
            return v
        result = self._axis_full(axis, v)
        if result is not None:
            return result
        return numpy.asarray(self).mean(axis=axis, **kwargs)

    def min(self, axis=None, **kwargs):
        self._require_backend()
        if self.size == 0:
            raise ValueError("zero-size array has no minimum")
        v = self.dtype.type(self.GetConstantValue())
        if axis is None:
            return v
        result = self._axis_full(axis, v)
        if result is not None:
            return result
        return numpy.asarray(self).min(axis=axis, **kwargs)

    def max(self, axis=None, **kwargs):
        self._require_backend()
        if self.size == 0:
            raise ValueError("zero-size array has no maximum")
        v = self.dtype.type(self.GetConstantValue())
        if axis is None:
            return v
        result = self._axis_full(axis, v)
        if result is not None:
            return result
        return numpy.asarray(self).max(axis=axis, **kwargs)

    def std(self, axis=None, **kwargs):
        if axis is None:
            return self.dtype.type(0)
        result = self._axis_full(axis, self.dtype.type(0))
        if result is not None:
            return result
        return numpy.asarray(self).std(axis=axis, **kwargs)

    def var(self, axis=None, **kwargs):
        if axis is None:
            return self.dtype.type(0)
        result = self._axis_full(axis, self.dtype.type(0))
        if result is not None:
            return result
        return numpy.asarray(self).var(axis=axis, **kwargs)

    def any(self, axis=None, **kwargs):
        self._require_backend()
        v = bool(self.GetConstantValue())
        if axis is None:
            return v
        result = self._axis_full(axis, v)
        if result is not None:
            return result
        return numpy.asarray(self).any(axis=axis, **kwargs)

    def all(self, axis=None, **kwargs):
        self._require_backend()
        v = bool(self.GetConstantValue())
        if axis is None:
            return v
        result = self._axis_full(axis, v)
        if result is not None:
            return result
        return numpy.asarray(self).all(axis=axis, **kwargs)

    def prod(self, axis=None, **kwargs):
        self._require_backend()
        v = self.GetConstantValue()
        nt = self.GetNumberOfTuples()
        nc = self.GetNumberOfComponents()
        if axis is None:
            return self.dtype.type(v ** self.size)
        result = self._axis_full(
            axis, self.dtype.type(v ** nt) if axis == 0 else self.dtype.type(v ** nc))
        if result is not None:
            return result
        return numpy.asarray(self).prod(axis=axis, **kwargs)

    def astype(self, dtype):
        self._require_backend()
        return self._new_like(self.GetConstantValue(), dtype)

    # ---- utilities ----------------------------------------------------------
    def __iter__(self):
        self._require_backend()
        val = self.dtype.type(self.GetConstantValue())
        nc = self.GetNumberOfComponents()
        if nc == 1:
            for _ in range(self.GetNumberOfTuples()):
                yield val
        else:
            row = numpy.full((nc,), self.GetConstantValue(), dtype=self.dtype)
            for _ in range(self.GetNumberOfTuples()):
                yield row.copy()

    def __repr__(self):
        if not self.IsBackendConstructed():
            return (f"VTKConstantArray(uninitialized, "
                    f"shape={self.shape}, dtype={self.dtype})")
        return (f"VTKConstantArray(value={self.GetConstantValue()}, "
                f"shape={self.shape}, dtype={self.dtype})")

    def __str__(self):
        return repr(self)


# ---- closed-form numpy function overrides -----------------------------------

@_override_constant_numpy(numpy.sum)
def _constant_sum(a, axis=None, **kwargs):
    if not isinstance(a, VTKConstantArray):
        return numpy.sum(numpy.asarray(a), axis=axis, **kwargs)
    return a.sum(axis=axis, **kwargs)


@_override_constant_numpy(numpy.mean)
def _constant_mean(a, axis=None, **kwargs):
    if not isinstance(a, VTKConstantArray):
        return numpy.mean(numpy.asarray(a), axis=axis, **kwargs)
    return a.mean(axis=axis, **kwargs)


@_override_constant_numpy(numpy.min)
def _constant_min(a, axis=None, **kwargs):
    if not isinstance(a, VTKConstantArray):
        return numpy.min(numpy.asarray(a), axis=axis, **kwargs)
    return a.min(axis=axis, **kwargs)


@_override_constant_numpy(numpy.max)
def _constant_max(a, axis=None, **kwargs):
    if not isinstance(a, VTKConstantArray):
        return numpy.max(numpy.asarray(a), axis=axis, **kwargs)
    return a.max(axis=axis, **kwargs)


@_override_constant_numpy(numpy.std)
def _constant_std(a, axis=None, **kwargs):
    if not isinstance(a, VTKConstantArray):
        return numpy.std(numpy.asarray(a), axis=axis, **kwargs)
    return a.std(axis=axis, **kwargs)


@_override_constant_numpy(numpy.var)
def _constant_var(a, axis=None, **kwargs):
    if not isinstance(a, VTKConstantArray):
        return numpy.var(numpy.asarray(a), axis=axis, **kwargs)
    return a.var(axis=axis, **kwargs)


@_override_constant_numpy(numpy.any)
def _constant_any(a, axis=None, **kwargs):
    if not isinstance(a, VTKConstantArray):
        return numpy.any(numpy.asarray(a), axis=axis, **kwargs)
    return a.any(axis=axis, **kwargs)


@_override_constant_numpy(numpy.all)
def _constant_all(a, axis=None, **kwargs):
    if not isinstance(a, VTKConstantArray):
        return numpy.all(numpy.asarray(a), axis=axis, **kwargs)
    return a.all(axis=axis, **kwargs)


@_override_constant_numpy(numpy.prod)
def _constant_prod(a, axis=None, **kwargs):
    if not isinstance(a, VTKConstantArray):
        return numpy.prod(numpy.asarray(a), axis=axis, **kwargs)
    return a.prod(axis=axis, **kwargs)


# ---- Register overrides for all constant array template types ---------------
def _register_constant_overrides():
    """Register VTKConstantArray mixin as override for all
    vtkConstantArray template types."""
    from vtkmodules.vtkCommonCore import vtkConstantArray

    register_template_overrides(
        VTKConstantArray, vtkConstantArray, 'VTKConstantArray')

_register_constant_overrides()

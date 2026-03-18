"""VTKAffineArray — numpy-compatible mixin for vtkAffineArray.

This module provides the VTKAffineArray mixin class and registers
overrides for all vtkAffineArray instantiations so that affine arrays
coming from VTK automatically have numpy-compatible operations.

vtkAffineArray computes values using an affine function:
    value = slope * index + intercept

Arithmetic with scalars produces new affine arrays (staying lazy).
Reductions like sum, mean, min, max, std use closed-form formulas.
The full array is never materialized unless explicitly requested.
"""
import warnings

import numpy

from ..util import numpy_support
from ._vtk_array_mixin import (
    VTKDataArrayMixin, make_override_registry, register_template_overrides,
)


# Registry for __array_function__ overrides
_AFFINE_OVERRIDE, _override_affine_numpy = make_override_registry()


class VTKAffineArray(VTKDataArrayMixin):
    """A memory-efficient array whose values follow an affine function.

    Each element is computed as ``slope * index + intercept``, using O(1)
    memory regardless of array size.  This is useful for representing
    regularly-spaced sequences (coordinates, time steps, indices) without
    allocating storage for every element.

    This mixin is automatically applied to all ``vtkAffineArray``
    instantiations.  Any affine array that crosses from C++ to Python
    is already a ``VTKAffineArray`` instance.

    Parameters
    ----------
    shape : int or tuple of int, optional
        Number of tuples (single int or 1-tuple), or ``(ntuples, ncomps)``
        for a multi-component array.  When omitted, creates an empty array.
    slope : scalar, optional
        Slope of the affine function (default ``0``).
    intercept : scalar, optional
        Intercept of the affine function (default ``0``).

    Construction
    ------------
    Create with bracket notation and positional arguments::

        from vtkmodules.vtkCommonCore import vtkAffineArray

        # [0, 1, 2, ..., 99]
        a = vtkAffineArray[numpy.float64](100, 1.0, 0.0)

        # [10, 12, 14, ..., 30]  (11 elements)
        b = vtkAffineArray[numpy.float64](11, 2.0, 10.0)

    Available dtypes: ``'float32'``, ``'float64'``, ``'int8'``, ``'int16'``,
    ``'int32'``, ``'int64'``, ``'uint8'``, ``'uint16'``, ``'uint32'``,
    ``'uint64'``.

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

    Lazy Arithmetic
    ---------------
    Several operations produce a new affine array instead of
    materialising the full data:

    - ``arr * scalar`` scales slope and intercept.
    - ``arr + scalar`` shifts the intercept.
    - ``arr / scalar`` scales slope and intercept.
    - ``-arr`` negates slope and intercept.
    - Slicing with a step produces a new affine array with adjusted
      parameters.

    O(1) Reductions
    ---------------
    ``sum``, ``mean``, ``min``, ``max``, ``std``, ``var``, ``any``,
    ``all``, ``argmin``, ``argmax`` use closed-form arithmetic series
    formulas and never materialise the array.

    NumPy Integration
    -----------------
    - ``numpy.asarray(arr)`` materialises the full array.
    - ``numpy.sum(arr)``, ``numpy.mean(arr)``, etc. are intercepted and
      computed in O(1).
    - Ufuncs with scalar operands stay lazy when possible.
    - Unoptimized numpy functions trigger a warning and fall back to
      materialisation.

    Indexing
    --------
    Scalar indexing returns the computed value.  Slicing a single-component
    array returns a new ``VTKAffineArray`` with adjusted parameters.
    The array is read-only; ``__setitem__`` raises ``TypeError``.

    See Also
    --------
    vtkAffineArray : The underlying C++ implicit array class.
    VTKConstantArray : Similar lazy evaluation for uniform-value arrays.
    VTKAOSArray : Mixin for regular (array-of-structures) VTK arrays.
    """

    # ---- construction -------------------------------------------------------
    def __init__(self, shape=None, slope=0, intercept=0, **kwargs):
        """Create a new affine array.

        Parameters
        ----------
        shape : int or tuple of int, optional
            Number of tuples, or ``(ntuples, ncomps)`` tuple.
        slope : scalar, optional
            Slope of the affine function (default ``0``).
        intercept : scalar, optional
            Intercept of the affine function (default ``0``).
        **kwargs
            Forwarded to the VTK base-class initializer.
        """
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip mixin init to avoid clobbering state.
        if isinstance(shape, str):
            return
        # Wrapping a pre-existing C++ object (e.g. from a filter output):
        # __init__ is called with no args.  If the backend is already
        # constructed by C++, skip to avoid clobbering its values.
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
        # Always construct the backend so that GetSlope()/GetIntercept()
        # never dereference a null pointer.
        self.ConstructBackend(slope, intercept)

    # ---- properties delegating to self (we ARE the VTK object) --------------
    def _require_backend(self):
        """Raise if backend is not constructed."""
        if not self.IsBackendConstructed():
            raise RuntimeError(
                "Cannot operate on a VTKAffineArray whose backend has not "
                "been constructed. Provide shape, slope and intercept arguments.")

    @property
    def _slope(self):
        self._require_backend()
        return self.GetSlope()

    @property
    def _intercept(self):
        self._require_backend()
        return self.GetIntercept()

    @property
    def _num_values(self):
        return self.GetNumberOfValues()

    @property
    def _num_tuples(self):
        return self.GetNumberOfTuples()

    @property
    def _num_components(self):
        return self.GetNumberOfComponents()

    @property
    def _dtype(self):
        return numpy_support.get_numpy_array_type(self.GetDataType())

    # ---- factory helpers ----------------------------------------------------
    @classmethod
    def _create_vtk_array(cls, slope, intercept, num_values, dtype, name=None):
        """Create a vtkAffineArray with the given parameters."""
        from vtkmodules.vtkCommonCore import vtkAffineArray as _vtkAffineArray

        dtype_key = numpy.dtype(dtype).type

        arr = _vtkAffineArray[dtype_key](num_values, dtype(slope), dtype(intercept))
        if name:
            arr.SetName(name)
        return arr

    @classmethod
    def _from_params(cls, slope, intercept, num_values, dtype):
        """Create a new affine array from parameters."""
        return cls._create_vtk_array(
            slope, intercept, num_values,
            dtype if dtype is not None else numpy.float64)

    # ---- core numpy-like properties (override base where needed) ------------
    @property
    def dtype(self):
        return self._dtype

    @property
    def nbytes(self):
        return self.size * numpy.dtype(self._dtype).itemsize

    def __repr__(self):
        if not self.IsBackendConstructed():
            return (f"VTKAffineArray(uninitialized, "
                    f"num_values={self._num_values}, dtype={self._dtype})")
        return (f"VTKAffineArray(slope={self._slope}, intercept={self._intercept}, "
                f"num_values={self._num_values}, dtype={self._dtype})")

    def __str__(self):
        return repr(self)

    # ---- numpy protocol -----------------------------------------------------
    def to_numpy(self, dtype=None):
        """Return the full materialized array as a numpy ndarray."""
        return self.__array__(dtype=dtype)

    def __array__(self, dtype=None, **kwargs):
        """Materialize the full array."""
        indices = numpy.arange(self._num_values, dtype=self._dtype)
        result = self._slope * indices + self._intercept
        if self._num_components > 1:
            result = result.reshape(self._num_tuples, self._num_components)
        if dtype is not None:
            result = result.astype(dtype)
        return result

    def __buffer__(self, flags):
        """Override C-level buffer protocol to materialize via __array__."""
        return memoryview(self.__array__())

    def __getitem__(self, index):
        """Index into the array."""
        if isinstance(index, (int, numpy.integer)):
            n = self._num_tuples
            if index < 0:
                index += n
            if index < 0 or index >= n:
                raise IndexError(
                    f"index {index} is out of bounds for axis 0 with size {n}")
            if self._num_components == 1:
                return self._dtype(self._slope * index + self._intercept)
            else:
                start_idx = index * self._num_components
                return numpy.array([
                    self._slope * (start_idx + c) + self._intercept
                    for c in range(self._num_components)
                ], dtype=self._dtype)
        elif isinstance(index, slice) and self._num_components == 1:
            n = self._num_tuples
            start, stop, step = index.indices(n)
            new_n = len(range(start, stop, step))
            if new_n == 0:
                return numpy.array([], dtype=self._dtype)
            new_slope = self._slope * step
            new_intercept = self._slope * start + self._intercept
            return VTKAffineArray._from_params(
                new_slope, new_intercept, new_n, self._dtype)
        else:
            return numpy.asarray(self)[index]

    def __setitem__(self, key, value):
        raise TypeError("VTKAffineArray is read-only")

    def __iter__(self):
        for i in range(self._num_tuples):
            yield self[i]

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        """Handle numpy ufuncs. Many operations stay lazy."""
        if method != '__call__':
            return NotImplemented

        out = kwargs.get('out', None)
        if out is not None:
            return NotImplemented

        # Unary ufunc
        if len(inputs) == 1 and isinstance(inputs[0], VTKAffineArray):
            arr = inputs[0]
            if ufunc is numpy.negative:
                return VTKAffineArray._from_params(
                    -arr._slope, -arr._intercept, arr._num_values, arr._dtype)
            if ufunc is numpy.positive:
                return VTKAffineArray._from_params(
                    arr._slope, arr._intercept, arr._num_values, arr._dtype)
            if ufunc is numpy.abs or ufunc is numpy.absolute:
                first_val = arr._intercept
                last_val = arr._slope * (arr._num_values - 1) + arr._intercept
                if first_val >= 0 and last_val >= 0:
                    return VTKAffineArray._from_params(
                        arr._slope, arr._intercept, arr._num_values, arr._dtype)
                elif first_val <= 0 and last_val <= 0:
                    return VTKAffineArray._from_params(
                        -arr._slope, -arr._intercept, arr._num_values, arr._dtype)

        # Binary ufunc with scalar
        if len(inputs) == 2:
            self_input = None
            other_input = None
            self_first = True
            for i, inp in enumerate(inputs):
                if isinstance(inp, VTKAffineArray):
                    self_input = inp
                    self_first = (i == 0)
                else:
                    other_input = inp

            if self_input is not None and other_input is not None:
                if numpy.isscalar(other_input) or (
                        isinstance(other_input, numpy.ndarray) and other_input.ndim == 0):
                    scalar = float(other_input)

                    if ufunc is numpy.multiply:
                        return VTKAffineArray._from_params(
                            self_input._slope * scalar,
                            self_input._intercept * scalar,
                            self_input._num_values,
                            self_input._dtype)

                    if ufunc is numpy.true_divide and self_first:
                        return VTKAffineArray._from_params(
                            self_input._slope / scalar,
                            self_input._intercept / scalar,
                            self_input._num_values,
                            self_input._dtype)

                    if ufunc is numpy.add:
                        return VTKAffineArray._from_params(
                            self_input._slope,
                            self_input._intercept + scalar,
                            self_input._num_values,
                            self_input._dtype)

                    if ufunc is numpy.subtract:
                        if self_first:
                            return VTKAffineArray._from_params(
                                self_input._slope,
                                self_input._intercept - scalar,
                                self_input._num_values,
                                self_input._dtype)
                        else:
                            return VTKAffineArray._from_params(
                                -self_input._slope,
                                scalar - self_input._intercept,
                                self_input._num_values,
                                self_input._dtype)

        # Fall back to materialization, wrap result for metadata propagation
        materialized = [numpy.asarray(x) if isinstance(x, VTKAffineArray) else x
                        for x in inputs]
        result = ufunc(*materialized, **kwargs)
        return self._wrap_result(result)

    def __array_function__(self, func, types, args, kwargs):
        """Handle numpy functions like sum, min, max, mean with optimized paths."""
        if func in _AFFINE_OVERRIDE:
            return _AFFINE_OVERRIDE[func](*args, **kwargs)

        warnings.warn(
            f"numpy.{func.__name__}() is not optimized for "
            f"VTKAffineArray; the full array will be materialized.",
            stacklevel=2,
        )

        new_args = []
        for a in args:
            if isinstance(a, VTKAffineArray):
                new_args.append(numpy.asarray(a))
            else:
                new_args.append(a)
        return func(*new_args, **kwargs)

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
    def __neg__(self):              return numpy.negative(self)
    def __pos__(self):              return numpy.positive(self)
    def __abs__(self):              return numpy.absolute(self)

    # ---- O(1) reduction overrides -------------------------------------------

    def _axis_is_flat(self, axis):
        """True when axis reduces the entire 1-component array (axis=0 ≡ None)."""
        return axis is None or (axis == 0 and self._num_components == 1)

    def sum(self, axis=None, **kwargs):
        if self._axis_is_flat(axis):
            n = self._num_values
            return self._dtype(n * self._intercept + self._slope * n * (n - 1) / 2)
        return numpy.asarray(self).sum(axis=axis, **kwargs)

    def mean(self, axis=None, **kwargs):
        if self._axis_is_flat(axis):
            n = self._num_values
            return self._dtype(self._intercept + self._slope * (n - 1) / 2)
        return numpy.asarray(self).mean(axis=axis, **kwargs)

    def min(self, axis=None, **kwargs):
        if self._axis_is_flat(axis):
            n = self._num_values
            if n == 0:
                raise ValueError("zero-size array has no minimum")
            if self._slope >= 0:
                return self._dtype(self._intercept)
            else:
                return self._dtype(self._slope * (n - 1) + self._intercept)
        return numpy.asarray(self).min(axis=axis, **kwargs)

    def max(self, axis=None, **kwargs):
        if self._axis_is_flat(axis):
            n = self._num_values
            if n == 0:
                raise ValueError("zero-size array has no maximum")
            if self._slope >= 0:
                return self._dtype(self._slope * (n - 1) + self._intercept)
            else:
                return self._dtype(self._intercept)
        return numpy.asarray(self).max(axis=axis, **kwargs)

    def std(self, axis=None, ddof=0, **kwargs):
        if self._axis_is_flat(axis):
            n = self._num_values
            if n <= ddof:
                return self._dtype(numpy.nan)
            variance = (self._slope ** 2) * (n ** 2 - 1) / 12
            if ddof > 0:
                variance = variance * n / (n - ddof)
            return self._dtype(numpy.sqrt(variance))
        return numpy.asarray(self).std(axis=axis, ddof=ddof, **kwargs)

    def var(self, axis=None, ddof=0, **kwargs):
        if self._axis_is_flat(axis):
            n = self._num_values
            if n <= ddof:
                return self._dtype(numpy.nan)
            variance = (self._slope ** 2) * (n ** 2 - 1) / 12
            if ddof > 0:
                variance = variance * n / (n - ddof)
            return self._dtype(variance)
        return numpy.asarray(self).var(axis=axis, ddof=ddof, **kwargs)

    def any(self, axis=None, **kwargs):
        if self._axis_is_flat(axis):
            n = self._num_values
            if n == 0:
                return False
            return not (self._slope == 0 and self._intercept == 0)
        return numpy.asarray(self).any(axis=axis, **kwargs)

    def all(self, axis=None, **kwargs):
        if self._axis_is_flat(axis):
            n = self._num_values
            if n == 0:
                return True
            if self._slope == 0:
                return self._intercept != 0
            quotient, remainder = divmod(-self._intercept, self._slope)
            if remainder == 0 and 0 <= quotient < n:
                return False
            return True
        return numpy.asarray(self).all(axis=axis, **kwargs)

    def prod(self, axis=None, **kwargs):
        if self._axis_is_flat(axis):
            return numpy.asarray(self).prod()
        return numpy.asarray(self).prod(axis=axis, **kwargs)

    def argmin(self, axis=None, **kwargs):
        if self._axis_is_flat(axis) and self._num_values > 0:
            if self._slope >= 0:
                return 0
            else:
                return self._num_values - 1
        return numpy.argmin(numpy.asarray(self), axis=axis, **kwargs)

    def argmax(self, axis=None, **kwargs):
        if self._axis_is_flat(axis) and self._num_values > 0:
            if self._slope >= 0:
                return self._num_values - 1
            else:
                return 0
        return numpy.argmax(numpy.asarray(self), axis=axis, **kwargs)


# ---- closed-form numpy function overrides -----------------------------------

@_override_affine_numpy(numpy.sum)
def _affine_sum(a, axis=None, **kwargs):
    """sum = n*intercept + slope*n*(n-1)/2."""
    if not isinstance(a, VTKAffineArray):
        return numpy.sum(numpy.asarray(a), axis=axis, **kwargs)
    return a.sum(axis=axis, **kwargs)


@_override_affine_numpy(numpy.mean)
def _affine_mean(a, axis=None, **kwargs):
    """mean = intercept + slope*(n-1)/2."""
    if not isinstance(a, VTKAffineArray):
        return numpy.mean(numpy.asarray(a), axis=axis, **kwargs)
    return a.mean(axis=axis, **kwargs)


@_override_affine_numpy(numpy.min)
def _affine_min(a, axis=None, **kwargs):
    """min depends on slope sign."""
    if not isinstance(a, VTKAffineArray):
        return numpy.min(numpy.asarray(a), axis=axis, **kwargs)
    return a.min(axis=axis, **kwargs)


@_override_affine_numpy(numpy.max)
def _affine_max(a, axis=None, **kwargs):
    """max depends on slope sign."""
    if not isinstance(a, VTKAffineArray):
        return numpy.max(numpy.asarray(a), axis=axis, **kwargs)
    return a.max(axis=axis, **kwargs)


@_override_affine_numpy(numpy.std)
def _affine_std(a, axis=None, ddof=0, **kwargs):
    """std using closed-form: |slope| * sqrt((n^2-1)/12)."""
    if not isinstance(a, VTKAffineArray):
        return numpy.std(numpy.asarray(a), axis=axis, ddof=ddof, **kwargs)
    return a.std(axis=axis, ddof=ddof, **kwargs)


@_override_affine_numpy(numpy.var)
def _affine_var(a, axis=None, ddof=0, **kwargs):
    """var using closed-form: slope^2 * (n^2-1) / 12."""
    if not isinstance(a, VTKAffineArray):
        return numpy.var(numpy.asarray(a), axis=axis, ddof=ddof, **kwargs)
    return a.var(axis=axis, ddof=ddof, **kwargs)


@_override_affine_numpy(numpy.any)
def _affine_any(a, axis=None, **kwargs):
    """O(1): any is True unless all values are zero (slope==0 and intercept==0)."""
    if not isinstance(a, VTKAffineArray):
        return numpy.any(numpy.asarray(a), axis=axis, **kwargs)
    return a.any(axis=axis, **kwargs)


@_override_affine_numpy(numpy.all)
def _affine_all(a, axis=None, **kwargs):
    """O(1) when possible: check if any value in the sequence is zero."""
    if not isinstance(a, VTKAffineArray):
        return numpy.all(numpy.asarray(a), axis=axis, **kwargs)
    return a.all(axis=axis, **kwargs)


@_override_affine_numpy(numpy.prod)
def _affine_prod(a, axis=None, **kwargs):
    """No useful closed form — materialize."""
    if not isinstance(a, VTKAffineArray):
        return numpy.prod(numpy.asarray(a), axis=axis, **kwargs)
    return a.prod(axis=axis, **kwargs)


# ---- Register overrides for all affine array template types -----------------
def _register_affine_overrides():
    """Register VTKAffineArray mixin as override for all
    vtkAffineArray template types."""
    from vtkmodules.vtkCommonCore import vtkAffineArray as _vtkAffineArray

    register_template_overrides(
        VTKAffineArray, _vtkAffineArray, 'VTKAffineArray')

_register_affine_overrides()

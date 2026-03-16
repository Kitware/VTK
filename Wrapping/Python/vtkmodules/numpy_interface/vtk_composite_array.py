"""VTKCompositeArray — numpy-compatible mixin for vtkCompositeArray.

This module provides the VTKCompositeArray mixin class and registers
overrides for all vtkCompositeArray instantiations so that composite
arrays coming from VTK automatically have numpy-compatible operations.

A vtkCompositeArray concatenates multiple sub-arrays into a single
virtual array.  Each sub-array already has its own numpy mixin, so
operations are delegated per-sub-array and combined, minimizing
materialization.  Reductions (sum, min, max, mean, std, var) operate
in O(n_arrays) rather than O(n_elements).
"""
import bisect
import warnings

import numpy

from ..util import numpy_support
from ._vtk_array_mixin import (
    VTKDataArrayMixin, make_override_registry, register_template_overrides,
)


# Registry for __array_function__ overrides
_COMPOSITE_OVERRIDE, _override_composite_numpy = make_override_registry()


class VTKCompositeArray(VTKDataArrayMixin):
    """A numpy-compatible mixin for composite (concatenated) implicit arrays.

    ``vtkCompositeArray`` stores multiple sub-arrays and presents them as
    a single virtual array using O(log n) binary search for element access.
    This Python mixin adds numpy integration: arithmetic, reductions,
    indexing, and ``to_numpy()`` — delegating to per-sub-array operations
    to minimize materialization.
    """

    # ---- construction -------------------------------------------------------
    def __init__(self, arrays=None, **kwargs):
        """Create a composite array.

        Parameters
        ----------
        arrays : iterable of vtkDataArray, optional
            Sub-arrays to concatenate.  All must have the same number of
            components.  When omitted, creates an empty composite that
            can be populated later via the C++ API.
        **kwargs
            Forwarded to the VTK base-class initializer.
        """
        # SWIG pointer reconstruction guard
        if isinstance(arrays, str):
            return
        super().__init__(**kwargs)
        self._dataset = None
        self._association = None
        self._arrays_cache = None
        if arrays is not None:
            from ..vtkCommonCore import vtkDataArrayCollection
            array_list = list(arrays)
            if not array_list:
                return
            ncomps = array_list[0].GetNumberOfComponents()
            total_tuples = 0
            collection = vtkDataArrayCollection()
            for arr in array_list:
                collection.AddItem(arr)
                total_tuples += arr.GetNumberOfTuples()
            self.SetNumberOfComponents(ncomps)
            self.SetNumberOfTuples(total_tuples)
            self.ConstructBackend(collection)

    # ---- sub-array access ---------------------------------------------------
    @property
    def arrays(self):
        """Return the list of original sub-arrays (with their own mixins).

        Each sub-array is the original vtkDataArray passed to the backend,
        enhanced with whatever mixin it has (VTKAOSArray, VTKConstantArray, etc.).
        """
        if self._arrays_cache is not None:
            return self._arrays_cache
        n = self.GetNumberOfArrays()
        if n == 0:
            return []
        result = [self.GetArray(i) for i in range(n)]
        self._arrays_cache = result
        return result

    @property
    def _offsets(self):
        """Return the list of tuple offsets for each sub-array."""
        n = self.GetNumberOfArrays()
        return [self.GetOffset(i) for i in range(n)]

    # ---- core properties ----------------------------------------------------
    @property
    def dtype(self):
        return numpy.dtype(
            numpy_support.get_numpy_array_type(self.GetDataType()))

    @property
    def nbytes(self):
        return self.size * self.dtype.itemsize

    # ---- numpy protocol -----------------------------------------------------
    def to_numpy(self, dtype=None):
        """Materialize the full array as a numpy ndarray."""
        return self.__array__(dtype=dtype)

    def __array__(self, dtype=None, copy=None):
        """Materialize the full array by concatenating sub-arrays."""
        sub_arrays = self.arrays
        if not sub_arrays:
            dt = dtype or self.dtype
            nc = self.GetNumberOfComponents()
            shape = (0,) if nc == 1 else (0, nc)
            return numpy.empty(shape, dtype=dt)

        parts = []
        for arr in sub_arrays:
            a = numpy.asarray(arr)
            parts.append(a)

        result = numpy.concatenate(parts, axis=0)

        dt = dtype or self.dtype
        if result.dtype != dt:
            result = result.astype(dt)
        return result

    def __buffer__(self, flags):
        """Override C-level buffer protocol to materialize via concatenation."""
        return memoryview(self.__array__())

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        """Handle numpy ufuncs by applying per-sub-array."""
        if method != '__call__':
            return NotImplemented

        out = kwargs.get('out', None)
        if out is not None:
            return NotImplemented

        sub_arrays = self.arrays
        if not sub_arrays:
            return NotImplemented

        offsets = self._offsets
        ntuples = self.GetNumberOfTuples()

        # Classify inputs
        new_input_parts = []  # list of lists, one per sub-array
        for _ in sub_arrays:
            new_input_parts.append([])

        for inp in inputs:
            if isinstance(inp, VTKCompositeArray):
                # Split into sub-arrays
                inp_subs = inp.arrays
                if len(inp_subs) == len(sub_arrays):
                    for i, sub in enumerate(inp_subs):
                        new_input_parts[i].append(sub)
                else:
                    # Different structure — materialize
                    arr = numpy.asarray(inp)
                    for i, sub in enumerate(sub_arrays):
                        start = offsets[i]
                        end = start + sub.GetNumberOfTuples()
                        new_input_parts[i].append(arr[start:end])
            elif isinstance(inp, numpy.ndarray) and inp.shape[0] == ntuples:
                # Slice the numpy array per sub-array
                for i, sub in enumerate(sub_arrays):
                    start = offsets[i]
                    end = start + sub.GetNumberOfTuples()
                    new_input_parts[i].append(inp[start:end])
            elif numpy.isscalar(inp) or (
                    isinstance(inp, numpy.ndarray) and inp.ndim == 0):
                for i in range(len(sub_arrays)):
                    new_input_parts[i].append(inp)
            else:
                # Unknown operand type — fall back to materialization
                materialized = [numpy.asarray(x)
                                if isinstance(x, VTKCompositeArray) else x
                                for x in inputs]
                return ufunc(*materialized, **kwargs)

        # Apply ufunc per sub-array
        result_parts = []
        for i in range(len(sub_arrays)):
            part_result = ufunc(*new_input_parts[i], **kwargs)
            result_parts.append(part_result)

        # Concatenate results
        result = numpy.concatenate(result_parts, axis=0)
        return self._wrap_result(result)

    def __array_function__(self, func, types, args, kwargs):
        """Dispatch numpy functions with per-sub-array overrides where possible."""
        if func in _COMPOSITE_OVERRIDE:
            return _COMPOSITE_OVERRIDE[func](*args, **kwargs)

        warnings.warn(
            f"numpy.{func.__name__}() is not optimized for "
            f"VTKCompositeArray; the full array will be materialized.",
            stacklevel=2,
        )

        def convert(arg):
            if isinstance(arg, VTKCompositeArray):
                return numpy.asarray(arg)
            elif isinstance(arg, (list, tuple)):
                return type(arg)(convert(a) for a in arg)
            return arg

        new_args = [convert(arg) for arg in args]
        return func(*new_args, **kwargs)

    # ---- indexing -----------------------------------------------------------
    def __getitem__(self, key):
        """Index into the composite array.

        Scalar index delegates to the appropriate sub-array.
        Slices map to sub-array ranges and concatenate.
        Fancy/boolean indexing materializes.
        """
        sub_arrays = self.arrays
        if not sub_arrays:
            raise IndexError("index out of range for empty composite array")

        ntuples = self.GetNumberOfTuples()
        nc = self.GetNumberOfComponents()
        offsets = self._offsets

        # Scalar tuple index
        if isinstance(key, (int, numpy.integer)):
            if key < 0:
                key += ntuples
            if key < 0 or key >= ntuples:
                raise IndexError(
                    f"index {key} is out of bounds for axis 0 with size {ntuples}")
            # Binary search to find the right sub-array
            idx = bisect.bisect_right(offsets, key) - 1
            local_key = key - offsets[idx]
            sub = sub_arrays[idx]
            if nc == 1:
                return self.dtype.type(sub.GetValue(local_key))
            else:
                return numpy.array(sub.GetTuple(local_key), dtype=self.dtype)

        # Slice
        if isinstance(key, slice):
            start, stop, step = key.indices(ntuples)
            if step == 1:
                # Contiguous slice — collect from sub-arrays
                parts = []
                for i, sub in enumerate(sub_arrays):
                    sub_start = offsets[i]
                    sub_end = sub_start + sub.GetNumberOfTuples()
                    # Overlap with [start, stop)
                    lo = max(start, sub_start)
                    hi = min(stop, sub_end)
                    if lo < hi:
                        local_lo = lo - sub_start
                        local_hi = hi - sub_start
                        parts.append(numpy.asarray(sub)[local_lo:local_hi])
                if not parts:
                    shape = (0,) if nc == 1 else (0, nc)
                    return numpy.empty(shape, dtype=self.dtype)
                return numpy.concatenate(parts, axis=0)
            else:
                # Non-unit step — materialize
                return self.__array__()[key]

        # Tuple indexing (row, col)
        if isinstance(key, tuple) and len(key) == 2:
            row_key, col_key = key
            if isinstance(row_key, (int, numpy.integer)):
                row = self[row_key]
                return row[col_key] if nc > 1 else row

        # Fancy/boolean indexing — materialize
        return self.__array__()[key]

    def __setitem__(self, key, value):
        raise TypeError("VTKCompositeArray is read-only")

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

    # ---- O(n_arrays) reduction overrides ------------------------------------
    def sum(self, axis=None, **kwargs):
        if axis is not None:
            return numpy.asarray(self).sum(axis=axis, **kwargs)
        return sum(numpy.asarray(sub).sum() for sub in self.arrays)

    def min(self, axis=None, **kwargs):
        if axis is not None:
            return numpy.asarray(self).min(axis=axis, **kwargs)
        sub_arrays = self.arrays
        if not sub_arrays:
            raise ValueError("zero-size array has no minimum")
        return min(numpy.asarray(sub).min() for sub in sub_arrays)

    def max(self, axis=None, **kwargs):
        if axis is not None:
            return numpy.asarray(self).max(axis=axis, **kwargs)
        sub_arrays = self.arrays
        if not sub_arrays:
            raise ValueError("zero-size array has no maximum")
        return max(numpy.asarray(sub).max() for sub in sub_arrays)

    def mean(self, axis=None, **kwargs):
        if axis is not None:
            return numpy.asarray(self).mean(axis=axis, **kwargs)
        total = sum(numpy.asarray(sub).sum() for sub in self.arrays)
        return total / self.size

    def std(self, axis=None, **kwargs):
        if axis is not None:
            return numpy.asarray(self).std(axis=axis, **kwargs)
        mean_val = self.mean()
        total_sq = sum(
            numpy.asarray(sub).astype(numpy.float64).var() * sub.GetNumberOfTuples() * sub.GetNumberOfComponents()
            + sub.GetNumberOfTuples() * sub.GetNumberOfComponents() * (numpy.asarray(sub).mean() - mean_val) ** 2
            for sub in self.arrays
        )
        return numpy.sqrt(total_sq / self.size)

    def var(self, axis=None, **kwargs):
        if axis is not None:
            return numpy.asarray(self).var(axis=axis, **kwargs)
        mean_val = self.mean()
        total_sq = sum(
            numpy.asarray(sub).astype(numpy.float64).var() * sub.GetNumberOfTuples() * sub.GetNumberOfComponents()
            + sub.GetNumberOfTuples() * sub.GetNumberOfComponents() * (numpy.asarray(sub).mean() - mean_val) ** 2
            for sub in self.arrays
        )
        return total_sq / self.size

    def any(self, axis=None, **kwargs):
        if axis is not None:
            return numpy.asarray(self).any(axis=axis, **kwargs)
        return any(numpy.asarray(sub).any() for sub in self.arrays)

    def all(self, axis=None, **kwargs):
        if axis is not None:
            return numpy.asarray(self).all(axis=axis, **kwargs)
        return all(numpy.asarray(sub).all() for sub in self.arrays)

    def prod(self, axis=None, **kwargs):
        if axis is not None:
            return numpy.asarray(self).prod(axis=axis, **kwargs)
        result = numpy.float64(1)
        for sub in self.arrays:
            result *= numpy.asarray(sub).prod()
        return result

    # ---- utilities ----------------------------------------------------------
    def __iter__(self):
        for i in range(self.GetNumberOfTuples()):
            yield self[i]

    def __repr__(self):
        return (f"VTKCompositeArray(n_arrays={self.GetNumberOfArrays()}, "
                f"shape={self.shape}, dtype={self.dtype})")

    def __str__(self):
        return repr(self)


# ---- numpy function overrides -----------------------------------------------

@_override_composite_numpy(numpy.sum)
def _composite_sum(a, axis=None, **kwargs):
    if not isinstance(a, VTKCompositeArray):
        return numpy.sum(numpy.asarray(a), axis=axis, **kwargs)
    return a.sum(axis=axis, **kwargs)


@_override_composite_numpy(numpy.mean)
def _composite_mean(a, axis=None, **kwargs):
    if not isinstance(a, VTKCompositeArray):
        return numpy.mean(numpy.asarray(a), axis=axis, **kwargs)
    return a.mean(axis=axis, **kwargs)


@_override_composite_numpy(numpy.min)
def _composite_min(a, axis=None, **kwargs):
    if not isinstance(a, VTKCompositeArray):
        return numpy.min(numpy.asarray(a), axis=axis, **kwargs)
    return a.min(axis=axis, **kwargs)


@_override_composite_numpy(numpy.max)
def _composite_max(a, axis=None, **kwargs):
    if not isinstance(a, VTKCompositeArray):
        return numpy.max(numpy.asarray(a), axis=axis, **kwargs)
    return a.max(axis=axis, **kwargs)


@_override_composite_numpy(numpy.std)
def _composite_std(a, axis=None, **kwargs):
    if not isinstance(a, VTKCompositeArray):
        return numpy.std(numpy.asarray(a), axis=axis, **kwargs)
    return a.std(axis=axis, **kwargs)


@_override_composite_numpy(numpy.var)
def _composite_var(a, axis=None, **kwargs):
    if not isinstance(a, VTKCompositeArray):
        return numpy.var(numpy.asarray(a), axis=axis, **kwargs)
    return a.var(axis=axis, **kwargs)


@_override_composite_numpy(numpy.any)
def _composite_any(a, axis=None, **kwargs):
    if not isinstance(a, VTKCompositeArray):
        return numpy.any(numpy.asarray(a), axis=axis, **kwargs)
    return a.any(axis=axis, **kwargs)


@_override_composite_numpy(numpy.all)
def _composite_all(a, axis=None, **kwargs):
    if not isinstance(a, VTKCompositeArray):
        return numpy.all(numpy.asarray(a), axis=axis, **kwargs)
    return a.all(axis=axis, **kwargs)


@_override_composite_numpy(numpy.prod)
def _composite_prod(a, axis=None, **kwargs):
    if not isinstance(a, VTKCompositeArray):
        return numpy.prod(numpy.asarray(a), axis=axis, **kwargs)
    return a.prod(axis=axis, **kwargs)


@_override_composite_numpy(numpy.concatenate)
def _composite_concatenate(arrays, axis=0, **kwargs):
    converted = []
    for a in arrays:
        if isinstance(a, VTKCompositeArray):
            converted.append(numpy.asarray(a))
        else:
            converted.append(a)
    return numpy.concatenate(converted, axis=axis, **kwargs)


# ---- Register overrides for all composite array template types ---------------
def _register_composite_overrides():
    """Register VTKCompositeArray mixin as override for all
    vtkCompositeArray template types."""
    from vtkmodules.vtkCommonCore import vtkCompositeArray

    register_template_overrides(
        VTKCompositeArray, vtkCompositeArray, 'VTKCompositeArray')

_register_composite_overrides()

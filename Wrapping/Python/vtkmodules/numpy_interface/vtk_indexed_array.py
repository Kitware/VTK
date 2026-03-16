"""VTKIndexedArray — numpy-compatible mixin for vtkIndexedArray.

This module provides the VTKIndexedArray mixin class and registers
overrides for all vtkIndexedArray instantiations so that indexed
arrays coming from VTK automatically have numpy-compatible operations.

A vtkIndexedArray provides reindexed access to a base array via an
index list: ``result[i] = base_array[index_array[i]]``.  It is used
in HyperTree filters and probe filters for zero-copy subset views.

Materialization uses efficient numpy fancy indexing:
``numpy.asarray(base)[numpy.asarray(indexes)]``.  Reductions delegate
to the materialized array since there is no closed-form shortcut for
arbitrary index subsets.
"""
import warnings

import numpy

from ..util import numpy_support
from ._vtk_array_mixin import (
    VTKDataArrayMixin, make_override_registry, register_template_overrides,
)


# Registry for __array_function__ overrides
_INDEXED_OVERRIDE, _override_indexed_numpy = make_override_registry()


class VTKIndexedArray(VTKDataArrayMixin):
    """A numpy-compatible mixin for indexed (reindexed subset) implicit arrays.

    ``vtkIndexedArray`` stores a base array and an index array, presenting
    ``base[indexes[i]]`` as a virtual array.  This Python mixin adds numpy
    integration: arithmetic, reductions, indexing, and ``to_numpy()``.
    """

    # ---- construction -------------------------------------------------------
    def __init__(self, indexes=None, array=None, **kwargs):
        """Create an indexed array.

        Parameters
        ----------
        indexes : vtkDataArray, optional
            Index array for indirection.
        array : vtkDataArray, optional
            Base array to index into.
        **kwargs
            Forwarded to the VTK base-class initializer.
        """
        # SWIG pointer reconstruction guard
        if isinstance(indexes, str):
            return
        super().__init__(**kwargs)
        self._dataset = None
        self._association = None
        if indexes is not None and array is not None:
            ncomps = array.GetNumberOfComponents()
            ntuples = indexes.GetNumberOfTuples()
            self.SetNumberOfComponents(ncomps)
            self.SetNumberOfTuples(ntuples)
            self.ConstructBackend(indexes, array)

    # ---- accessor properties ------------------------------------------------
    @property
    def base_array(self):
        """Return the original base array used for value lookup."""
        return self.GetBaseArray()

    @property
    def index_array(self):
        """Return the original index array used for indirection."""
        return self.GetIndexArray()

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
        """Materialize the full array via numpy fancy indexing."""
        base = self.GetBaseArray()
        indexes = self.GetIndexArray()
        if base is None or indexes is None:
            dt = dtype or self.dtype
            nc = self.GetNumberOfComponents()
            shape = (0,) if nc == 1 else (0, nc)
            return numpy.empty(shape, dtype=dt)

        base_np = numpy.asarray(base).ravel()
        index_np = numpy.asarray(indexes).ravel().astype(numpy.intp)

        # Indexes are value-level (flat), so index into flattened base
        result = base_np[index_np]

        nc = self.GetNumberOfComponents()
        if nc > 1:
            result = result.reshape(-1, nc)

        dt = dtype or self.dtype
        if result.dtype != dt:
            result = result.astype(dt)
        return result

    def __buffer__(self, flags):
        """Override C-level buffer protocol to materialize via __array__."""
        return memoryview(self.__array__())

    def _new_indexed(self, new_base_np):
        """Create a new VTKIndexedArray with the same indexes but a new base.

        Parameters
        ----------
        new_base_np : numpy.ndarray
            The transformed base array (flat values).

        Returns
        -------
        VTKIndexedArray or None
            A new indexed array, or None if the dtype is unsupported.
        """
        from ..vtkCommonCore import vtkIndexedArray

        dt = new_base_np.dtype
        # VTK has no bool type; fall back to materialization
        if dt == numpy.bool_:
            return None
        try:
            vtk_base = numpy_support.numpy_to_vtk(new_base_np)
        except (TypeError, KeyError):
            return None
        idx_arr = self.GetIndexArray()
        nc = self.GetNumberOfComponents()
        nt = self.GetNumberOfTuples()
        try:
            result = vtkIndexedArray[dt.name]()
        except (KeyError, TypeError):
            return None
        result.SetNumberOfComponents(nc)
        result.SetNumberOfTuples(nt)
        result.ConstructBackend(idx_arr, vtk_base)
        result._dataset = self._dataset
        result._association = self._association
        return result

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        """Handle numpy ufuncs, staying lazy when possible.

        For unary ufuncs and binary ufuncs with a scalar operand,
        the ufunc is applied to the base array and a new indexed
        array is returned with the same index mapping — no
        materialization needed.

        For binary ufuncs with another array operand, the indexed
        array is materialized since the other array's elements don't
        align with the base.
        """
        if method != '__call__':
            return NotImplemented

        out = kwargs.get('out', None)
        if out is not None:
            return NotImplemented

        # Check if we can stay lazy: exactly one VTKIndexedArray
        # and the rest are scalars / 0-d arrays.  Apply the ufunc to
        # the base array instead and reuse the same index mapping.
        n_indexed = sum(1 for inp in inputs if isinstance(inp, VTKIndexedArray))
        all_others_scalar = all(
            isinstance(inp, VTKIndexedArray)
            or numpy.isscalar(inp)
            or (isinstance(inp, numpy.ndarray) and inp.ndim == 0)
            for inp in inputs
        )

        if n_indexed == 1 and all_others_scalar:
            base = self.GetBaseArray()
            if base is not None:
                base_np = numpy.asarray(base).ravel()
                new_inputs = []
                for inp in inputs:
                    if isinstance(inp, VTKIndexedArray):
                        new_inputs.append(base_np)
                    else:
                        new_inputs.append(inp)
                new_base = ufunc(*new_inputs, **kwargs)
                result = self._new_indexed(new_base)
                if result is not None:
                    return result

        # Fall back to materialization
        new_inputs = []
        for inp in inputs:
            if isinstance(inp, VTKIndexedArray):
                new_inputs.append(numpy.asarray(inp))
            else:
                new_inputs.append(inp)

        result = ufunc(*new_inputs, **kwargs)
        return self._wrap_result(result)

    def __array_function__(self, func, types, args, kwargs):
        """Dispatch numpy functions via override registry."""
        if func in _INDEXED_OVERRIDE:
            return _INDEXED_OVERRIDE[func](*args, **kwargs)

        warnings.warn(
            f"numpy.{func.__name__}() is not optimized for "
            f"VTKIndexedArray; the full array will be materialized.",
            stacklevel=2,
        )

        def convert(arg):
            if isinstance(arg, VTKIndexedArray):
                return numpy.asarray(arg)
            elif isinstance(arg, (list, tuple)):
                return type(arg)(convert(a) for a in arg)
            return arg

        new_args = [convert(arg) for arg in args]
        return func(*new_args, **kwargs)

    def _sub_index_array(self, tuple_indices):
        """Build a new value-level index array for the given tuple indices.

        Parameters
        ----------
        tuple_indices : array-like of int
            Which tuples to select.

        Returns
        -------
        numpy.ndarray
            Flat value-level indices into the base array.
        """
        nc = self.GetNumberOfComponents()
        idx_np = numpy.asarray(self.GetIndexArray()).ravel()
        if nc == 1:
            return idx_np[tuple_indices]
        # Multi-component: each tuple occupies nc consecutive entries
        tuple_indices = numpy.asarray(tuple_indices)
        value_indices = numpy.empty(len(tuple_indices) * nc, dtype=idx_np.dtype)
        for c in range(nc):
            value_indices[c::nc] = idx_np[tuple_indices * nc + c]
        return value_indices

    def _new_from_sub_indexes(self, new_idx_np):
        """Create a new VTKIndexedArray with sub-selected indexes, same base.

        Parameters
        ----------
        new_idx_np : numpy.ndarray
            New value-level index array (flat).

        Returns
        -------
        VTKIndexedArray or None
        """
        from ..vtkCommonCore import vtkIndexedArray

        base = self.GetBaseArray()
        if base is None:
            return None
        nc = self.GetNumberOfComponents()
        nt = len(new_idx_np) // nc
        idx_vtk = numpy_support.numpy_to_vtk(
            new_idx_np.astype(numpy.int64))
        try:
            result = vtkIndexedArray[self.dtype.name]()
        except (KeyError, TypeError):
            return None
        result.SetNumberOfComponents(nc)
        result.SetNumberOfTuples(nt)
        result.ConstructBackend(idx_vtk, base)
        result._dataset = self._dataset
        result._association = self._association
        return result

    # ---- indexing -----------------------------------------------------------
    def __getitem__(self, key):
        """Index into the indexed array.

        Scalar index: direct lookup, no materialization.
        Slice / fancy int / boolean mask on axis 0: compose a new
        index array pointing at the same base — stays lazy.
        Tuple indexing (row, col) or other: materialize.
        """
        ntuples = self.GetNumberOfTuples()
        nc = self.GetNumberOfComponents()

        # Scalar tuple index — no materialization needed
        if isinstance(key, (int, numpy.integer)):
            if key < 0:
                key += ntuples
            if key < 0 or key >= ntuples:
                raise IndexError(
                    f"index {key} is out of bounds for axis 0 with size {ntuples}")
            if nc == 1:
                return self.dtype.type(self.GetValue(key))
            else:
                return numpy.array(self.GetTuple(key), dtype=self.dtype)

        # Slice on axis 0 — compose new index array, stay lazy
        if isinstance(key, slice):
            indices = numpy.arange(*key.indices(ntuples))
            new_idx = self._sub_index_array(indices)
            result = self._new_from_sub_indexes(new_idx)
            if result is not None:
                return result

        # Fancy integer array on axis 0
        if isinstance(key, (list, numpy.ndarray)):
            key_arr = numpy.asarray(key)
            if key_arr.dtype == numpy.bool_:
                # Boolean mask
                if key_arr.shape[0] == ntuples:
                    indices = numpy.where(key_arr)[0]
                    new_idx = self._sub_index_array(indices)
                    result = self._new_from_sub_indexes(new_idx)
                    if result is not None:
                        return result
            elif numpy.issubdtype(key_arr.dtype, numpy.integer):
                # Handle negative indices
                key_arr = numpy.where(key_arr < 0, key_arr + ntuples, key_arr)
                new_idx = self._sub_index_array(key_arr)
                result = self._new_from_sub_indexes(new_idx)
                if result is not None:
                    return result

        # Everything else — materialize and index
        return self.__array__()[key]

    def __setitem__(self, key, value):
        raise TypeError("VTKIndexedArray is read-only")

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

    # ---- reduction overrides (delegate to materialized array) ---------------
    def sum(self, axis=None, **kwargs):
        return numpy.asarray(self).sum(axis=axis, **kwargs)

    def min(self, axis=None, **kwargs):
        return numpy.asarray(self).min(axis=axis, **kwargs)

    def max(self, axis=None, **kwargs):
        return numpy.asarray(self).max(axis=axis, **kwargs)

    def mean(self, axis=None, **kwargs):
        return numpy.asarray(self).mean(axis=axis, **kwargs)

    def std(self, axis=None, **kwargs):
        return numpy.asarray(self).std(axis=axis, **kwargs)

    def var(self, axis=None, **kwargs):
        return numpy.asarray(self).var(axis=axis, **kwargs)

    def any(self, axis=None, **kwargs):
        return numpy.asarray(self).any(axis=axis, **kwargs)

    def all(self, axis=None, **kwargs):
        return numpy.asarray(self).all(axis=axis, **kwargs)

    def prod(self, axis=None, **kwargs):
        return numpy.asarray(self).prod(axis=axis, **kwargs)

    # ---- utilities ----------------------------------------------------------
    def __iter__(self):
        for i in range(self.GetNumberOfTuples()):
            yield self[i]

    def __repr__(self):
        return (f"VTKIndexedArray(shape={self.shape}, dtype={self.dtype})")

    def __str__(self):
        return repr(self)


# ---- numpy function overrides -----------------------------------------------

@_override_indexed_numpy(numpy.sum)
def _indexed_sum(a, axis=None, **kwargs):
    if not isinstance(a, VTKIndexedArray):
        return numpy.sum(numpy.asarray(a), axis=axis, **kwargs)
    return a.sum(axis=axis, **kwargs)


@_override_indexed_numpy(numpy.mean)
def _indexed_mean(a, axis=None, **kwargs):
    if not isinstance(a, VTKIndexedArray):
        return numpy.mean(numpy.asarray(a), axis=axis, **kwargs)
    return a.mean(axis=axis, **kwargs)


@_override_indexed_numpy(numpy.min)
def _indexed_min(a, axis=None, **kwargs):
    if not isinstance(a, VTKIndexedArray):
        return numpy.min(numpy.asarray(a), axis=axis, **kwargs)
    return a.min(axis=axis, **kwargs)


@_override_indexed_numpy(numpy.max)
def _indexed_max(a, axis=None, **kwargs):
    if not isinstance(a, VTKIndexedArray):
        return numpy.max(numpy.asarray(a), axis=axis, **kwargs)
    return a.max(axis=axis, **kwargs)


@_override_indexed_numpy(numpy.std)
def _indexed_std(a, axis=None, **kwargs):
    if not isinstance(a, VTKIndexedArray):
        return numpy.std(numpy.asarray(a), axis=axis, **kwargs)
    return a.std(axis=axis, **kwargs)


@_override_indexed_numpy(numpy.var)
def _indexed_var(a, axis=None, **kwargs):
    if not isinstance(a, VTKIndexedArray):
        return numpy.var(numpy.asarray(a), axis=axis, **kwargs)
    return a.var(axis=axis, **kwargs)


@_override_indexed_numpy(numpy.any)
def _indexed_any(a, axis=None, **kwargs):
    if not isinstance(a, VTKIndexedArray):
        return numpy.any(numpy.asarray(a), axis=axis, **kwargs)
    return a.any(axis=axis, **kwargs)


@_override_indexed_numpy(numpy.all)
def _indexed_all(a, axis=None, **kwargs):
    if not isinstance(a, VTKIndexedArray):
        return numpy.all(numpy.asarray(a), axis=axis, **kwargs)
    return a.all(axis=axis, **kwargs)


@_override_indexed_numpy(numpy.prod)
def _indexed_prod(a, axis=None, **kwargs):
    if not isinstance(a, VTKIndexedArray):
        return numpy.prod(numpy.asarray(a), axis=axis, **kwargs)
    return a.prod(axis=axis, **kwargs)


@_override_indexed_numpy(numpy.concatenate)
def _indexed_concatenate(arrays, axis=0, **kwargs):
    converted = []
    for a in arrays:
        if isinstance(a, VTKIndexedArray):
            converted.append(numpy.asarray(a))
        else:
            converted.append(a)
    return numpy.concatenate(converted, axis=axis, **kwargs)


# ---- Register overrides for all indexed array template types -----------------
def _register_indexed_overrides():
    """Register VTKIndexedArray mixin as override for all
    vtkIndexedArray template types."""
    from vtkmodules.vtkCommonCore import vtkIndexedArray

    register_template_overrides(
        VTKIndexedArray, vtkIndexedArray, 'VTKIndexedArray')

_register_indexed_overrides()

"""VTKPartitionedArray — numpy-compatible wrapper for composite datasets.

This module provides the VTKPartitionedArray class that manages a set of
arrays of the same name contained within a composite dataset.  Its main
purpose is to provide a Numpy-type interface to composite data arrays which
are naturally nothing but a collection of vtkDataArrays.

It also registers __array_function__ overrides so that standard numpy calls
(``numpy.sum``, ``numpy.max``, etc.) are dispatched correctly when handed
a VTKPartitionedArray.
"""
import warnings

import numpy
import operator
from typing import Generator, List, Tuple, Union
from math import ceil

from .utils import ArrayAssociation, NoneArray, reshape_append_ones
from ._vtk_array_mixin import make_override_registry

# Save builtins before they are shadowed by the override functions below.
_builtin_min = min
_builtin_max = max
_builtin_sum = sum
_builtin_all = all
_builtin_any = any

# ---------------------------------------------------------------------------
# Numpy __array_function__ dispatch registry
# ---------------------------------------------------------------------------
PARTITIONED_OVERRIDE, _override_numpy = make_override_registry()


class VTKPartitionedArray(object):
    """This class manages a set of arrays of the same name contained
    within a composite dataset. Its main purpose is to provide a
    Numpy-type interface to composite data arrays which are naturally
    nothing but a collection of vtkDataArrays. A VTKPartitionedArray
    makes such a collection appear as a single Numpy array and support
    all array operations that this module and the associated algorithm
    module support. Note that this is not a subclass of a Numpy array
    and as such cannot be passed to native Numpy functions. Instead
    VTK modules should be used to process composite arrays.
    """

    def __init__(self, arrays = [], dataset = None, name = None,
                 association = None):
        """Construct a composite array given a container of
        arrays, a dataset, name and association. It is sufficient
        to define a container of arrays to define a composite array.
        It is also possible to initialize an array by defining
        the dataset, name and array association. In that case,
        the underlying arrays will be created lazily when they
        are needed. It is recommended to use the latter method
        when initializing from an existing composite dataset."""
        self._arrays = arrays
        self.dataset = dataset
        self.name = name
        validAssociation = True
        if association == None:
            for array in self._arrays:
                if hasattr(array, "association"):
                    if association == None:
                        association = array.association
                    elif array.association and association != array.association:
                        validAssociation = False
                        break
        if validAssociation:
            self.association = association
        else:
            self.association = ArrayAssociation.FIELD
        self._initialized = False

    def __init_from_composite(self):
        if self._initialized:
            return

        self._initialized = True

        if self.dataset is None or self.name is None:
            return

        self._arrays = []
        for ds in self.dataset:
            # Use the data_model properties (point_data/cell_data/field_data)
            # which set .dataset and .association on the returned FieldDataBase,
            # ensuring the per-block arrays get metadata propagated via get_array.
            if self.association == 0:   # POINT
                dsa = ds.point_data
            elif self.association == 1: # CELL
                dsa = ds.cell_data
            else:
                dsa = ds.field_data
            self._arrays.append(dsa[self.name])

    def _get_size(self):
        "Returns the number of elements in the array."
        self.__init_from_composite()
        size = numpy.int64(0)
        for a in self._arrays:
            try:
                size += a.size
            except AttributeError:
                pass
        return size

    size = property(_get_size)

    def _get_arrays(self):
        """Returns the internal container of VTKArrays. If necessary,
        this will populate the array list from a composite dataset."""
        self.__init_from_composite()
        return self._arrays

    arrays = property(_get_arrays)

    def __setitem__(self, index, value) -> None:
        """Setter overwritten to defer indexing to underlying VTKArrays.
        For the most part, this will behave like Numpy."""
        self.__init_from_composite()

        partition_sizes = [len(a) if a is not NoneArray else 0 for a in self._arrays]
        offsets = numpy.cumsum([0] + partition_sizes)
        total_size = offsets[-1]

        if isinstance(index, VTKPartitionedArray):
            for array, idx in zip(self._arrays, index._arrays):
                if array is not NoneArray:
                    array[idx] = value
        elif isinstance(index, (numpy.ndarray, list)):
            index = numpy.asarray(index)
            if index.dtype == bool and index.shape == self.shape:
                for array_id, array in enumerate(self._arrays):
                    if array is NoneArray:
                        continue
                    start = offsets[array_id]
                    end   = offsets[array_id + 1]
                    local_mask = index[start:end]
                    array[local_mask] = value
            elif index.ndim == 0:
                self.__setitem__(int(index), value)
            elif index.ndim == 1:
                index = numpy.where(index < 0, index + total_size, index)
                for i, array in enumerate(self._arrays):
                    if array is NoneArray:
                        continue
                    idx_mask = numpy.logical_and(index >= offsets[i], index < offsets[i+1])
                    whr = numpy.nonzero(idx_mask)
                    array[index[whr]-offsets[i]] = value
            else:
              raise IndexError(f"Unsupported index type: {type(index)}")
        else:
          if not isinstance(index, tuple):
              index = (index,)
          global_index = index[0]
          remaining_indices = index[1:]

          if isinstance(global_index, int):
              chunk_idx, local_idx = self._global_to_local_id(global_index, total_size, offsets)
              self._arrays[chunk_idx][local_idx][tuple(remaining_indices)] = value
          elif isinstance(global_index, slice):
              for array, offset, size in zip(self._arrays, offsets, partition_sizes):
                  if array is NoneArray:
                      continue
                  local_slice = self._slice_intersection(global_index, offset, size, total_size)
                  if local_slice is not None:
                      array[(local_slice,) + remaining_indices] = value
          else:
              raise TypeError(f"Unsupported index type: {type(index)}")

    def __getitem__(self, index) -> Union[List, "VTKPartitionedArray", numpy.ndarray]:
        """Getter overwritten to refer indexing to underlying VTKArrays.
        For the most part, this will behave like Numpy."""
        self.__init_from_composite()

        empty = True
        for a in self._arrays:
            if a is not NoneArray:
                empty = False
                dtype = a.dtype
                output_shape = a.shape[1:]
                break
        if len(self._arrays) == 0 or empty:
            raise IndexError("Index out of bounds")

        partition_sizes = [len(a) if a is not NoneArray else 0 for a in self._arrays]
        offsets = numpy.cumsum([0] + partition_sizes)
        total_size = offsets[-1]

        if isinstance(index, VTKPartitionedArray):
            res = []
            for array, idx in zip(self._arrays, index._arrays):
                if array is not NoneArray:
                    res.append(array.__getitem__(idx))
                else:
                    res.append(NoneArray)
            return VTKPartitionedArray(res, dataset=self.dataset)

        if isinstance(index, (numpy.ndarray, list)):
            index = numpy.asarray(index)
            if index.ndim == 0:
                return self[int(index)]
            elif index.ndim == 1:
                index = numpy.where(index < 0, index + total_size, index)
                res = numpy.empty(index.shape + output_shape, dtype=dtype)
                for i, array in enumerate(self._arrays):
                    if array is NoneArray:
                        continue
                    idx_mask = numpy.logical_and(index >= offsets[i], index < offsets[i+1])
                    whr = numpy.nonzero(idx_mask)
                    res[whr] = array[index[whr]-offsets[i]]
                return res
            else:
                raise IndexError("Only 1D Numpy or list indexing is supported for now")

        if not isinstance(index, tuple):
            index = (index,)

        global_index = index[0]
        remaining_indices = index[1:]

        if isinstance(global_index, int):
            chunk_idx, local_idx = self._global_to_local_id(global_index, total_size, offsets)
            result = self._arrays[chunk_idx][local_idx]
            if remaining_indices:
                result = result[tuple(remaining_indices)]
            return result

        if isinstance(global_index, slice):
            step = global_index.step if global_index.step is not None else 1
            result = [NoneArray] * len(self._arrays)
            i = 0
            for (array, offset, size) in zip(self._arrays, offsets, partition_sizes):
                if array is NoneArray:
                    i += 1
                    continue
                local_slice = self._slice_intersection(global_index, offset, size, total_size)
                if local_slice is not None:
                    part = array[local_slice]
                    if remaining_indices:
                        # Slice all dimensions of the array except the first one
                        part = part[(slice(0, None, None),) + remaining_indices]
                    if step > 0:
                        result[i] = part
                    if step < 0:
                        result[-i-1] = part
                    i += 1
            return self.__class__(result, dataset=self.dataset, association=self.association)

        if isinstance(global_index, numpy.ndarray):
            return self[numpy.concatenate([array for array in index])]

        raise TypeError(f"Unsupported index type: {type(index)}")

    def _global_to_local_id(self, global_index, total_size, offsets):
        """Returns the chunk index and the local index of the chunk from the global index"""
        if global_index < 0:
            global_index += total_size
        if not (0 <= global_index < total_size):
            raise IndexError("Index out of bounds")
        i = numpy.searchsorted(offsets, global_index, side='right') - 1
        return i, global_index - offsets[i]

    def _slice_intersection(self, global_slice, chunk_offset, chunk_size, total_length):
        """Returns a local slice into the chunk for the global_slice, or None if no overlap.
        Handles both positive and negative steps."""
        start, stop, step = global_slice.indices(total_length)
        chunk_start = chunk_offset
        chunk_end   = chunk_offset + chunk_size

        if step > 0:
            # Find the first global index in the slice that is in the chunk.
            if start < chunk_start:
                offset_steps = (chunk_start - start + step - 1) // step
                first = start + offset_steps * step
            else:
                first = start
            if first >= stop or first >= chunk_end:
                return None

            # Find the last global index in the slice that is in the chunk.
            max_possible = _builtin_min(stop, chunk_end)
            if first > max_possible:
                last = first
            else:
                num_steps = ceil((max_possible - first) / step)
                last = first + num_steps * step

        else:
            # Find the first global index in the slice that is in the chunk.
            if start >= chunk_end:
                first = chunk_end - 1
                remainder = (start - first) % (-step)
                if remainder != 0:
                    first = first - ( (-step) - remainder )
            else:
                first = start
            if first < chunk_start or first <= stop:
                return None

            # Find the last global index in the slice that is in the chunk.
            if chunk_start > stop:
                last = chunk_start - 1
            else:
                num_steps = ceil((start - chunk_start) / (-step))
                last = start + num_steps * step
                if last <= chunk_start:
                    last = chunk_start
            if last > first or last < stop:
                return None

        # Global to local indices
        local_first = first - chunk_offset
        local_last = last - chunk_offset
        local_last = local_last if local_last != -1 else None # For negative steps

        return slice(local_first, local_last, step)


    def _binop(self, other, op):
        """Used to implement numpy-style numerical operations such as __add__,
        __mul__, etc."""
        self.__init_from_composite()
        res = []
        if type(other) == VTKPartitionedArray:
            for a1, a2 in zip(self._arrays, other.arrays):
                if a1 is not NoneArray and a2 is not NoneArray:
                    l = reshape_append_ones(a1, a2)
                    res.append(op(l[0],l[1]))
                else:
                    res.append(NoneArray)
        else:
            for a in self._arrays:
                if a is not NoneArray:
                    l = reshape_append_ones(a, other)
                    res.append(op(l[0], l[1]))
                else:
                    res.append(NoneArray)
        return VTKPartitionedArray(
            res, dataset=self.dataset, association=self.association)

    def _rbinop(self, other, op):
        """Used to implement numpy-style numerical operations such as __add__,
        __mul__, etc."""
        self.__init_from_composite()
        res = []
        if type(other) == VTKPartitionedArray:
            for a1, a2 in zip(self._arrays, other.arrays):
                if a1 is not NoneArray and a2 is not NoneArray:
                    l = reshape_append_ones(a2,a1)
                    res.append(op(l[0],l[1]))
                else:
                    res.append(NoneArray)
        else:
            for a in self._arrays:
                if a is not NoneArray:
                    l = reshape_append_ones(other, a)
                    res.append(op(l[0], l[1]))
                else:
                    res.append(NoneArray)
        return VTKPartitionedArray(
            res, dataset=self.dataset, association = self.association)

    # Arithmetic (forward + reverse)
    def __add__(self, other):       return self._binop(other, operator.add)
    def __radd__(self, other):      return self._rbinop(other, operator.add)
    def __sub__(self, other):       return self._binop(other, operator.sub)
    def __rsub__(self, other):      return self._rbinop(other, operator.sub)
    def __mul__(self, other):       return self._binop(other, operator.mul)
    def __rmul__(self, other):      return self._rbinop(other, operator.mul)
    def __truediv__(self, other):   return self._binop(other, operator.truediv)
    def __rtruediv__(self, other):  return self._rbinop(other, operator.truediv)
    def __floordiv__(self, other):  return self._binop(other, operator.floordiv)
    def __rfloordiv__(self, other): return self._rbinop(other, operator.floordiv)
    def __mod__(self, other):       return self._binop(other, operator.mod)
    def __rmod__(self, other):      return self._rbinop(other, operator.mod)
    def __pow__(self, other):       return self._binop(other, operator.pow)
    def __rpow__(self, other):      return self._rbinop(other, operator.pow)
    def __lshift__(self, other):    return self._binop(other, operator.lshift)
    def __rlshift__(self, other):   return self._rbinop(other, operator.lshift)
    def __rshift__(self, other):    return self._binop(other, operator.rshift)
    def __rrshift__(self, other):   return self._rbinop(other, operator.rshift)
    def __and__(self, other):       return self._binop(other, operator.and_)
    def __rand__(self, other):      return self._rbinop(other, operator.and_)
    def __xor__(self, other):       return self._binop(other, operator.xor)
    def __rxor__(self, other):      return self._rbinop(other, operator.xor)
    def __or__(self, other):        return self._binop(other, operator.or_)
    def __ror__(self, other):       return self._rbinop(other, operator.or_)

    # Comparison (forward only, matching metaclass)
    def __lt__(self, other):        return self._binop(other, operator.lt)
    def __le__(self, other):        return self._binop(other, operator.le)
    def __eq__(self, other):        return self._binop(other, operator.eq)
    def __ne__(self, other):        return self._binop(other, operator.ne)
    def __ge__(self, other):        return self._binop(other, operator.ge)
    def __gt__(self, other):        return self._binop(other, operator.gt)

    def __str__(self):
        return self.arrays.__str__()

    def astype(self, dtype):
        """Implements numpy array's as array method."""
        res = []
        if self is not NoneArray:
            for a in self.arrays:
                if a is NoneArray:
                    res.append(NoneArray)
                else:
                    res.append(a.astype(dtype))
        return VTKPartitionedArray(
            res, dataset = self.dataset, association = self.association)

    # ------------------------------------------------------------------
    # Properties
    # ------------------------------------------------------------------
    @property
    def dtype(self):
        """Return the dtype of the first non-NoneArray block."""
        for a in self.arrays:
            if a is not NoneArray:
                return a.dtype
        return None

    @property
    def ndim(self):
        """Return the number of dimensions."""
        return len(self.shape)

    @property
    def nbytes(self):
        """Return the total number of bytes consumed by all blocks."""
        self.__init_from_composite()
        total = 0
        for a in self._arrays:
            if a is not NoneArray:
                total += a.nbytes
        return total

    @property
    def T(self):
        """Return the transpose of the materialized array."""
        return self.__array__().T

    @property
    def range(self):
        """Return the (min, max) range across all blocks."""
        lo = None
        hi = None
        for a in self.arrays:
            if a is not NoneArray:
                a_min = numpy.min(a)
                a_max = numpy.max(a)
                if lo is None or a_min < lo:
                    lo = a_min
                if hi is None or a_max > hi:
                    hi = a_max
        if lo is None:
            return (0, 0)
        return (lo, hi)

    # ------------------------------------------------------------------
    # Reduction methods — delegate to numpy free functions which route
    # through __array_function__ overrides
    # ------------------------------------------------------------------
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

    # ------------------------------------------------------------------
    # Shape / layout methods — materialize via __array__()
    # ------------------------------------------------------------------
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
        return numpy.clip(self, a_min, a_max, **kwargs)

    def round(self, decimals=0, **kwargs):
        return numpy.round(self, decimals=decimals, **kwargs)

    def sort(self, axis=0, **kwargs):
        return numpy.sort(self, axis=axis, **kwargs)

    def dot(self, other):
        return self.__array__().dot(other)

    @property
    def shape(self) -> Tuple[int, ...]:
        """Return a tuple representing the shape of the composite array."""
        if not self.arrays:
            return (0,)
        return numpy.shape(self)

    def __len__(self) -> int:
        """Return the total number of elements in the composite array."""
        return self.shape[0]

    def __contains__(self, item) -> bool:
        """Check if the item exists in any of the non-null sub-arrays."""
        return any(item in array for array in self.arrays if array is not NoneArray)

    def __reversed__(self) -> Generator[numpy.ndarray, None, None]:
        """Iterate over all subarrays in the composite array in backward order."""
        for array in reversed(self.arrays):
            if array is not NoneArray:
                for subarray in reversed(array):
                    yield subarray

    def __iter__(self) -> Generator[numpy.ndarray, None, None]:
        """Iterate over all subarrays in the composite array in forward order."""
        for array in self.arrays:
            if array is not NoneArray:
                for subarray in array:
                    yield subarray

    def __array__(self, dtype=None, copy=None) -> numpy.ndarray:
        """Convert the composite array into a single NumPy array."""
        if copy is False:
            raise RuntimeError("VTKPartitionedArray must create a copy to be converted into a Numpy array.")
        return numpy.concatenate([numpy.asarray(a) for a in self.arrays if a is not NoneArray], axis=0, dtype=dtype)

    def __array_function__(self, func, types, args, kwargs):
        """Implements Numpy dispatch mechanism. Functions registered in PARTITIONED_OVERRIDE
        are captured here. See:
        https://numpy.org/doc/stable/user/basics.interoperability.html#the-array-function-protocol"""

        if func not in PARTITIONED_OVERRIDE:
            warnings.warn(
                f"numpy.{func.__name__}() is not optimized for "
                f"VTKPartitionedArray; all blocks will be materialized "
                f"into a single array.",
                stacklevel=2,
            )
            return NotImplemented
        return PARTITIONED_OVERRIDE[func](*args, **kwargs)

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        """Handles Numpy functions calls that takes a fixed number of specific inputs and outputs."""
        if method != '__call__':
            raise NotImplementedError(f"Method {method} is not supported by __array_ufunc__")

        num_chunks = len(self.arrays)
        partition_sizes = [sub.shape[0] for sub in self.arrays]
        total_size = _builtin_sum(partition_sizes) if partition_sizes else 0
        global_indices = numpy.cumsum(partition_sizes)[:-1]

        # Split inputs arguments to each chunk
        # Basically, this returns a 2D array where the first dimension represents each chunck of the
        # composite array and the second dimension represents the data associated for each chunk
        def process_input(input_):
            if isinstance(input_, type(self)):
                if len(input_.arrays) != num_chunks:
                    raise ValueError("Composite operands must have the same number of subarrays")
                return input_.arrays
            elif numpy.isscalar(input_):
                return [input_] * num_chunks
            elif isinstance(input_, numpy.ndarray):
                if input_.ndim >= 1 and input_.shape[0] == total_size:
                    return list(numpy.split(input_, global_indices, axis=0))
                else:
                    return [input_] * num_chunks
            else:
                return [input_] * num_chunks

        splitted_inputs = [process_input(input_) for input_ in inputs]

        results = []
        for i in range(num_chunks):
            arg0 = splitted_inputs[0][i]
            args = [lst[i] for lst in splitted_inputs[1:]]
            if arg0 is NoneArray or (args and args[0] is NoneArray):
                results.append(NoneArray)
                continue
            result = ufunc(arg0, *args, **kwargs)
            results.append(result)

        return self.__class__(results, dataset=self.dataset, association=self.association)


# ---------------------------------------------------------------------------
# Numpy __array_function__ overrides
# ---------------------------------------------------------------------------
# All the functions below are registered in PARTITIONED_OVERRIDE so that
# ``numpy.sum(composite)``, ``numpy.max(composite)``, etc. are dispatched
# correctly.  They use deferred imports to avoid circular dependencies
# with algorithms.py / numpy_algorithms.py.
# ---------------------------------------------------------------------------

@_override_numpy(numpy.bitwise_or)
def bitwise_or(array1, array2):
    """Implements element by element or (bitwise, | in C/C++) operation.
    If one of the arrays is a NoneArray, this will return the array
    that is not NoneArray, treating NoneArray as 0 in the or operation."""
    if type(array1) == VTKPartitionedArray and type(array2) == VTKPartitionedArray:
        res = []
        for a1, a2 in zip(array1.arrays, array2.arrays):
            l = reshape_append_ones(a1, a2)
            res.append(bitwise_or(l[0], l[1]))
        return VTKPartitionedArray(res, dataset = array1.dataset)
    elif type(array1) == VTKPartitionedArray:
        res = []
        for a in array1.arrays :
            l = reshape_append_ones(a, array2)
            res.append(bitwise_or(l[0], l[1]))
        return VTKPartitionedArray(res, dataset = array1.dataset)
    elif array1 is NoneArray:
        return array2
    elif array2 is NoneArray:
        return array1
    else:
        l = reshape_append_ones(array1, array2)
        return numpy.bitwise_or(l[0], l[1])

@_override_numpy(numpy.sum)
def sum(array, axis=None, **kwargs):
    """Local (non-parallel) sum dispatched from numpy.sum()."""
    from ..util.functions import sum as _sum
    return _sum(array, axis=axis, controller=False)

@_override_numpy(numpy.max)
def max(array, axis=None, **kwargs):
    """Local (non-parallel) max dispatched from numpy.max()."""
    from ..util.functions import max as _max
    return _max(array, axis=axis, controller=False)

@_override_numpy(numpy.min)
def min(array, axis=None, **kwargs):
    """Local (non-parallel) min dispatched from numpy.min()."""
    from ..util.functions import min as _min
    return _min(array, axis=axis, controller=False)

@_override_numpy(numpy.all)
def all(array, axis=None, **kwargs):
    """Local (non-parallel) all dispatched from numpy.all()."""
    from ..util.functions import all as _all
    return _all(array, axis=axis, controller=False)

@_override_numpy(numpy.mean)
def mean(array, axis=None, **kwargs):
    """Local (non-parallel) mean dispatched from numpy.mean()."""
    from ..util.functions import mean as _mean
    return _mean(array, axis=axis, controller=False)

@_override_numpy(numpy.var)
def var(array, axis=None, **kwargs):
    """Local (non-parallel) var dispatched from numpy.var()."""
    from ..util.functions import var as _var
    return _var(array, axis=axis, controller=False)

@_override_numpy(numpy.std)
def std(array, axis=None, **kwargs):
    """Local (non-parallel) std dispatched from numpy.std()."""
    from ..util.functions import std as _std
    return _std(array, axis=axis, controller=False)

@_override_numpy(numpy.shape)
def shape(array):
    "Returns the shape (dimensions) of an array."
    if type(array) == VTKPartitionedArray:
        shp = None
        for a in array.arrays:
            if a is not NoneArray:
                if shp is None:
                    shp = list(a.shape)
                else:
                    tmp = a.shape
                    if (len(shp) != len(tmp)):
                        raise ValueError("Expected arrays of same shape")
                    shp[0] += tmp[0]
                    for idx in range(1,len(tmp)):
                        if shp[idx] != tmp[idx]:
                            raise ValueError("Expected arrays of same shape")
        return tuple(shp)
    elif array is NoneArray:
        return ()
    else:
        return numpy.shape(array)

@_override_numpy(numpy.flatnonzero)
def flatnonzero(array):
    """
    Return indices that are non-zero in the flattened version of the input array.
    """
    if array is NoneArray:
        return NoneArray
    elif type(array) == VTKPartitionedArray:
        res = []
        offset = 0
        for arr in array.arrays:
            if arr is NoneArray:
                res.append(NoneArray)
            else:
                res.append(numpy.flatnonzero(arr) + offset)
                offset += arr.size
        return VTKPartitionedArray(res, dataset = array.dataset)
    else:
        return numpy.flatnonzero(array)

def _like_VTKPartitionedArray(array, kind, dtype=None, value=None):
    """Helper to construct like-structured VTKPartitionedArray."""
    arrays = []
    for arr in array.arrays:
        if arr is NoneArray:
            arrays.append(NoneArray)
            continue

        new_dtype = dtype if dtype is not None else arr.dtype

        if kind == "full":
            arrays.append(numpy.full(arr.shape, value, dtype=new_dtype))
        else:
            arrays.append(numpy.empty(arr.shape, dtype=new_dtype))

    return VTKPartitionedArray(arrays, dataset=array.dataset, association=array.association)

@_override_numpy(numpy.zeros_like)
def zeros_like(array, dtype=None):
    """Create a new VTKPartitionedArray filled with 0 of the same shape as array."""
    return _like_VTKPartitionedArray(array, "full", dtype, 0)

@_override_numpy(numpy.ones_like)
def ones_like(array, dtype=None):
    """Create a new VTKPartitionedArray filled with 1 of the same shape as array."""
    return _like_VTKPartitionedArray(array, "full", dtype, 1)

@_override_numpy(numpy.empty_like)
def empty_like(array, dtype=None):
    """Create a new uninitialized VTKPartitionedArray of the same shape as array."""
    return _like_VTKPartitionedArray(array, "empty", dtype)

@_override_numpy(numpy.nonzero)
def nonzero(array, *args):
    """Return the indices of the non-zero elements of the input array.
    Indices are local to each chunk."""
    from ..util.functions import apply_ufunc
    return apply_ufunc(numpy.nonzero, array, args)

@_override_numpy(numpy.where)
def where(array, *args):
    """Returns the location (indices) of an array where the given
    expression is true. Indices are local to each chunk.
    For scalars, it returns a single array of indices.
    For vectors and matrices, it returns two arrays: first with tuple indices,
    second with component indices. The output of this method can be used to
    extract the values from the array also by using it as the index of the [] operator.

    For example:

    >>> algs.where(algs.array([1,2,3]) == 2)
    (array([1]),)

    >>> algs.where(algs.array([[1,2,3], [2,1,1]]) == 2)
    (array([0, 1]), array([1, 0]))

    >>> a = array([[1,2,3], [2,1,1]])
    >>> indices = algs.where(a > 2)
    >>> a[indices]
    array([3])
    """
    from ..util.functions import apply_ufunc
    return apply_ufunc(numpy.where, array, args)

@_override_numpy(numpy.isin)
def isin(array, *args):
    "Test whether each element of a 1-D array is also present in a second array."
    from ..util.functions import apply_ufunc
    return apply_ufunc(numpy.isin, array, args)

@_override_numpy(numpy.any)
def any(array, axis=None, **kwargs):
    """Local (non-parallel) any dispatched from numpy.any()."""
    if type(array) == VTKPartitionedArray:
        if axis is None:
            return _builtin_any(
                numpy.any(a) for a in array.arrays if a is not NoneArray)
        if axis == 0:
            results = [numpy.any(a, axis=0, **kwargs)
                       for a in array.arrays if a is not NoneArray]
            if not results:
                return numpy.array([], dtype=bool)
            combined = results[0]
            for r in results[1:]:
                combined = numpy.logical_or(combined, r)
            return combined
        res = []
        for a in array.arrays:
            if a is not NoneArray:
                res.append(numpy.any(a, axis=axis, **kwargs))
            else:
                res.append(NoneArray)
        return VTKPartitionedArray(
            res, dataset=array.dataset, association=array.association)
    return numpy.any(array, axis=axis, **kwargs)

@_override_numpy(numpy.prod)
def prod(array, axis=None, **kwargs):
    """Local (non-parallel) prod dispatched from numpy.prod()."""
    if type(array) == VTKPartitionedArray:
        if axis is None or axis == 0:
            results = [numpy.prod(a, axis=axis, **kwargs)
                       for a in array.arrays if a is not NoneArray]
            if not results:
                return numpy.intp(1)
            combined = results[0]
            for r in results[1:]:
                combined = combined * r
            return combined
        res = []
        for a in array.arrays:
            if a is not NoneArray:
                res.append(numpy.prod(a, axis=axis, **kwargs))
            else:
                res.append(NoneArray)
        return VTKPartitionedArray(
            res, dataset=array.dataset, association=array.association)
    return numpy.prod(array, axis=axis, **kwargs)

@_override_numpy(numpy.round)
def round_(array, decimals=0, **kwargs):
    """Per-block round dispatched from numpy.round()."""
    if type(array) == VTKPartitionedArray:
        res = []
        for a in array.arrays:
            if a is not NoneArray:
                res.append(numpy.round(a, decimals=decimals, **kwargs))
            else:
                res.append(NoneArray)
        return VTKPartitionedArray(res, dataset=array.dataset, association=array.association)
    return numpy.round(array, decimals=decimals, **kwargs)

@_override_numpy(numpy.clip)
def clip(array, a_min=None, a_max=None, **kwargs):
    """Per-block clip dispatched from numpy.clip()."""
    if type(array) == VTKPartitionedArray:
        res = []
        for a in array.arrays:
            if a is not NoneArray:
                res.append(numpy.clip(a, a_min, a_max, **kwargs))
            else:
                res.append(NoneArray)
        return VTKPartitionedArray(res, dataset=array.dataset, association=array.association)
    return numpy.clip(array, a_min, a_max, **kwargs)

@_override_numpy(numpy.sort)
def sort(array, axis=0, **kwargs):
    """Sort dispatched from numpy.sort(). Per-block for axis != 0."""
    if type(array) == VTKPartitionedArray:
        if axis is not None and axis != 0:
            res = []
            for a in array.arrays:
                if a is not NoneArray:
                    res.append(numpy.sort(a, axis=axis, **kwargs))
                else:
                    res.append(NoneArray)
            return VTKPartitionedArray(
                res, dataset=array.dataset, association=array.association)
        return numpy.sort(numpy.asarray(array), axis=axis, **kwargs)
    return numpy.sort(array, axis=axis, **kwargs)

@_override_numpy(numpy.argmin)
def argmin(array, axis=None, **kwargs):
    """argmin dispatched from numpy.argmin(). Per-block for axis > 0."""
    if type(array) == VTKPartitionedArray:
        if axis is not None and axis != 0:
            res = []
            for a in array.arrays:
                if a is not NoneArray:
                    res.append(numpy.argmin(a, axis=axis, **kwargs))
                else:
                    res.append(NoneArray)
            return VTKPartitionedArray(
                res, dataset=array.dataset, association=array.association)
        return numpy.argmin(numpy.asarray(array), axis=axis, **kwargs)
    return numpy.argmin(array, axis=axis, **kwargs)

@_override_numpy(numpy.argmax)
def argmax(array, axis=None, **kwargs):
    """argmax dispatched from numpy.argmax(). Per-block for axis > 0."""
    if type(array) == VTKPartitionedArray:
        if axis is not None and axis != 0:
            res = []
            for a in array.arrays:
                if a is not NoneArray:
                    res.append(numpy.argmax(a, axis=axis, **kwargs))
                else:
                    res.append(NoneArray)
            return VTKPartitionedArray(
                res, dataset=array.dataset, association=array.association)
        return numpy.argmax(numpy.asarray(array), axis=axis, **kwargs)
    return numpy.argmax(array, axis=axis, **kwargs)

@_override_numpy(numpy.cumsum)
def cumsum(array, axis=None, **kwargs):
    """cumsum dispatched from numpy.cumsum(). Per-block for axis > 0."""
    if type(array) == VTKPartitionedArray:
        if axis is not None and axis != 0:
            res = []
            for a in array.arrays:
                if a is not NoneArray:
                    res.append(numpy.cumsum(a, axis=axis, **kwargs))
                else:
                    res.append(NoneArray)
            return VTKPartitionedArray(
                res, dataset=array.dataset, association=array.association)
        return numpy.cumsum(numpy.asarray(array), axis=axis, **kwargs)
    return numpy.cumsum(array, axis=axis, **kwargs)

@_override_numpy(numpy.cumprod)
def cumprod(array, axis=None, **kwargs):
    """cumprod dispatched from numpy.cumprod(). Per-block for axis > 0."""
    if type(array) == VTKPartitionedArray:
        if axis is not None and axis != 0:
            res = []
            for a in array.arrays:
                if a is not NoneArray:
                    res.append(numpy.cumprod(a, axis=axis, **kwargs))
                else:
                    res.append(NoneArray)
            return VTKPartitionedArray(
                res, dataset=array.dataset, association=array.association)
        return numpy.cumprod(numpy.asarray(array), axis=axis, **kwargs)
    return numpy.cumprod(array, axis=axis, **kwargs)

@_override_numpy(numpy.unique)
def unique(array, **kwargs):
    """unique dispatched from numpy.unique()."""
    if type(array) == VTKPartitionedArray:
        return numpy.unique(numpy.asarray(array), **kwargs)
    return numpy.unique(array, **kwargs)

@_override_numpy(numpy.concatenate)
def concatenate(arrays, axis=0, **kwargs):
    """Concatenate all blocks of composite arrays into a flat array."""
    if isinstance(arrays, (list, tuple)) and \
       _builtin_any(type(a) == VTKPartitionedArray for a in arrays):
        all_parts = []
        for a in arrays:
            if type(a) == VTKPartitionedArray:
                for sub in a.arrays:
                    if sub is not NoneArray:
                        all_parts.append(numpy.asarray(sub))
            else:
                all_parts.append(numpy.asarray(a))
        if not all_parts:
            return numpy.array([])
        return numpy.concatenate(all_parts, axis=axis, **kwargs)
    return numpy.concatenate(arrays, axis=axis, **kwargs)

@_override_numpy(numpy.expand_dims)
def expand_dims(array1, val2):
    """Insert a new dimension, corresponding to a given
    position in the array shape. In VTK, this function's main use is to
    enable an operator to work on a vector and a scalar field. For example,
    say you want to divide each component of a vector by the magnitude of
    that vector. You might try this:

    >>> v
    VTKArray([[ 1.,  1.,  1.],
           [ 1.,  1.,  1.],
           [ 1.,  1.,  1.],
           [ 1.,  1.,  1.],
           [ 1.,  1.,  1.]])
    >>> algs.mag(v)
    VTKArray([ 1.73205081,  1.73205081,  1.73205081,  1.73205081,  1.73205081])
    >>> v / algs.mag(v)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    ValueError: operands could not be broadcast together with shapes (5,3) (5)

    The division operator does not know how to map a scalar to a vector
    due to a mismatch in dimensions. This can be solved by making the
    scalar a vector of 1 component (increasing its dimension to 2) as follows:

    >>> v / np.expand_dims(algs.mag(v), 1)
    VTKArray([[ 0.57735027,  0.57735027,  0.57735027],
           [ 0.57735027,  0.57735027,  0.57735027],
           [ 0.57735027,  0.57735027,  0.57735027],
           [ 0.57735027,  0.57735027,  0.57735027],
           [ 0.57735027,  0.57735027,  0.57735027]])"""
    from ..util.functions import apply_dfunc
    return apply_dfunc(numpy.expand_dims, array1, val2)

@_override_numpy(numpy.count_nonzero)
def count_nonzero(array, axis=None, **kwargs):
    """Per-block count_nonzero, results summed."""
    if type(array) == VTKPartitionedArray:
        if axis is None or axis == 0:
            results = [numpy.count_nonzero(a, axis=axis, **kwargs)
                       for a in array.arrays if a is not NoneArray]
            if not results:
                return numpy.intp(0)
            combined = results[0]
            for r in results[1:]:
                combined = combined + r
            return combined
        res = []
        for a in array.arrays:
            if a is not NoneArray:
                res.append(numpy.count_nonzero(a, axis=axis, **kwargs))
            else:
                res.append(NoneArray)
        return VTKPartitionedArray(
            res, dataset=array.dataset, association=array.association)
    return numpy.count_nonzero(array, axis=axis, **kwargs)

@_override_numpy(numpy.full_like)
def full_like(array, fill_value, dtype=None, **kwargs):
    """Per-block full_like."""
    if type(array) == VTKPartitionedArray:
        return _like_VTKPartitionedArray(array, "full", dtype, fill_value)
    return numpy.full_like(array, fill_value, dtype=dtype, **kwargs)

@_override_numpy(numpy.copy)
def copy(array, **kwargs):
    """Per-block copy."""
    if type(array) == VTKPartitionedArray:
        res = []
        for a in array.arrays:
            if a is not NoneArray:
                res.append(numpy.copy(a, **kwargs))
            else:
                res.append(NoneArray)
        return VTKPartitionedArray(
            res, dataset=array.dataset, association=array.association)
    return numpy.copy(array, **kwargs)

@_override_numpy(numpy.average)
def average(array, axis=None, weights=None, **kwargs):
    """Per-block weighted average."""
    if type(array) == VTKPartitionedArray:
        if axis is None or axis == 0:
            if weights is None:
                # Unweighted: delegate to mean
                from ..util.functions import mean as _mean
                return _mean(array, axis=axis, controller=False)
            # Weighted average: sum(a*w) / sum(w)
            w_arrays = weights
            if type(weights) == VTKPartitionedArray:
                w_blocks = weights.arrays
            else:
                w_blocks = [weights] * len(array.arrays)
            numer = None
            denom = None
            for a, w in zip(array.arrays, w_blocks):
                if a is not NoneArray:
                    aw = numpy.sum(a * w, axis=axis)
                    sw = numpy.sum(w, axis=axis)
                    if numer is None:
                        numer = aw.astype(numpy.float64)
                        denom = sw.astype(numpy.float64)
                    else:
                        numer = numer + aw
                        denom = denom + sw
            if numer is None:
                return numpy.float64(0)
            return numer / denom
        # axis > 0: per-block
        if weights is not None and type(weights) == VTKPartitionedArray:
            res = []
            for a, w in zip(array.arrays, weights.arrays):
                if a is not NoneArray:
                    res.append(numpy.average(a, axis=axis, weights=w, **kwargs))
                else:
                    res.append(NoneArray)
        else:
            res = []
            for a in array.arrays:
                if a is not NoneArray:
                    res.append(numpy.average(a, axis=axis, weights=weights, **kwargs))
                else:
                    res.append(NoneArray)
        return VTKPartitionedArray(
            res, dataset=array.dataset, association=array.association)
    return numpy.average(array, axis=axis, weights=weights, **kwargs)

@_override_numpy(numpy.histogram)
def histogram(array, bins=10, range=None, density=None, weights=None, **kwargs):
    """Per-block histogram with consistent bins, counts summed."""
    if type(array) == VTKPartitionedArray:
        # First pass: determine bin edges if not provided explicitly
        if isinstance(bins, (int, numpy.integer)):
            if range is None:
                from ..util.functions import min as _min, max as _max
                lo = _min(array, controller=False)
                hi = _max(array, controller=False)
                range = (float(lo), float(hi))
            bins = numpy.linspace(range[0], range[1], int(bins) + 1)
            range = None  # already baked into edges

        # Second pass: accumulate per-block counts
        total_counts = None
        for a in array.arrays:
            if a is not NoneArray:
                counts, edges = numpy.histogram(
                    a, bins=bins, range=range, density=False,
                    weights=weights, **kwargs)
                if total_counts is None:
                    total_counts = counts.astype(numpy.float64)
                else:
                    total_counts += counts
        if total_counts is None:
            total_counts = numpy.zeros(len(bins) - 1, dtype=numpy.float64)
            edges = numpy.asarray(bins)
        if density:
            bin_widths = numpy.diff(edges)
            total = total_counts.sum()
            if total > 0:
                total_counts = total_counts / (total * bin_widths)
        return total_counts, edges
    return numpy.histogram(
        array, bins=bins, range=range, density=density,
        weights=weights, **kwargs)

@_override_numpy(numpy.bincount)
def bincount(array, weights=None, minlength=0, **kwargs):
    """Per-block bincount, counts summed."""
    if type(array) == VTKPartitionedArray:
        total = None
        w_blocks = None
        if weights is not None and type(weights) == VTKPartitionedArray:
            w_blocks = weights.arrays
        for i, a in enumerate(array.arrays):
            if a is not NoneArray:
                w = w_blocks[i] if w_blocks is not None else weights
                counts = numpy.bincount(
                    numpy.asarray(a).ravel(), weights=w,
                    minlength=minlength, **kwargs)
                if total is None:
                    total = counts
                else:
                    # Zero-pad shorter array to match lengths
                    max_len = _builtin_max(len(counts), len(total))
                    if len(total) < max_len:
                        padded = numpy.zeros(max_len, dtype=total.dtype)
                        padded[:len(total)] = total
                        total = padded
                    if len(counts) < max_len:
                        padded = numpy.zeros(max_len, dtype=counts.dtype)
                        padded[:len(counts)] = counts
                        counts = padded
                    total = total + counts
        if total is None:
            return numpy.zeros(minlength, dtype=numpy.intp)
        return total
    return numpy.bincount(array, weights=weights, minlength=minlength, **kwargs)

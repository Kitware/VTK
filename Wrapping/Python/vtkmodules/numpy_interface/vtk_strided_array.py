"""VTKStridedArray — numpy-compatible mixin for vtkStridedArray.

This module provides the VTKStridedArray mixin class and registers
overrides for all vtkStridedArray instantiations so that strided
arrays coming from VTK automatically have numpy-compatible operations.

A vtkStridedArray provides a strided view on a raw buffer:
``value[tupleIdx][compIdx] = buffer[stride * tupleIdx + offset + compIdx]``.
Used in InSitu/Catalyst (vtkConduitArrayUtilities) for zero-copy access
to external memory with interleaved data layouts.

When the underlying buffer is accessible (via ``GetBufferSource()`` or a
stored numpy reference), ``__array__`` returns a zero-copy strided numpy
view — no data is copied.  When the buffer is not accessible (raw-pointer
construction from C++), a DeepCopy fallback is used.
"""
import warnings

import numpy

from ..util import numpy_support
from ._vtk_array_mixin import (
    VTKDataArrayMixin, make_override_registry, register_template_overrides,
)


# Registry for __array_function__ overrides
_STRIDED_OVERRIDE, _override_strided_numpy = make_override_registry()


class VTKStridedArray(VTKDataArrayMixin):
    """A numpy-compatible mixin for strided implicit arrays.

    ``vtkStridedArray`` stores a strided view on a buffer, presenting
    ``buffer[stride * tupleIdx + offset + compIdx]`` as a virtual array.
    This Python mixin adds numpy integration: arithmetic, reductions,
    indexing, and ``to_numpy()``.
    """

    # ---- construction -------------------------------------------------------
    def __init__(self, buffer=None, stride=1, offset=0, **kwargs):
        """Create a strided array.

        Can be called as::

            vtkStridedArray['float32']()                         # empty
            vtkStridedArray['float32'](data, 2)                  # 1D, stride=2
            vtkStridedArray['float32'](data_2d, 3)               # 2D, ncomps from shape[1]
            vtkStridedArray['float32'](data, 3, offset=1)        # with offset

        *buffer* is any array-like (list, numpy array, etc.).  A 1D buffer
        gives a single-component array; a 2D buffer with shape ``(N, M)``
        gives an ``M``-component array (the buffer is flattened internally).

        The data is converted to a VTK buffer via ``numpy_to_vtk`` (zero-copy
        when the dtype already matches) and ``ConstructBackend`` is called.
        """
        # SWIG pointer reconstruction guard
        if buffer is not None and isinstance(buffer, str):
            return
        super().__init__(**kwargs)
        self._dataset = None
        self._association = None
        self._buffer_source = None

        # Convenience construction: (buffer, stride[, offset])
        if buffer is not None:
            buf_np = numpy.asarray(buffer, dtype=self.dtype)
            if buf_np.ndim == 2:
                components = buf_np.shape[1]
                buf_np = buf_np.ravel()
            elif buf_np.ndim == 1:
                components = 1
            else:
                raise ValueError("buffer must be 1D or 2D")

            buf_vtk = numpy_support.numpy_to_vtk(buf_np)
            vtk_buffer = buf_vtk.GetBuffer()
            self.ConstructBackend(vtk_buffer, stride, components, offset)

    def ConstructBackend(self, buffer, stride, components=None, offset=None):
        """Set strided backend parameters.

        *buffer* is a ``vtkAbstractBuffer`` (e.g. obtained via
        ``array.GetBuffer()``).  The C++ side stores the buffer with
        reference counting so the vtkBuffer object stays alive.

        If the buffer was created by ``numpy_to_vtk`` (zero-copy), the
        underlying numpy array reference is also preserved here so that
        the raw memory remains valid.
        """
        # numpy_to_vtk stores the numpy array on the buffer Python wrapper
        # as _numpy_reference.  Each GetBuffer() call returns a fresh
        # wrapper, so grab the reference now before it can be lost.
        if hasattr(buffer, '_numpy_reference'):
            self._numpy_reference = buffer._numpy_reference
        self._buffer_source = buffer
        args = [buffer, stride]
        if components is not None:
            args.append(components)
        if offset is not None:
            args.append(offset)
        super().ConstructBackend(*args)

    def _get_buffer_numpy(self):
        """Return the underlying buffer as a flat numpy array (zero-copy).

        Returns None if no buffer is accessible (raw-pointer construction).
        """
        # Direct numpy reference (from numpy_to_vtk zero-copy path)
        ref = getattr(self, '_numpy_reference', None)
        if ref is not None:
            return numpy.asarray(ref).ravel()

        # Buffer source stored from Python ConstructBackend
        buf = getattr(self, '_buffer_source', None)
        if buf is not None:
            return numpy.asarray(buf)

        # C++ buffer source via GetBufferSource()
        buf = self.GetBufferSource()
        if buf is not None:
            return numpy.asarray(buf)

        return None

    # ---- accessor properties ------------------------------------------------
    @property
    def stride(self):
        """Return the stride of the strided backend."""
        return self.GetStride()

    @property
    def offset(self):
        """Return the offset of the strided backend."""
        return self.GetOffset()

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
        """Return the array as a numpy ndarray (zero-copy when possible)."""
        return self.__array__(dtype=dtype)

    def __array__(self, dtype=None, copy=None):
        """Return a numpy view using strides (zero-copy when possible).

        When the underlying buffer is accessible, creates a strided numpy
        view that maps directly to the buffer memory.  Falls back to
        DeepCopy only when the buffer is not accessible from Python.
        """
        dt = dtype or self.dtype
        nt = self.GetNumberOfTuples()
        nc = self.GetNumberOfComponents()

        if nt == 0:
            shape = (0,) if nc == 1 else (0, nc)
            return numpy.empty(shape, dtype=dt)

        buf_np = self._get_buffer_numpy()
        if buf_np is not None:
            vtk_stride = self.GetStride()
            vtk_offset = self.GetOffset()
            itemsize = buf_np.dtype.itemsize

            if nc == 1:
                result = numpy.lib.stride_tricks.as_strided(
                    buf_np[vtk_offset:],
                    shape=(nt,),
                    strides=(vtk_stride * itemsize,),
                )
            else:
                result = numpy.lib.stride_tricks.as_strided(
                    buf_np[vtk_offset:],
                    shape=(nt, nc),
                    strides=(vtk_stride * itemsize, itemsize),
                )
            result.flags.writeable = False
        else:
            # Fallback: no buffer accessible — DeepCopy to AOS
            from ..vtkCommonCore import vtkAOSDataArrayTemplate

            aos = vtkAOSDataArrayTemplate[self.dtype.name]()
            aos.DeepCopy(self)
            result = numpy_support.vtk_to_numpy(aos)
            if nc > 1:
                result = result.reshape(-1, nc)

        if result.dtype != dt:
            result = result.astype(dt)
        if copy:
            result = result.copy()
        return result

    def __buffer__(self, flags):
        """Override C-level buffer protocol to return strided view."""
        return memoryview(self.__array__())

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        """Handle numpy ufuncs by materializing."""
        if method != '__call__':
            return NotImplemented

        out = kwargs.get('out', None)
        if out is not None:
            return NotImplemented

        # Materialize all strided inputs
        new_inputs = []
        for inp in inputs:
            if isinstance(inp, VTKStridedArray):
                new_inputs.append(numpy.asarray(inp))
            else:
                new_inputs.append(inp)

        result = ufunc(*new_inputs, **kwargs)
        return self._wrap_result(result)

    def __array_function__(self, func, types, args, kwargs):
        """Dispatch numpy functions via override registry."""
        if func in _STRIDED_OVERRIDE:
            return _STRIDED_OVERRIDE[func](*args, **kwargs)

        warnings.warn(
            f"numpy.{func.__name__}() is not optimized for "
            f"VTKStridedArray; the full array will be materialized.",
            stacklevel=2,
        )

        def convert(arg):
            if isinstance(arg, VTKStridedArray):
                return numpy.asarray(arg)
            elif isinstance(arg, (list, tuple)):
                return type(arg)(convert(a) for a in arg)
            return arg

        new_args = [convert(arg) for arg in args]
        return func(*new_args, **kwargs)

    # ---- indexing -----------------------------------------------------------
    def __getitem__(self, key):
        """Index into the strided array.

        Scalar index: direct lookup via GetComponent/GetTuple.
        Everything else: delegates to the numpy strided view.
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
                return self.dtype.type(self.GetComponent(key, 0))
            else:
                return numpy.array(self.GetTuple(key), dtype=self.dtype)

        # Everything else — materialize and index
        return self.__array__()[key]

    def __setitem__(self, key, value):
        raise TypeError("VTKStridedArray is read-only")

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
        return (f"VTKStridedArray(shape={self.shape}, stride={self.stride}, "
                f"offset={self.offset}, dtype={self.dtype})")

    def __str__(self):
        return repr(self)


# ---- numpy function overrides -----------------------------------------------

@_override_strided_numpy(numpy.sum)
def _strided_sum(a, axis=None, **kwargs):
    if not isinstance(a, VTKStridedArray):
        return numpy.sum(numpy.asarray(a), axis=axis, **kwargs)
    return a.sum(axis=axis, **kwargs)


@_override_strided_numpy(numpy.mean)
def _strided_mean(a, axis=None, **kwargs):
    if not isinstance(a, VTKStridedArray):
        return numpy.mean(numpy.asarray(a), axis=axis, **kwargs)
    return a.mean(axis=axis, **kwargs)


@_override_strided_numpy(numpy.min)
def _strided_min(a, axis=None, **kwargs):
    if not isinstance(a, VTKStridedArray):
        return numpy.min(numpy.asarray(a), axis=axis, **kwargs)
    return a.min(axis=axis, **kwargs)


@_override_strided_numpy(numpy.max)
def _strided_max(a, axis=None, **kwargs):
    if not isinstance(a, VTKStridedArray):
        return numpy.max(numpy.asarray(a), axis=axis, **kwargs)
    return a.max(axis=axis, **kwargs)


@_override_strided_numpy(numpy.std)
def _strided_std(a, axis=None, **kwargs):
    if not isinstance(a, VTKStridedArray):
        return numpy.std(numpy.asarray(a), axis=axis, **kwargs)
    return a.std(axis=axis, **kwargs)


@_override_strided_numpy(numpy.var)
def _strided_var(a, axis=None, **kwargs):
    if not isinstance(a, VTKStridedArray):
        return numpy.var(numpy.asarray(a), axis=axis, **kwargs)
    return a.var(axis=axis, **kwargs)


@_override_strided_numpy(numpy.any)
def _strided_any(a, axis=None, **kwargs):
    if not isinstance(a, VTKStridedArray):
        return numpy.any(numpy.asarray(a), axis=axis, **kwargs)
    return a.any(axis=axis, **kwargs)


@_override_strided_numpy(numpy.all)
def _strided_all(a, axis=None, **kwargs):
    if not isinstance(a, VTKStridedArray):
        return numpy.all(numpy.asarray(a), axis=axis, **kwargs)
    return a.all(axis=axis, **kwargs)


@_override_strided_numpy(numpy.prod)
def _strided_prod(a, axis=None, **kwargs):
    if not isinstance(a, VTKStridedArray):
        return numpy.prod(numpy.asarray(a), axis=axis, **kwargs)
    return a.prod(axis=axis, **kwargs)


@_override_strided_numpy(numpy.concatenate)
def _strided_concatenate(arrays, axis=0, **kwargs):
    converted = []
    for a in arrays:
        if isinstance(a, VTKStridedArray):
            converted.append(numpy.asarray(a))
        else:
            converted.append(a)
    return numpy.concatenate(converted, axis=axis, **kwargs)


# ---- Register overrides for all strided array template types -----------------
def _register_strided_overrides():
    """Register VTKStridedArray mixin as override for all
    vtkStridedArray template types."""
    from vtkmodules.vtkCommonCore import vtkStridedArray

    register_template_overrides(
        VTKStridedArray, vtkStridedArray, 'VTKStridedArray')

_register_strided_overrides()

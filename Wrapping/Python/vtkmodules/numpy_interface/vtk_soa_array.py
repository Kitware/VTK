"""VTKSOAArray — numpy-compatible mixin for vtkSOADataArrayTemplate.

This module provides the VTKSOAArray mixin class and registers overrides
for all vtkSOADataArrayTemplate instantiations so that SOA arrays coming from
VTK automatically have numpy-compatible operations.  Per-component buffers are
exposed as zero-copy numpy arrays; when a contiguous AOS array is needed, the
wrapper materialises one on the fly via ``numpy.column_stack``.
"""
import warnings

import numpy

from ..vtkCommonCore import vtkCommand
from ..util import numpy_support
from ._vtk_array_mixin import (
    VTKDataArrayMixin, make_override_registry, register_template_overrides,
)

_UNINITIALIZED = object()

_SOA_OVERRIDE, _override_numpy_soa = make_override_registry()

def _to_ndarray(x):
    """Materialise a VTKSOAArray to ndarray via ``__array__()``.

    Calls ``__array__()`` directly for VTKSOAArray to ensure
    per-component column_stack materialisation.
    """
    if isinstance(x, VTKSOAArray):
        return x.__array__()
    return numpy.asarray(x)

def _soa_to_ndarray_arg(arg):
    """Convert VTKSOAArray to ndarray, recursing into lists/tuples."""
    if isinstance(arg, VTKSOAArray):
        return arg.__array__()
    if isinstance(arg, (list, tuple)):
        return type(arg)(_soa_to_ndarray_arg(x) for x in arg)
    return arg

def _soa_as_ndarray(x):
    """Convert VTKSOAArray to ndarray; pass anything else through."""
    return x.__array__() if isinstance(x, VTKSOAArray) else x

class VTKSOAArray(VTKDataArrayMixin):
    """A numpy-compatible view of VTK's structure-of-arrays data arrays.

    VTK's SOA (structure-of-arrays) layout stores each component in a
    separate contiguous buffer.  This mixin exposes those per-component
    buffers as **zero-copy numpy arrays** and performs element-wise
    operations per-component, avoiding the overhead of converting to
    AOS layout.

    This mixin is automatically applied to all
    ``vtkSOADataArrayTemplate`` instantiations.  Any SOA array that
    crosses from C++ to Python is already a ``VTKSOAArray`` instance.

    Properties
    ----------
    shape : tuple of int
        ``(ntuples,)`` for 1-component, ``(ntuples, ncomps)`` otherwise.
    dtype : numpy.dtype
        The element data type.
    components : list of numpy.ndarray
        Per-component zero-copy numpy array views.
    dataset : vtkDataSet or None
        The owning VTK dataset, when the array is attached to one.
    association : int or None
        The attribute association (POINT, CELL, etc.).

    Per-Component Operations
    ------------------------
    Arithmetic, comparisons, and ufuncs operate on each component
    buffer independently, preserving the SOA layout in the result.
    This avoids the memory overhead and cache penalty of interleaving
    components into AOS form::

        arr = soa_filter.GetOutput().GetPointData().GetArray(0)
        scaled = arr * 2.0          # per-component multiply, stays SOA
        diff = arr1 - arr2          # per-component subtract

    When an operation cannot be decomposed per-component (e.g.
    cross-component reductions with ``axis=1``), the mixin
    materialises a contiguous AOS copy via ``numpy.column_stack``.

    Memory Safety
    -------------
    The mixin observes ``BufferChangedEvent`` from VTK.  If any
    component buffer is reallocated, the cached views are
    automatically invalidated.

    NumPy Integration
    -----------------
    - ``numpy.asarray(arr)`` materialises an AOS copy via column_stack.
    - Reductions (``sum``, ``mean``, ``min``, ``max``, ``std``, ``var``)
      operate per-component without materialisation when possible.
    - ``numpy.concatenate``, ``numpy.where``, ``numpy.clip``, etc. have
      SOA-aware overrides.
    - The ``__buffer__`` override (PEP 688) prevents ``GetVoidPointer``
      from destructively converting SOA storage to AOS.

    Examples
    --------
    Arrays from VTK are already the right type::

        arr = soa_output.point_data["velocity"]
        print(arr.components)      # [x_array, y_array, z_array]
        print(np.sum(arr, axis=0)) # per-component sum, no AOS copy

    Create from numpy::

        from vtkmodules.util.numpy_support import numpy_to_vtk_soa
        soa = numpy_to_vtk_soa([x, y, z], name="coords")

    See Also
    --------
    VTKAOSArray : Mixin for array-of-structures VTK arrays.
    """

    # ---- construction -------------------------------------------------------
    def __init__(self, *args, **kwargs):
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip mixin init to avoid clobbering state.
        if args and isinstance(args[0], str):
            return
        super().__init__(**kwargs)
        self._components_cache = _UNINITIALIZED
        self._observer_id = None
        self._buffers = []
        self._numpy_references = {}
        self._dataset = None
        self._association = None
        if args:
            self._init_from_data(args[0])

    def _init_from_data(self, data):
        """Populate this array from an iterable of per-component iterables.

        Each element of *data* becomes one component.  All components must
        have the same length.  Data is converted to match this array's VTK
        data type.
        """
        arr_dtype = numpy_support.get_numpy_array_type(self.GetDataType())
        arrays = []
        for c in data:
            a = numpy.ascontiguousarray(c)
            if a.dtype != numpy.dtype(arr_dtype):
                a = numpy.ascontiguousarray(a.astype(arr_dtype))
            arrays.append(a)
        if not arrays:
            raise ValueError("data must be a non-empty iterable of component arrays")
        n_tuples = len(arrays[0])
        for i, a in enumerate(arrays):
            if len(a) != n_tuples:
                raise ValueError(
                    f"All component arrays must have the same length; "
                    f"component 0 has {n_tuples}, component {i} has {len(a)}")
        self.SetNumberOfComponents(len(arrays))
        self.SetNumberOfTuples(n_tuples)
        for comp, arr in enumerate(arrays):
            self.SetArray(comp, arr, n_tuples, True, True)
        for comp, arr in enumerate(arrays):
            buf = self.GetComponentBuffer(comp)
            if buf is not None:
                buf._numpy_reference = arr

    def _copy_metadata_to(self, target):
        """Copy dataset/association metadata to target array."""
        if isinstance(target, VTKSOAArray):
            target._dataset = self._dataset
            target._association = self._association
        return target

    def SetArray(self, comp, array, size, *args, **kwargs):
        """Override to prevent garbage collection of numpy arrays passed in."""
        # Ensure numpy array dtype exactly matches VTK's expected type to
        # avoid platform-specific type mismatches (e.g. np.int32 vs C int
        # on Windows).
        if hasattr(array, 'dtype'):
            expected = numpy_support.get_numpy_array_type(self.GetDataType())
            if expected == numpy.intc and array.dtype == numpy.int32:
                array = array.view(numpy.intc)
            if array.dtype != expected:
                array = numpy.ascontiguousarray(array, dtype=expected)
        super().SetArray(comp, array, size, *args, **kwargs)
        self._components_cache = _UNINITIALIZED  # invalidate lazy cache
        if hasattr(array, '__array_interface__'):
            self._numpy_references[comp] = array

    # ---- lazy component initialization --------------------------------------
    @property
    def _component_arrays(self):
        if self._components_cache is _UNINITIALIZED:
            self._init_components()
        if self._components_cache is _UNINITIALIZED:
            return None
        return self._components_cache

    @_component_arrays.setter
    def _component_arrays(self, value):
        self._components_cache = value

    def _init_components(self):
        """Extract per-component numpy arrays from VTK buffers."""
        if self.GetNumberOfTuples() == 0:
            # Don't cache — array may be populated later
            return
        self._components_cache = numpy_support.vtk_soa_to_numpy(self)
        self._setup_observer()

    def _setup_observer(self):
        """Set up BufferChangedEvent observer for memory safety."""
        if self._observer_id is not None:
            return
        observer_id_holder = [None]
        def on_buffer_changed(vtk_obj, event):
            vtk_obj._components_cache = _UNINITIALIZED  # invalidate
            vtk_obj._observer_id = None
            if observer_id_holder[0] is not None:
                vtk_obj.RemoveObserver(observer_id_holder[0])
                observer_id_holder[0] = None
        observer_id = self.AddObserver(
            vtkCommand.BufferChangedEvent, on_buffer_changed)
        observer_id_holder[0] = observer_id
        self._observer_id = observer_id_holder

        # Store references to component buffers for memory safety
        self._buffers = []
        nc = self.GetNumberOfComponents()
        for c in range(nc):
            buf = self.GetComponentBuffer(c)
            if buf is not None:
                self._buffers.append(buf)

    # ---- compatibility properties (used by _SOA_OVERRIDE functions) ----------
    @property
    def _num_tuples(self):
        return self.GetNumberOfTuples()

    @property
    def _num_components(self):
        return self.GetNumberOfComponents()

    @property
    def _dtype(self):
        comps = self._component_arrays
        if comps is not None and len(comps) > 0:
            return comps[0].dtype
        # Empty array — derive dtype from VTK data type
        return numpy_support.get_numpy_array_type(self.GetDataType())

    # ---- numpy-like properties (override base where needed) -----------------
    @property
    def dtype(self):
        return self._dtype

    @property
    def components(self):
        """Return the list of per-component zero-copy numpy arrays."""
        comps = self._component_arrays
        return list(comps) if comps else []

    def to_numpy(self, dtype=None):
        """Return the full materialized array as a numpy ndarray."""
        return self.__array__(dtype=dtype)

    # ---- static helper to build from result arrays --------------------------
    @staticmethod
    def _from_components(components, dtype=None):
        """Build a VTKSOAArray from a list of numpy arrays with VTK backing.

        For bool components, the backing VTK array is
        ``vtkSOADataArrayTemplate<char>`` (int8) since bool and char are
        binary compatible (both 1 byte).
        """
        components = [numpy.ascontiguousarray(c) for c in components]
        actual_dtype = dtype if dtype is not None else components[0].dtype

        # For bool components, back with vtkSOADataArrayTemplate<char>
        if actual_dtype == numpy.bool_:
            vtk_components = [c.view(numpy.int8) for c in components]
        else:
            vtk_components = components

        vtk_array = numpy_support.numpy_to_vtk_soa(vtk_components)
        # Set components directly (bypass lazy init since we already have them)
        vtk_array._component_arrays = list(components)
        vtk_array._setup_observer()
        return vtk_array

    # ---- materialisation (SOA -> AOS copy) ----------------------------------
    def __array__(self, dtype=None, copy=None):
        comps = self._component_arrays
        if comps is None:
            return numpy.empty((0,), dtype=dtype or numpy.float64)
        if len(comps) == 1:
            return numpy.array(comps[0], dtype=dtype, copy=True)
        arr = numpy.column_stack(comps)
        if dtype is not None and arr.dtype != dtype:
            arr = arr.astype(dtype)
        return arr

    def __buffer__(self, flags):
        """Override C-level buffer protocol for SOA arrays.

        Materialises an AOS copy via ``__array__`` so that callers
        expecting a contiguous buffer get correct results without
        altering the underlying SOA storage.
        """
        return memoryview(self.__array__())

    # ---- element-wise ufunc dispatch ----------------------------------------
    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        out = kwargs.get('out', None)
        if out is not None:
            return NotImplemented

        # Empty array — fall back to ndarray operations
        if self._component_arrays is None:
            converted = [_to_ndarray(inp) if isinstance(inp, VTKSOAArray) else inp
                         for inp in inputs]
            return getattr(ufunc, method)(*converted, **kwargs)

        n_comps = self.GetNumberOfComponents()
        n_tuples = self.GetNumberOfTuples()

        # ---- ufunc.reduce(a, axis=...) ------------------------------------
        if method == 'reduce':
            axis = kwargs.get('axis', 0)
            a = inputs[0]
            if not isinstance(a, VTKSOAArray):
                return NotImplemented
            if axis == 0:
                # Reduce each component independently
                kw = {k: v for k, v in kwargs.items() if k != 'axis'}
                return numpy.array(
                    [ufunc.reduce(c, **kw) for c in a._component_arrays],
                    dtype=a._dtype)
            if axis == 1:
                # Reduce across components for each tuple
                kw = {k: v for k, v in kwargs.items() if k != 'axis'}
                result = a._component_arrays[0].copy()
                for c in a._component_arrays[1:]:
                    ufunc(result, c, out=result, **kw)
                return result
            if axis is None:
                # Reduce everything
                kw = {k: v for k, v in kwargs.items() if k != 'axis'}
                per_comp = [ufunc.reduce(c, **kw) for c in a._component_arrays]
                return ufunc.reduce(numpy.array(per_comp), **kw)
            return ufunc.reduce(_to_ndarray(a), **kwargs)

        # ---- ufunc.accumulate(a, axis=...) --------------------------------
        if method == 'accumulate':
            axis = kwargs.get('axis', 0)
            a = inputs[0]
            if not isinstance(a, VTKSOAArray):
                return NotImplemented
            if axis == 0:
                kw = {k: v for k, v in kwargs.items() if k != 'axis'}
                return self._copy_metadata_to(VTKSOAArray._from_components(
                    [ufunc.accumulate(c, **kw) for c in a._component_arrays]))
            return ufunc.accumulate(_to_ndarray(a), **kwargs)

        # ---- ufunc.__call__ -----------------------------------------------
        if method != '__call__':
            return NotImplemented

        # Resolve inputs to per-component lists
        comp_inputs = []  # list of lists, one inner list per component
        incompatible = False
        for inp in inputs:
            pcomps = self._per_component(inp)
            if pcomps is not None:
                comp_inputs.append(pcomps)
            else:
                incompatible = True
                break

        # Fallback: materialise all SOA inputs and delegate to plain ndarrays
        if incompatible:
            converted = [_soa_as_ndarray(inp) for inp in inputs]
            return ufunc(*converted, **kwargs)

        result_comps = []
        for c in range(n_comps):
            args = [ci[c] for ci in comp_inputs]
            result_comps.append(ufunc(*args, **kwargs))

        return self._copy_metadata_to(VTKSOAArray._from_components(result_comps))

    # ---- array function dispatch --------------------------------------------
    def __array_function__(self, func, types, args, kwargs):
        if func in _SOA_OVERRIDE:
            return _SOA_OVERRIDE[func](*args, **kwargs)

        warnings.warn(
            f"numpy.{func.__name__}() is not optimized for "
            f"VTKSOAArray; SOA components will be materialized "
            f"into a contiguous array.",
            stacklevel=2,
        )

        # Universal fallback: materialise SOA args to ndarray and re-call.
        # No recursion risk because converted args are plain ndarray.
        converted_args = tuple(_soa_to_ndarray_arg(a) for a in args)
        converted_kwargs = {k: _soa_to_ndarray_arg(v) for k, v in kwargs.items()}
        return func(*converted_args, **converted_kwargs)

    # ---- indexing -----------------------------------------------------------
    def __getitem__(self, index):
        comps = self._component_arrays
        if comps is None:
            return _to_ndarray(self)[index]
        nc = self.GetNumberOfComponents()
        nt = self.GetNumberOfTuples()
        if isinstance(index, tuple):
            if len(index) == 2:
                row, col = index
                # arr[:, c] – direct component access
                if isinstance(row, slice) and row == slice(None):
                    if isinstance(col, (int, numpy.integer)):
                        c = col if col >= 0 else nc + col
                        return comps[c]
                # arr[slice, c]
                if isinstance(col, (int, numpy.integer)):
                    c = col if col >= 0 else nc + col
                    return comps[c][row]
                # General 2-d indexing – materialise
                return _to_ndarray(self)[index]
            return _to_ndarray(self)[index]

        # Scalar integer index -> gather tuple from all components
        if isinstance(index, (int, numpy.integer)):
            idx = index if index >= 0 else nt + index
            if nc == 1:
                return comps[0][idx]
            return numpy.array([c[idx] for c in comps], dtype=self._dtype)

        # Slice -> per-component slice, return new VTKSOAArray
        if isinstance(index, slice):
            sliced = [c[index] for c in comps]
            if len(sliced[0]) == 0:
                return _to_ndarray(self)[index]
            return VTKSOAArray._from_components(sliced, dtype=self._dtype)

        # Boolean indexing — per-component
        if isinstance(index, numpy.ndarray) and index.dtype == numpy.bool_:
            if index.shape == (self.GetNumberOfTuples(),):
                selected = [c[index] for c in comps]
                if len(selected[0]) > 0:
                    return VTKSOAArray._from_components(
                        selected, dtype=self._dtype)

        # Fancy / other indexing – materialise
        return _to_ndarray(self)[index]

    def __setitem__(self, index, value):
        comps = self._component_arrays
        if comps is None:
            raise IndexError("cannot set items on an empty array")
        nc = self.GetNumberOfComponents()
        nt = self.GetNumberOfTuples()
        if isinstance(index, tuple) and len(index) == 2:
            row, col = index
            if isinstance(col, (int, numpy.integer)):
                c = col if col >= 0 else nc + col
                comps[c][row] = value
                return
        if isinstance(index, (int, numpy.integer)):
            idx = index if index >= 0 else nt + index
            if nc == 1:
                comps[0][idx] = value
            else:
                for c in range(nc):
                    comps[c][idx] = value[c]
            return
        # Slice assignment per-component
        if isinstance(index, slice):
            if nc == 1:
                comps[0][index] = value
            else:
                val = numpy.asarray(value)
                for c in range(nc):
                    comps[c][index] = val[..., c] if val.ndim > 1 else val
            return
        raise IndexError(f"Unsupported index type for VTKSOAArray: {type(index)}")

    # ---- arithmetic operators -----------------------------------------------
    def __neg__(self):
        comps = self._component_arrays
        if comps is None:
            return -_to_ndarray(self)
        return self._copy_metadata_to(
            VTKSOAArray._from_components([-c for c in comps], dtype=self._dtype))

    def __abs__(self):
        comps = self._component_arrays
        if comps is None:
            return numpy.abs(_to_ndarray(self))
        return self._copy_metadata_to(
            VTKSOAArray._from_components([numpy.abs(c) for c in comps], dtype=self._dtype))

    def _per_component(self, other):
        """Decompose other into per-component operands, or return None."""
        if numpy.isscalar(other) or (isinstance(other, numpy.ndarray) and other.ndim == 0):
            return [other] * self._num_components
        nt = self._num_tuples
        nc = self._num_components
        if isinstance(other, VTKSOAArray):
            ocomps = other._component_arrays
            if ocomps is not None and other._num_components == nc and other._num_tuples == nt:
                return ocomps
            return None
        if isinstance(other, numpy.ndarray):
            if other.shape == (nt,):
                return [other] * nc
            if other.shape == (nt, 1):
                flat = other.ravel()
                return [flat] * nc
            if other.shape == (nt, nc):
                return [other[:, i] for i in range(nc)]
            if other.size == 1:
                return [other.flat[0]] * nc
        return None

    def _binop(self, other, op):
        comps = self._component_arrays
        if comps is None:
            return op(_to_ndarray(self), _to_ndarray(other))
        ocomps = self._per_component(other)
        if ocomps is not None:
            return self._copy_metadata_to(VTKSOAArray._from_components(
                [op(a, b) for a, b in zip(comps, ocomps)]))
        return op(_to_ndarray(self), _soa_as_ndarray(other))

    def _rbinop(self, other, op):
        comps = self._component_arrays
        if comps is None:
            return op(other, _to_ndarray(self))
        ocomps = self._per_component(other)
        if ocomps is not None:
            return self._copy_metadata_to(VTKSOAArray._from_components(
                [op(b, a) for a, b in zip(comps, ocomps)]))
        return op(other, _to_ndarray(self))

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

    # comparison — per-component, returns VTKSOAArray with bool dtype
    def __lt__(self, other):  return self._binop(other, numpy.less)
    def __le__(self, other):  return self._binop(other, numpy.less_equal)
    def __eq__(self, other):  return self._binop(other, numpy.equal)
    def __ne__(self, other):  return self._binop(other, numpy.not_equal)
    def __ge__(self, other):  return self._binop(other, numpy.greater_equal)
    def __gt__(self, other):  return self._binop(other, numpy.greater)

    # ---- utilities ----------------------------------------------------------
    def __iter__(self):
        for i in range(self.GetNumberOfTuples()):
            yield self[i]

    def astype(self, dtype):
        comps = self._component_arrays
        if comps is None:
            return _to_ndarray(self).astype(dtype)
        return VTKSOAArray._from_components(
            [c.astype(dtype) for c in comps], dtype=numpy.dtype(dtype))

    def __repr__(self):
        return f"VTKSOAArray(shape={self.shape}, dtype={self._dtype})"

    def __str__(self):
        return str(_to_ndarray(self))

    # ---- SOA-specific overrides of base methods -----------------------------
    @property
    def T(self):
        return _to_ndarray(self).T

    @property
    def nbytes(self):
        comps = self._component_arrays
        if comps is None:
            return 0
        return sum(c.nbytes for c in comps)

    def copy(self, order='C'):
        comps = self._component_arrays
        if comps is None:
            return _to_ndarray(self).copy(order=order)
        return VTKSOAArray._from_components(
            [c.copy() for c in comps], dtype=self._dtype)

    def clip(self, a_min=None, a_max=None, **kwargs):
        comps = self._component_arrays
        if comps is None:
            return numpy.clip(_to_ndarray(self), a_min, a_max, **kwargs)
        return VTKSOAArray._from_components(
            [numpy.clip(c, a_min, a_max, **kwargs) for c in comps])

    def round(self, decimals=0, **kwargs):
        comps = self._component_arrays
        if comps is None:
            return numpy.round(_to_ndarray(self), decimals=decimals, **kwargs)
        return VTKSOAArray._from_components(
            [numpy.round(c, decimals=decimals, **kwargs) for c in comps])

    def sort(self, axis=0, **kwargs):
        """Sort per-component (axis=0) or materialise for other axes."""
        comps = self._component_arrays
        if comps is None:
            return numpy.sort(_to_ndarray(self), axis=axis, **kwargs)
        if axis == 0:
            return VTKSOAArray._from_components(
                [numpy.sort(c, **kwargs) for c in comps])
        return numpy.sort(_to_ndarray(self), axis=axis, **kwargs)


# ---- SOA array function overrides -------------------------------------------
@_override_numpy_soa(numpy.sum)
def _soa_sum(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.sum(_to_ndarray(a), axis=axis, **kwargs)
    if axis is None:
        return sum(numpy.sum(c, **kwargs) for c in a._component_arrays)
    if axis == 0:
        return numpy.array([numpy.sum(c, **kwargs) for c in a._component_arrays], dtype=a.dtype)
    if axis == 1:
        result = a._component_arrays[0].copy()
        for c in a._component_arrays[1:]:
            result = result + c
        return result
    return numpy.sum(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.mean)
def _soa_mean(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.mean(_to_ndarray(a), axis=axis, **kwargs)
    if axis is None:
        return sum(numpy.sum(c, **kwargs) for c in a._component_arrays) / a.size
    if axis == 0:
        return numpy.array([numpy.mean(c, **kwargs) for c in a._component_arrays], dtype=a.dtype)
    if axis == 1:
        result = a._component_arrays[0].copy()
        for c in a._component_arrays[1:]:
            result = result + c
        return result / a._num_components
    return numpy.mean(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.min)
def _soa_min(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.min(_to_ndarray(a), axis=axis, **kwargs)
    if axis is None:
        return min(numpy.min(c, **kwargs) for c in a._component_arrays)
    if axis == 0:
        return numpy.array([numpy.min(c, **kwargs) for c in a._component_arrays], dtype=a.dtype)
    if axis == 1:
        result = a._component_arrays[0].copy()
        for c in a._component_arrays[1:]:
            numpy.minimum(result, c, out=result)
        return result
    return numpy.min(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.max)
def _soa_max(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.max(_to_ndarray(a), axis=axis, **kwargs)
    if axis is None:
        return max(numpy.max(c, **kwargs) for c in a._component_arrays)
    if axis == 0:
        return numpy.array([numpy.max(c, **kwargs) for c in a._component_arrays], dtype=a.dtype)
    if axis == 1:
        result = a._component_arrays[0].copy()
        for c in a._component_arrays[1:]:
            numpy.maximum(result, c, out=result)
        return result
    return numpy.max(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.std)
def _soa_std(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.std(_to_ndarray(a), axis=axis, **kwargs)
    if axis == 0:
        return numpy.array([numpy.std(c, **kwargs) for c in a._component_arrays], dtype=a.dtype)
    if axis == 1:
        ddof = kwargs.get('ddof', 0)
        n = a._num_components
        # mean across components at each tuple
        s = a._component_arrays[0].copy()
        for c in a._component_arrays[1:]:
            s = s + c
        mean = s / n
        # variance across components at each tuple
        var = (a._component_arrays[0] - mean) ** 2
        for c in a._component_arrays[1:]:
            var = var + (c - mean) ** 2
        return numpy.sqrt(var / (n - ddof))
    if axis is None:
        ddof = kwargs.get('ddof', 0)
        n = a.size
        sum_x = sum(numpy.sum(c) for c in a._component_arrays)
        sum_x2 = sum(numpy.sum(c * c) for c in a._component_arrays)
        return numpy.sqrt((sum_x2 - sum_x * sum_x / n) / (n - ddof))
    return numpy.std(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.var)
def _soa_var(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.var(_to_ndarray(a), axis=axis, **kwargs)
    if axis == 0:
        return numpy.array([numpy.var(c, **kwargs) for c in a._component_arrays], dtype=a.dtype)
    if axis == 1:
        ddof = kwargs.get('ddof', 0)
        n = a._num_components
        s = a._component_arrays[0].copy()
        for c in a._component_arrays[1:]:
            s = s + c
        mean = s / n
        var = (a._component_arrays[0] - mean) ** 2
        for c in a._component_arrays[1:]:
            var = var + (c - mean) ** 2
        return var / (n - ddof)
    if axis is None:
        ddof = kwargs.get('ddof', 0)
        n = a.size
        sum_x = sum(numpy.sum(c) for c in a._component_arrays)
        sum_x2 = sum(numpy.sum(c * c) for c in a._component_arrays)
        return (sum_x2 - sum_x * sum_x / n) / (n - ddof)
    return numpy.var(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.any)
def _soa_any(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.any(_to_ndarray(a), axis=axis, **kwargs)
    if axis is None:
        return any(numpy.any(c, **kwargs) for c in a._component_arrays)
    if axis == 0:
        return numpy.array([numpy.any(c, **kwargs) for c in a._component_arrays])
    if axis == 1:
        result = a._component_arrays[0].astype(bool)
        for c in a._component_arrays[1:]:
            result = result | c.astype(bool)
        return result
    return numpy.any(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.all)
def _soa_all(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.all(_to_ndarray(a), axis=axis, **kwargs)
    if axis is None:
        return all(numpy.all(c, **kwargs) for c in a._component_arrays)
    if axis == 0:
        return numpy.array([numpy.all(c, **kwargs) for c in a._component_arrays])
    if axis == 1:
        result = a._component_arrays[0].astype(bool)
        for c in a._component_arrays[1:]:
            result = result & c.astype(bool)
        return result
    return numpy.all(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.prod)
def _soa_prod(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.prod(_to_ndarray(a), axis=axis, **kwargs)
    if axis is None:
        result = numpy.prod(a._component_arrays[0], **kwargs)
        for c in a._component_arrays[1:]:
            result *= numpy.prod(c, **kwargs)
        return result
    if axis == 0:
        return numpy.array([numpy.prod(c, **kwargs) for c in a._component_arrays], dtype=a.dtype)
    if axis == 1:
        result = a._component_arrays[0].copy()
        for c in a._component_arrays[1:]:
            result = result * c
        return result
    return numpy.prod(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.argmin)
def _soa_argmin(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.argmin(_to_ndarray(a), axis=axis, **kwargs)
    if axis == 0:
        return numpy.array([numpy.argmin(c, **kwargs) for c in a._component_arrays])
    if axis == 1:
        # For each tuple, find which component has the min value
        best = a._component_arrays[0].copy()
        result = numpy.zeros(a._num_tuples, dtype=numpy.intp)
        for j, c in enumerate(a._component_arrays[1:], 1):
            mask = c < best
            result[mask] = j
            numpy.minimum(best, c, out=best)
        return result
    if axis is None:
        # Find global argmin; return index into C-order flattened layout
        best_val = numpy.min(a._component_arrays[0])
        best_row = numpy.argmin(a._component_arrays[0])
        best_col = 0
        for j, c in enumerate(a._component_arrays[1:], 1):
            cmin = numpy.min(c)
            if cmin < best_val:
                best_val = cmin
                best_row = numpy.argmin(c)
                best_col = j
        if a._num_components == 1:
            return best_row
        return best_row * a._num_components + best_col
    return numpy.argmin(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.argmax)
def _soa_argmax(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.argmax(_to_ndarray(a), axis=axis, **kwargs)
    if axis == 0:
        return numpy.array([numpy.argmax(c, **kwargs) for c in a._component_arrays])
    if axis == 1:
        best = a._component_arrays[0].copy()
        result = numpy.zeros(a._num_tuples, dtype=numpy.intp)
        for j, c in enumerate(a._component_arrays[1:], 1):
            mask = c > best
            result[mask] = j
            numpy.maximum(best, c, out=best)
        return result
    if axis is None:
        best_val = numpy.max(a._component_arrays[0])
        best_row = numpy.argmax(a._component_arrays[0])
        best_col = 0
        for j, c in enumerate(a._component_arrays[1:], 1):
            cmax = numpy.max(c)
            if cmax > best_val:
                best_val = cmax
                best_row = numpy.argmax(c)
                best_col = j
        if a._num_components == 1:
            return best_row
        return best_row * a._num_components + best_col
    return numpy.argmax(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.cumsum)
def _soa_cumsum(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.cumsum(_to_ndarray(a), axis=axis, **kwargs)
    if axis == 0:
        return VTKSOAArray._from_components(
            [numpy.cumsum(c, **kwargs) for c in a._component_arrays])
    return numpy.cumsum(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.cumprod)
def _soa_cumprod(a, axis=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.cumprod(_to_ndarray(a), axis=axis, **kwargs)
    if axis == 0:
        return VTKSOAArray._from_components(
            [numpy.cumprod(c, **kwargs) for c in a._component_arrays])
    return numpy.cumprod(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.concatenate)
def _soa_concatenate(arrays, axis=0, **kwargs):
    # Only handle list/tuple of VTKSOAArrays along axis 0
    if not isinstance(arrays, (list, tuple)):
        return NotImplemented
    soa_arrays = [a for a in arrays if isinstance(a, VTKSOAArray)]
    if len(soa_arrays) != len(arrays):
        # Mixed types — materialize all
        converted = [_to_ndarray(a) if isinstance(a, VTKSOAArray) else a for a in arrays]
        return numpy.concatenate(converted, axis=axis, **kwargs)
    if axis != 0:
        return numpy.concatenate([_to_ndarray(a) for a in arrays], axis=axis, **kwargs)
    n_comps = soa_arrays[0]._num_components
    if any(a._num_components != n_comps for a in soa_arrays[1:]):
        return numpy.concatenate([_to_ndarray(a) for a in arrays], axis=axis, **kwargs)
    result_comps = []
    for c in range(n_comps):
        result_comps.append(numpy.concatenate([a._component_arrays[c] for a in soa_arrays], **kwargs))
    return VTKSOAArray._from_components(result_comps)

@_override_numpy_soa(numpy.clip)
def _soa_clip(a, a_min=None, a_max=None, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.clip(_to_ndarray(a), a_min, a_max, **kwargs)
    return VTKSOAArray._from_components(
        [numpy.clip(c, a_min, a_max, **kwargs) for c in a._component_arrays])

@_override_numpy_soa(numpy.sort)
def _soa_sort(a, axis=0, **kwargs):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.sort(_to_ndarray(a), axis=axis, **kwargs)
    if axis == 0:
        return VTKSOAArray._from_components(
            [numpy.sort(c, **kwargs) for c in a._component_arrays])
    return numpy.sort(_to_ndarray(a), axis=axis, **kwargs)

@_override_numpy_soa(numpy.where)
def _soa_where(condition, x=None, y=None):
    # 1-arg form: numpy.where(condition) — returns index tuples, materialise
    if x is None and y is None:
        return numpy.where(_to_ndarray(condition) if isinstance(condition, VTKSOAArray) else condition)
    # 3-arg form: per-component
    cond = condition
    if not isinstance(cond, VTKSOAArray) and not isinstance(x, VTKSOAArray) and not isinstance(y, VTKSOAArray):
        return numpy.where(cond, x, y)
    # Find a reference SOA array
    ref = cond if isinstance(cond, VTKSOAArray) else (x if isinstance(x, VTKSOAArray) else y)
    if ref._component_arrays is None:
        return numpy.where(
            _to_ndarray(cond) if isinstance(cond, VTKSOAArray) else cond,
            _to_ndarray(x) if isinstance(x, VTKSOAArray) else x,
            _to_ndarray(y) if isinstance(y, VTKSOAArray) else y)
    nc = ref._num_components
    def _get_comps(val):
        if isinstance(val, VTKSOAArray) and val._component_arrays is not None:
            return val._component_arrays
        if numpy.isscalar(val) or (isinstance(val, numpy.ndarray) and val.ndim == 0):
            return [val] * nc
        return None
    cond_c = _get_comps(cond)
    x_c = _get_comps(x)
    y_c = _get_comps(y)
    if cond_c is not None and x_c is not None and y_c is not None:
        return VTKSOAArray._from_components(
            [numpy.where(cond_c[c], x_c[c], y_c[c]) for c in range(nc)])
    # Fallback: materialise
    return numpy.where(
        _to_ndarray(cond) if isinstance(cond, VTKSOAArray) else cond,
        _to_ndarray(x) if isinstance(x, VTKSOAArray) else x,
        _to_ndarray(y) if isinstance(y, VTKSOAArray) else y)

@_override_numpy_soa(numpy.isin)
def _soa_isin(element, test_elements, **kwargs):
    if not isinstance(element, VTKSOAArray):
        return numpy.isin(element, test_elements, **kwargs)
    if element._component_arrays is None:
        return numpy.isin(_to_ndarray(element), test_elements, **kwargs)
    return VTKSOAArray._from_components(
        [numpy.isin(c, test_elements, **kwargs) for c in element._component_arrays])

@_override_numpy_soa(numpy.round)
def _soa_round(a, decimals=0, out=None):
    if not isinstance(a, VTKSOAArray):
        return numpy.round(a, decimals=decimals, out=out)
    if a._component_arrays is None:
        return numpy.round(_to_ndarray(a), decimals=decimals, out=out)
    if out is not None:
        return numpy.round(_to_ndarray(a), decimals=decimals, out=out)
    return VTKSOAArray._from_components(
        [numpy.round(c, decimals=decimals) for c in a._component_arrays])

@_override_numpy_soa(numpy.dot)
def _soa_dot(a, b):
    if not isinstance(a, VTKSOAArray):
        return NotImplemented
    if a._component_arrays is None:
        return numpy.dot(_to_ndarray(a), _to_ndarray(b))
    b = _to_ndarray(b)
    if b.ndim == 1 and len(b) == a._num_components:
        # Matrix-vector: each component scaled by corresponding element of b,
        # then summed across components.  Result is 1-D array of length n_tuples.
        result = a._component_arrays[0] * b[0]
        for j in range(1, a._num_components):
            result = result + a._component_arrays[j] * b[j]
        return result
    return numpy.dot(_to_ndarray(a), b)


# ---- Register overrides for all SOA template types --------------------------
def _register_soa_overrides():
    """Register VTKSOAArray mixin as override for all SOA template types."""
    from vtkmodules.vtkCommonCore import vtkSOADataArrayTemplate

    register_template_overrides(
        VTKSOAArray, vtkSOADataArrayTemplate, 'VTKSOAArray')

_register_soa_overrides()

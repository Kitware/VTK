"""VTKStructuredPointArray — lazy numpy-compatible wrapper for structured points.

This module provides VTKStructuredPointArray and VTKStructuredAxisArray.
VTKStructuredPointArray is registered as an override on the
``vtkStructuredPointArray`` template so that the coordinate array returned
by ``vtkImageData.points.data`` and ``vtkRectilinearGrid.points.data``
exposes a numpy-compatible surface without materializing the full
(N, 3) array.

For a grid with dimensions (nx, ny, nz), only O(nx + ny + nz) storage is
needed instead of O(nx * ny * nz * 3).  Ufuncs and scalar arithmetic
operate per-axis and stay lazy, producing another VTKStructuredPointArray
whose backend is built via ``ConstructBackend``.  Reductions (sum, min,
max, mean) use optimized O(nx+ny+nz) formulas.

VTK uses Fortran ordering where i increases fastest:
    flat_idx = i + j * nx + k * nx * ny
"""

import numpy

from ..util import numpy_support
from ..vtkCommonCore import vtkWeakReference
from ._vtk_array_mixin import VTKDataArrayMixin


# Registry for __array_function__ overrides
_STRUCTURED_POINT_OVERRIDE = {}


def _override_structured_point_numpy(numpy_function):
    """Register an __array_function__ override for VTKStructuredPointArray."""
    def decorator(func):
        _STRUCTURED_POINT_OVERRIDE[numpy_function] = func
        return func
    return decorator


def _lazy_dtype_supported(axes):
    """Whether a structured-point array can hold the per-axis results *axes*.

    A comparison gives bool axes, and there is no bool structured-point
    array; the caller materializes instead.
    """
    from ..vtkCommonCore import vtkStructuredPointArray
    return numpy.dtype(axes[0].dtype).name in vtkStructuredPointArray.keys()


class VTKStructuredAxisArray:
    """A lazy 1D array representing one coordinate component (X, Y, or Z)
    of a structured point array.

    For a structured grid with dims (nx, ny, nz), there are only nx unique X
    values, ny unique Y values, and nz unique Z values.  This class stores
    just the unique values and expands them lazily when needed.

    Repeat patterns (Fortran ordering, i fastest):
    - Axis 0 (X): X[i] appears at indices where flat_idx % nx == i
    - Axis 1 (Y): Y[j] appears at indices where (flat_idx // nx) % ny == j
    - Axis 2 (Z): Z[k] appears at indices where flat_idx // (nx * ny) == k
    """

    def __init__(self, values, axis, dims, dataset=None):
        """
        Parameters
        ----------
        values : numpy array
            The unique coordinate values for this axis.
        axis : int
            Which axis (0=X, 1=Y, 2=Z).
        dims : tuple of 3 ints
            Grid dimensions (nx, ny, nz).
        dataset : vtkDataObject, optional
            The owning dataset (kept as weak reference).
        """
        self.VTKObject = None
        self._values = numpy.asarray(values)
        self._axis = axis
        self._dims = dims
        self._dataset = None
        if dataset is not None:
            self._dataset = vtkWeakReference()
            self._dataset.Set(dataset)

    @property
    def shape(self):
        nx, ny, nz = self._dims
        return (nx * ny * nz,)

    @property
    def dtype(self):
        return self._values.dtype

    @property
    def ndim(self):
        return 1

    @property
    def size(self):
        return self.shape[0]

    def __len__(self):
        return self.shape[0]

    def __repr__(self):
        return (f"VTKStructuredAxisArray(axis={self._axis}, dims={self._dims}, "
                f"unique_values={len(self._values)}, dtype={self.dtype})")

    def __array__(self, dtype=None, **kwargs):
        """Materialize the full 1D array using Fortran ordering (i fastest)."""
        nx, ny, nz = self._dims
        if self._axis == 0:
            result = numpy.tile(self._values, ny * nz)
        elif self._axis == 1:
            result = numpy.tile(numpy.repeat(self._values, nx), nz)
        else:  # axis == 2
            result = numpy.repeat(self._values, nx * ny)

        if dtype is not None:
            result = result.astype(dtype)
        return result

    def __getitem__(self, index):
        """Index into the array using Fortran ordering."""
        if isinstance(index, (int, numpy.integer)):
            nx, ny, nz = self._dims
            n = nx * ny * nz
            if index < 0:
                index += n
            if index < 0 or index >= n:
                raise IndexError(
                    f"index {index} is out of bounds for axis 0 with size {n}")

            if self._axis == 0:
                return self._values[index % nx]
            elif self._axis == 1:
                return self._values[(index // nx) % ny]
            else:  # axis == 2
                return self._values[index // (nx * ny)]
        else:
            return numpy.asarray(self)[index]

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        """Handle numpy ufuncs.  Unary ufuncs and scalar operations stay lazy."""
        if method != '__call__':
            return NotImplemented

        # Unary ufunc
        if len(inputs) == 1 and isinstance(inputs[0], VTKStructuredAxisArray):
            new_values = ufunc(self._values)
            return VTKStructuredAxisArray(new_values, self._axis, self._dims)

        # Binary ufunc with scalar or same-axis array
        if len(inputs) == 2:
            self_input = None
            other_input = None
            for inp in inputs:
                if isinstance(inp, VTKStructuredAxisArray):
                    self_input = inp
                else:
                    other_input = inp

            if self_input is not None and other_input is not None:
                if numpy.isscalar(other_input) or (
                        isinstance(other_input, numpy.ndarray)
                        and other_input.ndim == 0):
                    if inputs[0] is self_input:
                        new_values = ufunc(self_input._values, other_input)
                    else:
                        new_values = ufunc(other_input, self_input._values)
                    return VTKStructuredAxisArray(
                        new_values, self_input._axis, self_input._dims)

                # Two VTKStructuredAxisArrays with the same axis pattern
                if (isinstance(other_input, VTKStructuredAxisArray)
                        and self_input._axis == other_input._axis
                        and self_input._dims == other_input._dims):
                    new_values = ufunc(self_input._values, other_input._values)
                    return VTKStructuredAxisArray(
                        new_values, self_input._axis, self_input._dims)

        # Fall back to materialization
        materialized = [numpy.asarray(x) if isinstance(x, VTKStructuredAxisArray)
                        else x for x in inputs]
        return ufunc(*materialized, **kwargs)

    # Arithmetic operators
    def __add__(self, other):       return numpy.add(self, other)
    def __radd__(self, other):      return numpy.add(other, self)
    def __sub__(self, other):       return numpy.subtract(self, other)
    def __rsub__(self, other):      return numpy.subtract(other, self)
    def __mul__(self, other):       return numpy.multiply(self, other)
    def __rmul__(self, other):      return numpy.multiply(other, self)
    def __truediv__(self, other):   return numpy.true_divide(self, other)
    def __rtruediv__(self, other):  return numpy.true_divide(other, self)
    def __neg__(self):              return numpy.negative(self)
    def __pow__(self, other):       return numpy.power(self, other)
    def __rpow__(self, other):      return numpy.power(other, self)


class VTKStructuredPointArray(VTKDataArrayMixin):
    """Numpy-compatible mixin for vtkStructuredPointArray template instances.

    Registered as an override for every ``vtkStructuredPointArray[dtype]``
    instantiation.  Every structured-point array returned by VTK (such as
    ``vtkImageData.points.data`` or ``vtkRectilinearGrid.points.data``) is
    an instance of this class.  Ufunc and scalar results also come back
    as instances of this class, built in-place via ``ConstructBackend``.

    Axis coordinates are read through the C++ ``GetXCoordinates()`` /
    ``GetYCoordinates()`` / ``GetZCoordinates()`` accessors so the wrapper
    stays in lockstep with the backing array.  With an identity direction
    matrix (the common case), indexing and ufuncs operate per-axis and
    stay lazy; reductions use O(nx+ny+nz) formulas.  With a non-identity
    direction matrix, operations fall back to a full materialization.
    """

    # ---- construction -------------------------------------------------------
    def __init__(self, *args, **kwargs):
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip mixin init to avoid clobbering state.
        if args and isinstance(args[0], str):
            return
        super().__init__(**kwargs)

    @classmethod
    def from_axes(cls, axis_arrays, dims=None):
        """Build a structured-point array from three 1D axis arrays.

        Picks a ``vtkStructuredPointArray[dtype]`` instantiation matching
        the input axes' dtype, wraps the axes as VTK data arrays, and
        calls ``ConstructBackend`` to set up the lazy backend.  The
        returned array is an instance of ``VTKStructuredPointArray``
        (via the registered override) and has identity direction matrix.
        """
        from ..vtkCommonCore import vtkStructuredPointArray
        from ..vtkCommonDataModel import vtkStructuredData
        x, y, z = [numpy.ascontiguousarray(a) for a in axis_arrays]
        if dims is None:
            dims = (len(x), len(y), len(z))
        dtype_name = numpy.dtype(x.dtype).name
        arr = vtkStructuredPointArray[dtype_name]()
        xc = numpy_support.numpy_to_vtk(x)
        yc = numpy_support.numpy_to_vtk(y)
        zc = numpy_support.numpy_to_vtk(z)
        extent = [0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1]
        data_desc = vtkStructuredData.GetDataDescriptionFromExtent(extent)
        arr.ConstructBackend(xc, yc, zc, extent, data_desc)
        # As vtk::CreateStructuredPointArray does: without this the array
        # has one component and reports its shape as (n,) instead of (n, 3).
        arr.SetNumberOfComponents(3)
        arr.SetNumberOfTuples(vtkStructuredData.GetNumberOfPoints(extent))
        return arr

    # ---- axis helpers -------------------------------------------------------
    def _get_axis_arrays(self):
        """Get the X, Y, Z coordinate arrays directly from the backend.

        Returns a list of 3 numpy arrays [X, Y, Z], or None if the
        backend has not been constructed yet.

        The axes are converted to this array's dtype. The coordinate
        arrays can have a different one -- a rectilinear grid with float32
        coordinates gets a double point array -- and everything built from
        the axes should agree with ``dtype``.
        """
        xc = self.GetXCoordinates()
        if xc is None:
            return None
        return [
            numpy.asarray(numpy_support.vtk_to_numpy(coords), dtype=self.dtype)
            for coords in (xc, self.GetYCoordinates(), self.GetZCoordinates())
        ]

    def _get_dims(self):
        """Get grid dimensions from axis arrays."""
        if self.GetNumberOfTuples() == 0:
            return (0, 0, 0)
        axes = self._get_axis_arrays()
        if axes is not None:
            return tuple(len(a) for a in axes)
        return None

    def _uses_dir_matrix(self):
        """Check if a non-identity direction matrix is being used."""
        return self.GetUsesDirectionMatrix()

    # ---- core properties ----------------------------------------------------
    @property
    def dtype(self):
        return numpy.dtype(
            numpy_support.get_numpy_array_type(self.GetDataType()))

    @property
    def nbytes(self):
        return self.size * self.dtype.itemsize

    # ---- numpy protocol -----------------------------------------------------
    def _materialize(self, dtype=None):
        """Materialize the full array as a numpy ndarray.

        When axis arrays are available and no direction matrix is used,
        materializes via meshgrid (efficient).  Otherwise, uses DeepCopy
        to an AOS array.
        """
        if self.GetNumberOfTuples() == 0:
            return numpy.empty((0, 3), dtype=dtype or self.dtype)
        axes = self._get_axis_arrays()
        if axes is not None and not self._uses_dir_matrix():
            X, Y, Z = axes
            gx, gy, gz = numpy.meshgrid(X, Y, Z, indexing='ij')
            result = numpy.column_stack([
                gx.ravel(order='F'),
                gy.ravel(order='F'),
                gz.ravel(order='F')
            ])
        else:
            # DeepCopy to AOS, then convert via buffer protocol
            from ..vtkCommonCore import vtkAOSDataArrayTemplate
            aos = vtkAOSDataArrayTemplate[self.dtype.name]()
            aos.DeepCopy(self)
            result = numpy_support.vtk_to_numpy(aos)

        if dtype is not None:
            result = result.astype(dtype)
        return result

    def to_numpy(self, dtype=None):
        """Return the full (N, 3) array as a numpy ndarray."""
        return self._materialize(dtype)

    def __array__(self, dtype=None, copy=None):
        return self._materialize(dtype)

    def __buffer__(self, flags):
        return memoryview(self._materialize())

    def __getitem__(self, index):
        if isinstance(index, (int, numpy.integer)):
            n = self.GetNumberOfTuples()
            if index < 0:
                index += n
            if index < 0 or index >= n:
                raise IndexError(
                    f"index {index} is out of bounds for axis 0 with size {n}")
            axes = self._get_axis_arrays()
            if axes is not None and not self._uses_dir_matrix():
                dims = tuple(len(a) for a in axes)
                nx, ny, nz = dims
                i = index % nx
                j = (index // nx) % ny
                k = index // (nx * ny)
                return numpy.array(
                    [axes[0][i], axes[1][j], axes[2][k]], dtype=self.dtype)
            else:
                return numpy.array(self.GetTuple(index), dtype=self.dtype)

        # Column indexing: [:, col]
        if isinstance(index, tuple) and len(index) == 2:
            row_idx, col_idx = index
            if (isinstance(row_idx, slice) and row_idx == slice(None)
                    and isinstance(col_idx, (int, numpy.integer))
                    and col_idx in (0, 1, 2)):
                axes = self._get_axis_arrays()
                if axes is not None and not self._uses_dir_matrix():
                    dims = tuple(len(a) for a in axes)
                    return VTKStructuredAxisArray(
                        axes[col_idx], col_idx, dims)

        return self._materialize()[index]

    def __setitem__(self, key, value):
        raise TypeError("VTKStructuredPointArray is read-only")

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        if method != '__call__':
            return NotImplemented

        out = kwargs.get('out', None)
        if out is not None:
            return NotImplemented

        axes = self._get_axis_arrays()
        if axes is not None and not self._uses_dir_matrix():
            dims = tuple(len(a) for a in axes)

            # Unary ufunc — apply per-axis, stay lazy
            if (len(inputs) == 1
                    and isinstance(inputs[0], VTKStructuredPointArray)):
                new_axes = [ufunc(a) for a in axes]
                if _lazy_dtype_supported(new_axes):
                    return VTKStructuredPointArray.from_axes(new_axes, dims=dims)

            # Binary ufunc with scalar
            if len(inputs) == 2:
                self_input = None
                other_input = None
                for inp in inputs:
                    if isinstance(inp, VTKStructuredPointArray):
                        self_input = inp
                    else:
                        other_input = inp
                if self_input is not None and other_input is not None:
                    if numpy.isscalar(other_input) or (
                            isinstance(other_input, numpy.ndarray)
                            and other_input.ndim == 0):
                        self_axes = self_input._get_axis_arrays()
                        if self_axes is not None:
                            if inputs[0] is self_input:
                                new_axes = [ufunc(a, other_input)
                                            for a in self_axes]
                            else:
                                new_axes = [ufunc(other_input, a)
                                            for a in self_axes]
                            if _lazy_dtype_supported(new_axes):
                                return VTKStructuredPointArray.from_axes(
                                    new_axes, dims=dims)

        # Fall back to materialization
        materialized = [numpy.asarray(x)
                        if isinstance(x, VTKStructuredPointArray) else x
                        for x in inputs]
        result = ufunc(*materialized, **kwargs)
        return self._wrap_result(result)

    def __array_function__(self, func, types, args, kwargs):
        if func in _STRUCTURED_POINT_OVERRIDE:
            return _STRUCTURED_POINT_OVERRIDE[func](*args, **kwargs)
        new_args = []
        for a in args:
            if isinstance(a, VTKStructuredPointArray):
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
    def __neg__(self):              return numpy.negative(self)
    def __pos__(self):              return numpy.positive(self)
    def __abs__(self):              return numpy.absolute(self)

    # comparison
    def __lt__(self, other):  return numpy.less(self, other)
    def __le__(self, other):  return numpy.less_equal(self, other)
    def __eq__(self, other):  return numpy.equal(self, other)
    def __ne__(self, other):  return numpy.not_equal(self, other)
    def __ge__(self, other):  return numpy.greater_equal(self, other)
    def __gt__(self, other):  return numpy.greater(self, other)

    def __iter__(self):
        for i in range(self.GetNumberOfTuples()):
            yield self[i]

    def __repr__(self):
        dims = self._get_dims()
        return (f"VTKStructuredPointArray(shape={self.shape}, "
                f"dims={dims}, dtype={self.dtype})")

    def __str__(self):
        return repr(self)


# ---- Optimized reductions ---------------------------------------------------

@_override_structured_point_numpy(numpy.sum)
def _sp_sum(a, axis=None, **kwargs):
    if not isinstance(a, VTKStructuredPointArray):
        return numpy.sum(numpy.asarray(a), axis=axis, **kwargs)
    axes = a._get_axis_arrays()
    if axes is not None and not a._uses_dir_matrix():
        dims = tuple(len(ax) for ax in axes)
        nx, ny, nz = dims
        X, Y, Z = axes
        if axis is None:
            return (ny * nz * numpy.sum(X)
                    + nx * nz * numpy.sum(Y)
                    + nx * ny * numpy.sum(Z))
        elif axis == 0:
            return numpy.array([
                ny * nz * numpy.sum(X),
                nx * nz * numpy.sum(Y),
                nx * ny * numpy.sum(Z)
            ], dtype=numpy.float64)
    return numpy.sum(numpy.asarray(a), axis=axis, **kwargs)


@_override_structured_point_numpy(numpy.min)
def _sp_min(a, axis=None, **kwargs):
    if not isinstance(a, VTKStructuredPointArray):
        return numpy.min(numpy.asarray(a), axis=axis, **kwargs)
    axes = a._get_axis_arrays()
    if axes is not None and not a._uses_dir_matrix():
        X, Y, Z = axes
        if axis is None:
            return min(numpy.min(X), numpy.min(Y), numpy.min(Z))
        elif axis == 0:
            return numpy.array([numpy.min(X), numpy.min(Y), numpy.min(Z)])
    return numpy.min(numpy.asarray(a), axis=axis, **kwargs)


@_override_structured_point_numpy(numpy.max)
def _sp_max(a, axis=None, **kwargs):
    if not isinstance(a, VTKStructuredPointArray):
        return numpy.max(numpy.asarray(a), axis=axis, **kwargs)
    axes = a._get_axis_arrays()
    if axes is not None and not a._uses_dir_matrix():
        X, Y, Z = axes
        if axis is None:
            return max(numpy.max(X), numpy.max(Y), numpy.max(Z))
        elif axis == 0:
            return numpy.array([numpy.max(X), numpy.max(Y), numpy.max(Z)])
    return numpy.max(numpy.asarray(a), axis=axis, **kwargs)


@_override_structured_point_numpy(numpy.mean)
def _sp_mean(a, axis=None, **kwargs):
    if not isinstance(a, VTKStructuredPointArray):
        return numpy.mean(numpy.asarray(a), axis=axis, **kwargs)
    axes = a._get_axis_arrays()
    if axes is not None and not a._uses_dir_matrix():
        dims = tuple(len(ax) for ax in axes)
        nx, ny, nz = dims
        X, Y, Z = axes
        if axis is None:
            total = (ny * nz * numpy.sum(X)
                     + nx * nz * numpy.sum(Y)
                     + nx * ny * numpy.sum(Z))
            return total / (nx * ny * nz * 3)
        elif axis == 0:
            return numpy.array(
                [numpy.mean(X), numpy.mean(Y), numpy.mean(Z)])
    return numpy.mean(numpy.asarray(a), axis=axis, **kwargs)


# ---- Register overrides for all template instantiations ---------------------

# Mapping from vtkType names to IA64 ABI mangling characters.
# The Python template subscript converts 'float64' -> mangled char 'd' ->
# looks up 'vtkStructuredPointArray_IdE' in the template dict.  Templates
# that only have vtkType-based instantiations (e.g. vtkTypeFloat64) need
# aliases under the native-type mangled names.
_VTKTYPE_TO_MANGLING = {
    'vtkTypeInt8': 'a',
    'vtkTypeUInt8': 'h',
    'vtkTypeInt16': 's',
    'vtkTypeUInt16': 't',
    'vtkTypeInt32': 'i',
    'vtkTypeUInt32': 'j',
    'vtkTypeInt64': 'x',
    'vtkTypeUInt64': 'y',
    'vtkTypeFloat32': 'f',
    'vtkTypeFloat64': 'd',
}


def _add_template_type_aliases(template_cls, prefix):
    """Add native-type aliases so template['float64'] etc. work.

    Templates that only have vtkType-based instantiations (e.g.
    vtkTypeFloat64) need aliases under the native IA64 mangled names
    (e.g. _IdE for double) so that the Python template subscript
    notation template['float64'] can find them.
    """
    d = template_cls.__dict__
    for vtk_type_name, mangling_char in _VTKTYPE_TO_MANGLING.items():
        alias_key = f'{prefix}_I{mangling_char}E'
        if alias_key not in d:
            try:
                cls = template_cls[vtk_type_name]
                d[alias_key] = cls
            except (KeyError, TypeError):
                pass


def _register_structured_point_overrides():
    """Register VTKStructuredPointArray for all vtkStructuredPointArray
    template instantiations."""
    from vtkmodules.vtkCommonCore import vtkStructuredPointArray

    # Add native-type aliases so template['float64'] etc. work
    _add_template_type_aliases(vtkStructuredPointArray,
                               'vtkStructuredPointArray')

    # Register VTKStructuredPointArray for each dtype instantiation.  The
    # generated override class inherits from VTKStructuredPointArray and
    # shares its name, so isinstance(arr, VTKStructuredPointArray) holds
    # for every instance returned by VTK.
    for dt in ('float32', 'float64',
               'int8', 'int16', 'int32', 'int64',
               'uint8', 'uint16', 'uint32', 'uint64'):
        base = vtkStructuredPointArray[dt]
        cls = type('VTKStructuredPointArray',
                   (VTKStructuredPointArray, base),
                   {'__doc__': VTKStructuredPointArray.__doc__})
        base.override(cls)

_register_structured_point_overrides()

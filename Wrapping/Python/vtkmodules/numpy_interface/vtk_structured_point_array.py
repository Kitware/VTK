"""VTKStructuredPointArray — lazy numpy-compatible wrapper for structured points.

This module provides VTKStructuredPointArray and VTKStructuredAxisArray classes
that lazily represent the point coordinates of vtkImageData and vtkRectilinearGrid
datasets without materializing the full (N, 3) array.

For a grid with dimensions (nx, ny, nz), only O(nx + ny + nz) storage is needed
instead of O(nx * ny * nz * 3).  Ufuncs and scalar arithmetic operate per-axis
and stay lazy.  Reductions (sum, min, max, mean) use optimized O(nx+ny+nz)
formulas.

VTK uses Fortran ordering where i increases fastest:
    flat_idx = i + j * nx + k * nx * ny
"""

import numpy

from ..util import numpy_support
from ..vtkCommonCore import vtkWeakReference
from ._vtk_array_mixin import VTKDataArrayMixin


# Registry for __array_function__ overrides for the dataset-level wrapper
_STRUCTURED_POINT_OVERRIDE = {}

# Registry for __array_function__ overrides for the VTK mixin
_STRUCTURED_POINT_MIXIN_OVERRIDE = {}

def _override_mixin_numpy(numpy_function):
    """Register an __array_function__ override for VTKStructuredPointArrayMixin."""
    def decorator(func):
        _STRUCTURED_POINT_MIXIN_OVERRIDE[numpy_function] = func
        return func
    return decorator

def _override_structured_point_numpy(numpy_function):
    """Register an __array_function__ implementation for VTKStructuredPointArray."""
    def decorator(func):
        _STRUCTURED_POINT_OVERRIDE[numpy_function] = func
        return func
    return decorator


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


class VTKStructuredPointArray:
    """A lazy array wrapper for structured point arrays that stores three
    1D axis arrays and computes points without materializing the full (N, 3)
    grid.

    When the direction matrix is identity (common case for vtkImageData and
    all vtkRectilinearGrid), ufuncs and arithmetic operate per-axis and stay
    lazy.  When a non-identity direction matrix is used, operations fall back
    to full materialization.

    VTK uses Fortran ordering where i increases fastest:
        flat_idx = i + j * nx + k * nx * ny
    """

    def __init__(self, dataset=None, axis_arrays=None, dims=None,
                 uses_dir_matrix=False):
        self._dataset = None
        if dataset is not None:
            self._dataset = vtkWeakReference()
            self._dataset.Set(dataset)
        self._cached_axis_arrays = axis_arrays
        self._cached_dims = dims
        self._cached_uses_dir_matrix = (
            uses_dir_matrix if axis_arrays is not None else None)
        self.Association = None

    @classmethod
    def from_image_data(cls, image_data):
        """Create from vtkImageData.

        Computes axis arrays from origin, spacing, and dimensions.
        Detects non-identity direction matrices.
        """
        obj = cls(dataset=image_data)
        return obj

    @classmethod
    def from_rectilinear_grid(cls, rectilinear_grid):
        """Create from vtkRectilinearGrid.

        Uses the X/Y/Z coordinate arrays directly.
        RectilinearGrid never has a direction matrix.
        """
        obj = cls(dataset=rectilinear_grid)
        return obj

    @classmethod
    def _from_axes(cls, axis_arrays, dims=None, uses_dir_matrix=False):
        """Create from axis arrays for intermediate results (no VTK backing)."""
        return cls(axis_arrays=list(axis_arrays), dims=dims,
                   uses_dir_matrix=uses_dir_matrix)

    def _get_dataset(self):
        """Get the dataset from the weak reference."""
        if self._dataset is not None:
            return self._dataset.Get()
        return None

    @property
    def _axis_arrays(self):
        """Get axis arrays, computing from dataset if needed."""
        if self._cached_axis_arrays is not None:
            return self._cached_axis_arrays

        dataset = self._get_dataset()
        if dataset is None:
            return [numpy.array([]), numpy.array([]), numpy.array([])]

        from ..vtkCommonDataModel import vtkImageData, vtkRectilinearGrid

        if isinstance(dataset, vtkImageData):
            dims = [0, 0, 0]
            dataset.GetDimensions(dims)
            origin = dataset.GetOrigin()
            spacing = dataset.GetSpacing()
            return [
                numpy.array(origin[i] + spacing[i] * numpy.arange(dims[i]),
                            dtype=numpy.float64)
                for i in range(3)
            ]
        elif isinstance(dataset, vtkRectilinearGrid):
            return [
                numpy_support.vtk_to_numpy(dataset.GetXCoordinates()),
                numpy_support.vtk_to_numpy(dataset.GetYCoordinates()),
                numpy_support.vtk_to_numpy(dataset.GetZCoordinates()),
            ]
        return [numpy.array([]), numpy.array([]), numpy.array([])]

    @property
    def _dims(self):
        """Get dimensions from axis arrays or dataset."""
        if self._cached_dims is not None:
            return self._cached_dims
        return tuple(len(a) for a in self._axis_arrays)

    @property
    def _uses_dir_matrix(self):
        """Check if a non-identity direction matrix is used."""
        if self._cached_uses_dir_matrix is not None:
            return self._cached_uses_dir_matrix

        dataset = self._get_dataset()
        if dataset is None:
            return False

        from ..vtkCommonDataModel import vtkImageData
        if isinstance(dataset, vtkImageData):
            dm = dataset.GetDirectionMatrix()
            for i in range(3):
                for j in range(3):
                    expected = 1.0 if i == j else 0.0
                    if abs(dm.GetElement(i, j) - expected) > 1e-10:
                        return True
        return False

    @property
    def VTKObject(self):
        """Get the underlying VTK point data array from the dataset."""
        dataset = self._get_dataset()
        if dataset is not None:
            pts = dataset.GetPoints()
            if pts is not None:
                return pts.GetData()
        return None

    @property
    def shape(self):
        nx, ny, nz = self._dims
        return (nx * ny * nz, 3)

    @property
    def dtype(self):
        return self._axis_arrays[0].dtype

    @property
    def ndim(self):
        return 2

    @property
    def size(self):
        return self.shape[0] * 3

    def __len__(self):
        return self.shape[0]

    def __repr__(self):
        return (f"VTKStructuredPointArray(dims={self._dims}, "
                f"dtype={self.dtype}, uses_dir_matrix={self._uses_dir_matrix})")

    def _flat_to_ijk(self, flat_idx):
        """Convert a flat index to (i, j, k) structured indices.

        VTK uses Fortran ordering (i fastest):
            flat_idx = i + j * nx + k * nx * ny
        """
        nx, ny, nz = self._dims
        i = flat_idx % nx
        j = (flat_idx // nx) % ny
        k = flat_idx // (nx * ny)
        return i, j, k

    def __array__(self, dtype=None, **kwargs):
        """Materialize the full (N, 3) array."""
        if self._uses_dir_matrix and self.VTKObject is not None:
            result = numpy_support.vtk_to_numpy(self.VTKObject)
            if dtype is not None:
                result = result.astype(dtype)
            return result

        X, Y, Z = self._axis_arrays
        gx, gy, gz = numpy.meshgrid(X, Y, Z, indexing='ij')
        result = numpy.column_stack([
            gx.ravel(order='F'),
            gy.ravel(order='F'),
            gz.ravel(order='F')
        ])

        if dtype is not None:
            result = result.astype(dtype)
        return result

    def __getitem__(self, index):
        """Index into the structured point array."""
        if isinstance(index, (int, numpy.integer)):
            n = self.shape[0]
            if index < 0:
                index += n
            if index < 0 or index >= n:
                raise IndexError(
                    f"index {index} is out of bounds for axis 0 with size {n}")
            if not self._uses_dir_matrix:
                i, j, k = self._flat_to_ijk(index)
                return numpy.array([
                    self._axis_arrays[0][i],
                    self._axis_arrays[1][j],
                    self._axis_arrays[2][k]
                ], dtype=self.dtype)
            else:
                return numpy.asarray(self)[index]

        # Column indexing: [:, 0], [:, 1], [:, 2]
        if isinstance(index, tuple) and len(index) == 2:
            row_idx, col_idx = index
            if (isinstance(row_idx, slice) and row_idx == slice(None)
                    and isinstance(col_idx, (int, numpy.integer))
                    and col_idx in (0, 1, 2)
                    and not self._uses_dir_matrix):
                dataset = self._get_dataset()
                return VTKStructuredAxisArray(
                    self._axis_arrays[col_idx], col_idx,
                    self._dims, dataset)

        # Fall back to materialization
        return numpy.asarray(self)[index]

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        """Handle numpy ufuncs.  For identity-matrix arrays, apply per-axis
        and stay lazy."""
        if method != '__call__':
            return NotImplemented

        # Unary ufunc
        if (len(inputs) == 1 and not self._uses_dir_matrix
                and isinstance(inputs[0], VTKStructuredPointArray)):
            new_axes = [ufunc(a) for a in self._axis_arrays]
            return VTKStructuredPointArray._from_axes(
                new_axes, dims=self._dims)

        # Binary ufunc with scalar
        if len(inputs) == 2 and not self._uses_dir_matrix:
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
                    if inputs[0] is self_input:
                        new_axes = [ufunc(a, other_input)
                                    for a in self_input._axis_arrays]
                    else:
                        new_axes = [ufunc(other_input, a)
                                    for a in self_input._axis_arrays]
                    return VTKStructuredPointArray._from_axes(
                        new_axes, dims=self_input._dims)

        # Fall back to materialization
        materialized = [numpy.asarray(x)
                        if isinstance(x, VTKStructuredPointArray) else x
                        for x in inputs]
        return ufunc(*materialized, **kwargs)

    def __array_function__(self, func, types, args, kwargs):
        """Handle numpy functions with optimized paths."""
        if func in _STRUCTURED_POINT_OVERRIDE:
            return _STRUCTURED_POINT_OVERRIDE[func](*args, **kwargs)
        new_args = []
        for a in args:
            if isinstance(a, VTKStructuredPointArray):
                new_args.append(numpy.asarray(a))
            else:
                new_args.append(a)
        return func(*new_args, **kwargs)

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


# ---- Optimized reductions ---------------------------------------------------

@_override_structured_point_numpy(numpy.sum)
def _sp_sum(a, axis=None, **kwargs):
    if not isinstance(a, VTKStructuredPointArray) or a._uses_dir_matrix:
        return numpy.sum(numpy.asarray(a), axis=axis, **kwargs)
    nx, ny, nz = a._dims
    X, Y, Z = a._axis_arrays
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
    else:
        return numpy.sum(numpy.asarray(a), axis=axis, **kwargs)


@_override_structured_point_numpy(numpy.min)
def _sp_min(a, axis=None, **kwargs):
    if not isinstance(a, VTKStructuredPointArray) or a._uses_dir_matrix:
        return numpy.min(numpy.asarray(a), axis=axis, **kwargs)
    X, Y, Z = a._axis_arrays
    if axis is None:
        return min(numpy.min(X), numpy.min(Y), numpy.min(Z))
    elif axis == 0:
        return numpy.array([numpy.min(X), numpy.min(Y), numpy.min(Z)])
    else:
        return numpy.min(numpy.asarray(a), axis=axis, **kwargs)


@_override_structured_point_numpy(numpy.max)
def _sp_max(a, axis=None, **kwargs):
    if not isinstance(a, VTKStructuredPointArray) or a._uses_dir_matrix:
        return numpy.max(numpy.asarray(a), axis=axis, **kwargs)
    X, Y, Z = a._axis_arrays
    if axis is None:
        return max(numpy.max(X), numpy.max(Y), numpy.max(Z))
    elif axis == 0:
        return numpy.array([numpy.max(X), numpy.max(Y), numpy.max(Z)])
    else:
        return numpy.max(numpy.asarray(a), axis=axis, **kwargs)


@_override_structured_point_numpy(numpy.mean)
def _sp_mean(a, axis=None, **kwargs):
    if not isinstance(a, VTKStructuredPointArray) or a._uses_dir_matrix:
        return numpy.mean(numpy.asarray(a), axis=axis, **kwargs)
    nx, ny, nz = a._dims
    X, Y, Z = a._axis_arrays
    if axis is None:
        total = (ny * nz * numpy.sum(X)
                 + nx * nz * numpy.sum(Y)
                 + nx * ny * numpy.sum(Z))
        return total / (nx * ny * nz * 3)
    elif axis == 0:
        return numpy.array([numpy.mean(X), numpy.mean(Y), numpy.mean(Z)])
    else:
        return numpy.mean(numpy.asarray(a), axis=axis, **kwargs)


# ---- VTK class mixin for vtkStructuredPointArray ---------------------------

class VTKStructuredPointArrayMixin(VTKDataArrayMixin):
    """Numpy-compatible mixin for vtkStructuredPointArray template instances.

    This mixin is automatically applied to all ``vtkStructuredPointArray``
    instantiations (e.g. ``vtkStructuredPointTypeFloat64Array``).  It
    provides ``__array__``, ``__buffer__``, indexing, and basic numpy
    protocol so that structured point arrays returned by VTK are directly
    usable with numpy.

    The mixin retrieves axis coordinate arrays directly from the C++
    backend via ``GetXCoordinates()``, ``GetYCoordinates()``, and
    ``GetZCoordinates()``, enabling lazy per-axis operations and
    O(nx+ny+nz) reductions.  When no backend is available, it
    materializes via ``DeepCopy`` to an AOS array.
    """

    # ---- construction -------------------------------------------------------
    def __init__(self, *args, **kwargs):
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip mixin init to avoid clobbering state.
        if args and isinstance(args[0], str):
            return
        super().__init__(**kwargs)

    # ---- axis helpers -------------------------------------------------------
    def _get_axis_arrays(self):
        """Get the X, Y, Z coordinate arrays directly from the backend.

        Returns a list of 3 numpy arrays [X, Y, Z], or None if the
        backend has not been constructed yet.
        """
        xc = self.GetXCoordinates()
        if xc is None:
            return None
        return [
            numpy_support.vtk_to_numpy(xc),
            numpy_support.vtk_to_numpy(self.GetYCoordinates()),
            numpy_support.vtk_to_numpy(self.GetZCoordinates()),
        ]

    def _get_dims(self):
        """Get grid dimensions from axis arrays."""
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

            # Unary ufunc — apply per-axis, return dataset-level wrapper
            if (len(inputs) == 1
                    and isinstance(inputs[0], VTKStructuredPointArrayMixin)):
                new_axes = [ufunc(a) for a in axes]
                return VTKStructuredPointArray._from_axes(
                    new_axes, dims=dims)

            # Binary ufunc with scalar
            if len(inputs) == 2:
                self_input = None
                other_input = None
                for inp in inputs:
                    if isinstance(inp, VTKStructuredPointArrayMixin):
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
                            return VTKStructuredPointArray._from_axes(
                                new_axes, dims=dims)

        # Fall back to materialization
        materialized = [numpy.asarray(x)
                        if isinstance(x, VTKStructuredPointArrayMixin) else x
                        for x in inputs]
        result = ufunc(*materialized, **kwargs)
        return self._wrap_result(result)

    def __array_function__(self, func, types, args, kwargs):
        if func in _STRUCTURED_POINT_MIXIN_OVERRIDE:
            return _STRUCTURED_POINT_MIXIN_OVERRIDE[func](*args, **kwargs)
        new_args = []
        for a in args:
            if isinstance(a, VTKStructuredPointArrayMixin):
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


# ---- Mixin-level numpy function overrides -----------------------------------

@_override_mixin_numpy(numpy.sum)
def _mixin_sum(a, axis=None, **kwargs):
    if not isinstance(a, VTKStructuredPointArrayMixin):
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


@_override_mixin_numpy(numpy.min)
def _mixin_min(a, axis=None, **kwargs):
    if not isinstance(a, VTKStructuredPointArrayMixin):
        return numpy.min(numpy.asarray(a), axis=axis, **kwargs)
    axes = a._get_axis_arrays()
    if axes is not None and not a._uses_dir_matrix():
        X, Y, Z = axes
        if axis is None:
            return min(numpy.min(X), numpy.min(Y), numpy.min(Z))
        elif axis == 0:
            return numpy.array([numpy.min(X), numpy.min(Y), numpy.min(Z)])
    return numpy.min(numpy.asarray(a), axis=axis, **kwargs)


@_override_mixin_numpy(numpy.max)
def _mixin_max(a, axis=None, **kwargs):
    if not isinstance(a, VTKStructuredPointArrayMixin):
        return numpy.max(numpy.asarray(a), axis=axis, **kwargs)
    axes = a._get_axis_arrays()
    if axes is not None and not a._uses_dir_matrix():
        X, Y, Z = axes
        if axis is None:
            return max(numpy.max(X), numpy.max(Y), numpy.max(Z))
        elif axis == 0:
            return numpy.array([numpy.max(X), numpy.max(Y), numpy.max(Z)])
    return numpy.max(numpy.asarray(a), axis=axis, **kwargs)


@_override_mixin_numpy(numpy.mean)
def _mixin_mean(a, axis=None, **kwargs):
    if not isinstance(a, VTKStructuredPointArrayMixin):
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
    """Register VTKStructuredPointArrayMixin for all
    vtkStructuredPointArray template instantiations."""
    from vtkmodules.vtkCommonCore import vtkStructuredPointArray

    # Add native-type aliases so template['float64'] etc. work
    _add_template_type_aliases(vtkStructuredPointArray,
                               'vtkStructuredPointArray')

    # Register the mixin for each dtype instantiation
    for dt in ('float32', 'float64',
               'int8', 'int16', 'int32', 'int64',
               'uint8', 'uint16', 'uint32', 'uint64'):
        base = vtkStructuredPointArray[dt]
        cls = type('VTKStructuredPointArray',
                   (VTKStructuredPointArrayMixin, base),
                   {'__doc__': VTKStructuredPointArrayMixin.__doc__})
        base.override(cls)

_register_structured_point_overrides()

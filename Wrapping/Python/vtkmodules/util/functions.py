"""Algorithm module for the new VTK array types.

This module provides algorithms (reductions, VTK filter wrappers, vector/matrix
ops, cell quality metrics) that work with the new array mixin classes
(VTKAOSArray, VTKSOAArray, VTKAffineArray, VTKConstantArray, VTKPartitionedArray).

It is a self-contained reimplementation of the functionality in the three legacy
modules (internal_algorithms, numpy_algorithms, algorithms) from
``numpy_interface/``, using:

- ``VTKPartitionedArray``  instead of ``dsa.VTKCompositeDataArray``
- ``NoneArray``          from ``utils.py`` instead of ``dsa.NoneArray``
- Lowercase properties   (``.dataset``, ``.association``, ``.arrays``)
- No ``dsa.vtkDataArrayToVTKArray()`` wrapping — the override mechanism
  handles types automatically.

The old modules remain in place for backward compatibility with
``dataset_adapter``.
"""
import numpy
from typing import Union

from ..numpy_interface.utils import ArrayAssociation, NoneArray, reshape_append_ones
from ..numpy_interface.vtk_partitioned_array import VTKPartitionedArray

from vtkmodules.util import numpy_support
from vtkmodules.vtkCommonDataModel import vtkImageData

try:
    from ..vtkParallelCore import vtkMultiProcessController
    from ..vtkParallelMPI4Py import vtkMPI4PyCommunicator
except ImportError:
    vtkMultiProcessController = None
    vtkMPI4PyCommunicator = None

# ---------------------------------------------------------------------------
# Builtin aliases (shadow protection)
# ---------------------------------------------------------------------------
_builtin_min = min
_builtin_max = max
_builtin_sum = sum
_builtin_all = all

# ---------------------------------------------------------------------------
# Helper: wrap numpy result as VTK array
# ---------------------------------------------------------------------------

def _to_vtk_array(result, source):
    """Wrap a numpy result as a VTK array, copying metadata from *source*.

    If *result* is an ndarray whose first dimension matches *source*,
    creates a VTK-backed array with association/dataset propagated.
    Returns the result unchanged for scalars, 0-d arrays, or mismatched shapes.
    """
    if not isinstance(result, numpy.ndarray) or result.ndim == 0:
        return result
    if result.ndim > 2:
        return result  # VTK arrays can only be 1D or 2D
    src_n = source.shape[0] if hasattr(source, 'shape') else -1
    if result.shape[0] != src_n:
        return result
    vtk_input = result.view(numpy.int8) if result.dtype == numpy.bool_ else result
    vtk_arr = numpy_support.numpy_to_vtk(vtk_input)
    if hasattr(source, '_association'):
        vtk_arr._association = source._association
    if hasattr(source, '_set_dataset'):
        ds = source.dataset if hasattr(source, 'dataset') else None
        vtk_arr._set_dataset(ds)
    return vtk_arr

# ===========================================================================
# Section 3: Composite dispatch utilities
# ===========================================================================

def _apply_func2(func, array, args):
    """Apply *func* to each block of a VTKPartitionedArray. Returns a list."""
    if array is NoneArray:
        return []
    res = []
    for a in array.arrays:
        if a is NoneArray:
            res.append(NoneArray)
        else:
            res.append(func(a, *args))
    return res

def apply_ufunc(func, array, args=()):
    """Apply a unary function to each block of a VTKPartitionedArray.
    Plain arrays and NoneArray are also supported."""
    if array is NoneArray:
        return NoneArray
    elif isinstance(array, VTKPartitionedArray):
        return VTKPartitionedArray(
            _apply_func2(func, array, args), dataset=array.dataset)
    else:
        return func(array, *args)

def apply_dfunc(dfunc, array1, val2):
    """Apply a binary function across composite / plain arrays."""
    if isinstance(array1, VTKPartitionedArray) and isinstance(val2, VTKPartitionedArray):
        res = []
        for a1, a2 in zip(array1.arrays, val2.arrays):
            if a1 is NoneArray or a2 is NoneArray:
                res.append(NoneArray)
            else:
                l = reshape_append_ones(a1, a2)
                res.append(dfunc(l[0], l[1]))
        return VTKPartitionedArray(res, dataset=array1.dataset)
    elif isinstance(array1, VTKPartitionedArray):
        res = []
        for a in array1.arrays:
            if a is NoneArray:
                res.append(NoneArray)
            else:
                l = reshape_append_ones(a, val2)
                res.append(dfunc(l[0], l[1]))
        return VTKPartitionedArray(res, dataset=array1.dataset)
    elif array1 is NoneArray:
        return NoneArray
    elif isinstance(val2, numpy.ndarray):
        return dfunc(array1, val2)
    else:
        l = reshape_append_ones(array1, val2)
        return dfunc(l[0], l[1])

def _make_ufunc(ufunc):
    """Factory: wrap a unary function for composite dispatch."""
    def new_ufunc(array, *args):
        return apply_ufunc(ufunc, array, args)
    return new_ufunc

def _make_dfunc(dfunc):
    """Factory: wrap a binary function for composite dispatch."""
    def new_dfunc(array1, val2):
        return apply_dfunc(dfunc, array1, val2)
    return new_dfunc

def _make_dsfunc(dsfunc):
    """Factory: wrap a (array, dataset) function for composite dispatch."""
    from .data_model import CompositeDataSetBase
    def new_dsfunc(array, ds=None):
        if isinstance(array, VTKPartitionedArray):
            comp_ds = ds if ds is not None else array.dataset
            res = []
            if comp_ds is not None and isinstance(comp_ds, CompositeDataSetBase):
                for a, block_ds in zip(array.arrays, comp_ds):
                    if a is NoneArray:
                        res.append(NoneArray)
                    else:
                        res.append(dsfunc(a, block_ds))
            else:
                for a in array.arrays:
                    if a is NoneArray:
                        res.append(NoneArray)
                    else:
                        res.append(dsfunc(a, ds))
            return VTKPartitionedArray(res, dataset=comp_ds)
        elif array is NoneArray:
            return NoneArray
        else:
            return dsfunc(array, ds)
    return new_dsfunc

def _make_dsfunc2(dsfunc):
    """Factory: wrap a (dataset) function for composite dispatch."""
    from .data_model import CompositeDataSetBase
    def new_dsfunc2(ds):
        if isinstance(ds, CompositeDataSetBase):
            res = []
            for dataset in ds:
                res.append(dsfunc(dataset))
            return VTKPartitionedArray(res, dataset=ds)
        else:
            return dsfunc(ds)
    return new_dsfunc2

# ===========================================================================
# Section 4: MPI utilities
# ===========================================================================

def _lookup_mpi_type(ntype):
    from mpi4py import MPI
    if ntype == bool:
        typecode = 'b'
    else:
        typecode = numpy.dtype(ntype).char
    try:
        return MPI.__TypeDict__[typecode]
    except AttributeError:
        return MPI._typedict[typecode]

def _reduce_dims(array, comm):
    from mpi4py import MPI
    dims = numpy.array([0, 0], dtype=numpy.int32)
    if array is not NoneArray:
        shp = numpy.shape(array)
        if len(shp) == 0:
            dims = numpy.array([1, 0], dtype=numpy.int32)
        elif len(shp) == 1:
            dims = numpy.array([shp[0], 0], dtype=numpy.int32)
        else:
            dims = numpy.array(shp, dtype=numpy.int32)
    max_dims = numpy.array(dims, dtype=numpy.int32)
    mpitype = _lookup_mpi_type(numpy.int32)
    comm.Allreduce([dims, mpitype], [max_dims, mpitype], MPI.MAX)

    if max_dims[1] == 0:
        max_dims = numpy.array((max_dims[0],))
        size = max_dims[0]
    else:
        size = max_dims[0] * max_dims[1]

    if max_dims[0] == 1:
        max_dims = 1

    return (max_dims, size)

def global_func(impl, array, axis, controller):
    """Reduction driver with MPI parallel support."""
    if isinstance(array, VTKPartitionedArray):
        if axis is None or axis == 0:
            res = impl.serial_composite(array, axis)
        else:
            res = apply_ufunc(impl.op(), array, (axis,))
    else:
        res = impl.op()(array, axis)
        if res is not NoneArray:
            res = res.astype(numpy.float64)

    if axis is None or axis == 0:
        if controller is None and vtkMultiProcessController is not None:
            controller = vtkMultiProcessController.GetGlobalController()
        if controller and controller.IsA("vtkMPIController") and controller.GetNumberOfProcesses() > 1:
            try:
                from mpi4py import MPI
            except ImportError:
                raise RuntimeError('MPI4Py is required to perform multi-rank operations')
            comm = vtkMPI4PyCommunicator.ConvertToPython(controller.GetCommunicator())

            max_dims, size = _reduce_dims(res, comm)

            if size == 0:
                return NoneArray

            if res is NoneArray:
                if numpy.isscalar(max_dims):
                    max_dims = ()
                res = numpy.empty(max_dims)
                res.fill(impl.default())

            res_recv = numpy.array(res)
            mpi_type = _lookup_mpi_type(res.dtype)
            comm.Allreduce([res, mpi_type], [res_recv, mpi_type], impl.mpi_op())
            if array is NoneArray:
                return NoneArray
            res = res_recv

    return res

def _local_array_count(array, axis):
    if array is NoneArray:
        return numpy.int64(0)
    elif axis is None:
        return numpy.int64(array.size)
    else:
        return numpy.int64(numpy.shape(array)[0])

def array_count(array, axis, controller):
    """Count elements in a VTK or NumPy array, with MPI support."""
    if array is NoneArray:
        size = numpy.int64(0)
    elif axis is None:
        size = numpy.int64(array.size)
    else:
        size = numpy.int64(numpy.shape(array)[0])

    if controller is None and vtkMultiProcessController is not None:
        controller = vtkMultiProcessController.GetGlobalController()

    if controller and controller.IsA("vtkMPIController"):
        from mpi4py import MPI
        comm = vtkMPI4PyCommunicator.ConvertToPython(controller.GetCommunicator())
        total_size = numpy.array(size, dtype=numpy.int64)
        mpitype = _lookup_mpi_type(numpy.int64)
        comm.Allreduce([size, mpitype], [total_size, mpitype], MPI.SUM)
        size = total_size

    return size

# ===========================================================================
# Section 5: Private VTK filter helpers
# ===========================================================================

def _cell_derivatives(array, dataset, attribute_type, filter):
    from vtkmodules.vtkFiltersCore import vtkCellDataToPointData

    if not dataset:
        raise RuntimeError('Need a dataset to compute _cell_derivatives.')

    # Save association before numpy conversion strips it
    assoc = array._association

    # Convert to contiguous numpy for VTK filter input
    arr = numpy.asarray(array)
    if arr.ndim == 1:
        arr = arr.reshape((arr.shape[0], 1))

    ncomp = arr.shape[1]
    if attribute_type == 'scalars' and ncomp != 1:
        raise RuntimeError('This function expects scalars. '
                           'Input shape ' + str(arr.shape))
    if attribute_type == 'vectors' and ncomp != 3:
        raise RuntimeError('This function expects vectors. '
                           'Input shape ' + str(arr.shape))

    if not arr.flags.contiguous:
        arr = arr.copy()
    varray = numpy_support.numpy_to_vtk(arr)

    if attribute_type == 'scalars':
        varray.SetName('scalars')
    else:
        varray.SetName('vectors')

    ds = dataset.NewInstance()
    ds.UnRegister(None)
    ds.CopyStructure(dataset)

    if ArrayAssociation.FIELD == assoc:
        raise RuntimeError('Unknown data association. Data should be associated with points or cells.')

    if ArrayAssociation.POINT == assoc:
        if arr.shape[0] != dataset.GetNumberOfPoints():
            raise RuntimeError('The number of points does not match the number of tuples in the array')
        if attribute_type == 'scalars':
            ds.GetPointData().SetScalars(varray)
        else:
            ds.GetPointData().SetVectors(varray)
    elif ArrayAssociation.CELL == assoc:
        if arr.shape[0] != dataset.GetNumberOfCells():
            raise RuntimeError('The number of does not match the number of tuples in the array')

        ds2 = dataset.NewInstance()
        ds2.UnRegister(None)
        ds2.CopyStructure(dataset)

        if attribute_type == 'scalars':
            ds2.GetCellData().SetScalars(varray)
        else:
            ds2.GetCellData().SetVectors(varray)

        c2p = vtkCellDataToPointData()
        c2p.SetInputData(ds2)
        c2p.Update()

        if attribute_type == 'scalars':
            ds.GetPointData().SetScalars(c2p.GetOutput().GetPointData().GetScalars())
        else:
            ds.GetPointData().SetVectors(c2p.GetOutput().GetPointData().GetVectors())

    filter.SetInputData(ds)

    if ArrayAssociation.POINT == assoc:
        c2p = vtkCellDataToPointData()
        c2p.SetInputConnection(filter.GetOutputPort())
        c2p.Update()
        return c2p.GetOutput().GetPointData()
    elif ArrayAssociation.CELL == assoc:
        filter.Update()
        return filter.GetOutput().GetCellData()
    else:
        raise RuntimeError('Unknown data association. Data should be associated with points or cells.')

def _cell_quality(dataset, quality):
    from vtkmodules.vtkFiltersVerdict import vtkCellQuality

    if not dataset:
        raise RuntimeError('Need a dataset to compute _cell_quality')

    ds = dataset.NewInstance()
    ds.UnRegister(None)
    ds.CopyStructure(dataset)

    filter = vtkCellQuality()
    filter.SetInputData(ds)

    if   "area"         == quality: filter.SetQualityMeasureToArea()
    elif "aspect"       == quality: filter.SetQualityMeasureToAspectRatio()
    elif "aspect_gamma" == quality: filter.SetQualityMeasureToAspectGamma()
    elif "condition"    == quality: filter.SetQualityMeasureToCondition()
    elif "diagonal"     == quality: filter.SetQualityMeasureToDiagonal()
    elif "jacobian"     == quality: filter.SetQualityMeasureToJacobian()
    elif "max_angle"    == quality: filter.SetQualityMeasureToMaxAngle()
    elif "shear"        == quality: filter.SetQualityMeasureToShear()
    elif "skew"         == quality: filter.SetQualityMeasureToSkew()
    elif "min_angle"    == quality: filter.SetQualityMeasureToMinAngle()
    elif "volume"       == quality: filter.SetQualityMeasureToVolume()
    else: raise RuntimeError('Unknown cell quality [' + quality + '].')

    filter.Update()

    ans = filter.GetOutput().GetCellData().GetArray("CellQuality")
    ans._association = ArrayAssociation.CELL

    return ans

def _matrix_math_filter(array, operation):
    from vtkmodules.vtkFiltersVerdict import vtkMatrixMathFilter

    if operation not in ['Determinant', 'Inverse', 'Eigenvalue', 'Eigenvector']:
        raise RuntimeError('Unknown quality measure [' + operation + ']'
                           ' Supported are [Determinant, Inverse, Eigenvalue, Eigenvector]')

    if array.ndim != 3:
        raise RuntimeError(operation + ' only works for an array of matrices(3D array).'
                           ' Input shape ' + str(array.shape))
    elif array.shape[1] != array.shape[2]:
        raise RuntimeError(operation + ' requires an array of 2D square matrices.'
                           ' Input shape ' + str(array.shape))

    assoc = getattr(array, '_association', None)
    ds_ref = getattr(array, 'dataset', None)

    arr = numpy.asarray(array)
    if not arr.flags.contiguous:
        arr = arr.copy()

    nrows = arr.shape[0]
    ncols = arr.shape[1] * arr.shape[2]
    arr = arr.reshape(nrows, ncols)

    ds = vtkImageData()
    ds.SetDimensions(nrows, 1, 1)

    varray = numpy_support.numpy_to_vtk(arr)
    varray.SetName('tensors')
    ds.GetPointData().SetTensors(varray)

    filter = vtkMatrixMathFilter()

    if   operation == 'Determinant':  filter.SetOperationToDeterminant()
    elif operation == 'Inverse':      filter.SetOperationToInverse()
    elif operation == 'Eigenvalue':   filter.SetOperationToEigenvalue()
    elif operation == 'Eigenvector':  filter.SetOperationToEigenvector()

    filter.SetInputData(ds)
    filter.Update()

    ans = filter.GetOutput().GetPointData().GetArray(operation)

    if assoc is not None:
        ans._association = assoc
    if ds_ref is not None:
        ans._set_dataset(ds_ref)

    return ans

# ===========================================================================
# Section 6: Private single-array implementations
# ===========================================================================

def _sum(array, axis=None):
    if array is NoneArray:
        return NoneArray
    return numpy.sum(array, axis)

def _max(array, axis=None):
    if array is NoneArray:
        return NoneArray
    return numpy.max(array, axis)

def _min(array, axis=None):
    if array is NoneArray:
        return NoneArray
    return numpy.min(array, axis)

def _all(array, axis=None):
    if array is NoneArray:
        return NoneArray
    return numpy.all(numpy.array(array), axis)

def _mean(array, axis=None):
    if array is NoneArray:
        return NoneArray
    return numpy.mean(numpy.array(array), axis)

def _var(array, axis=None):
    if array is NoneArray:
        return NoneArray
    return numpy.var(array, axis)

def _cross(x, y):
    if x is NoneArray or y is NoneArray:
        return NoneArray

    if x.ndim != y.ndim or x.shape != y.shape:
        raise RuntimeError('Both operands must have same dimension and shape.'
                           ' Input shapes ' + str(x.shape) + ' and ' + str(y.shape))

    if x.ndim != 1 and x.ndim != 2:
        raise RuntimeError('Cross only works for 3D vectors or an array of 3D vectors.'
                           ' Input shapes ' + str(x.shape) + ' and ' + str(y.shape))

    if x.ndim == 1 and x.shape[0] != 3:
        raise RuntimeError('Cross only works for 3D vectors.'
                           ' Input shapes ' + str(x.shape) + ' and ' + str(y.shape))

    if x.ndim == 2 and x.shape[1] != 3:
        raise RuntimeError('Cross only works for an array of 3D vectors.'
                           'Input shapes ' + str(x.shape) + ' and ' + str(y.shape))

    return _to_vtk_array(numpy.cross(numpy.asarray(x), numpy.asarray(y)), x)

def _dot(a1, a2):
    if a1 is NoneArray or a2 is NoneArray:
        return NoneArray

    if a1.shape[1] != a2.shape[1]:
        raise RuntimeError('Dot product only works with vectors of same dimension.'
                           ' Input shapes ' + str(a1.shape) + ' and ' + str(a2.shape))
    m = numpy.asarray(a1) * numpy.asarray(a2)
    va = numpy.add.reduce(m, 1)
    return _to_vtk_array(va, a1)

def _mag(a):
    d = _dot(a, a)
    if d is NoneArray:
        return NoneArray
    result = numpy.sqrt(numpy.asarray(d))
    return _to_vtk_array(result, a)

def _matmul(a, b):
    ashape = a.shape
    if (len(ashape) == 3 and (ashape[1] != 3 or ashape[2] not in [1, 3])) \
       or (len(ashape) == 2 and ashape[1] != 3) \
       or (len(ashape) != 2 and len(ashape) != 3):
        return NoneArray

    bshape = b.shape
    if (len(bshape) == 3 and (bshape[1] != 3 or bshape[2] not in [1, 3])) \
       or (len(bshape) == 2 and bshape[1] != 3) \
       or (len(bshape) != 2 and len(bshape) != 3):
        return NoneArray

    aindices = "...j"
    if len(ashape) == 3:
        aindices = "...ij"

    bindices = "...j"
    if len(bshape) == 3:
        bindices = "...jk"

    indices = aindices + ',' + bindices
    result = numpy.einsum(indices, numpy.asarray(a), numpy.asarray(b))
    return _to_vtk_array(result, a)

def _norm(a):
    m = numpy.asarray(_mag(a)).reshape((a.shape[0], 1))
    return _to_vtk_array(numpy.asarray(a) / m, a)

def _det(array):
    return _matrix_math_filter(array, "Determinant")

def _eigenvalue(array):
    return _matrix_math_filter(array, "Eigenvalue")

def _eigenvector(array):
    return _matrix_math_filter(array, "Eigenvector")

def _inv(array):
    return _matrix_math_filter(array, "Inverse")

def _trace(array):
    ax1 = 0
    ax2 = 1
    if array.ndim > 2:
        ax1 = 1
        ax2 = 2
    result = numpy.trace(numpy.asarray(array), axis1=ax1, axis2=ax2)
    return _to_vtk_array(result, array)

def _curl(array, dataset=None):
    from vtkmodules.vtkFiltersGeneral import vtkCellDerivatives

    if not dataset:
        dataset = array.dataset
    if not dataset:
        raise RuntimeError('Need a dataset to compute curl.')

    if array.ndim != 2 or array.shape[1] != 3:
        raise RuntimeError('Curl only works with an array of 3D vectors.'
                           'Input shape ' + str(array.shape))

    cd = vtkCellDerivatives()
    cd.SetVectorModeToComputeVorticity()

    res = _cell_derivatives(array, dataset, 'vectors', cd)

    retVal = res.GetVectors()
    retVal.SetName("vorticity")

    retVal._association = array._association
    retVal._set_dataset(dataset)

    return retVal

def _gradient(array, dataset=None):
    from vtkmodules.vtkFiltersGeneral import vtkCellDerivatives

    if not dataset:
        dataset = array.dataset
    if not dataset:
        raise RuntimeError('Need a dataset to compute gradient')

    try:
        ncomp = array.shape[1]
    except IndexError:
        ncomp = 1
    if ncomp != 1 and ncomp != 3:
        raise RuntimeError('Gradient only works with scalars (1 component) and vectors (3 component)'
                           ' Input shape ' + str(array.shape))

    cd = vtkCellDerivatives()
    if ncomp == 1:
        attribute_type = 'scalars'
    else:
        attribute_type = 'vectors'

    res = _cell_derivatives(array, dataset, attribute_type, cd)

    if ncomp == 1:
        retVal = res.GetVectors()
    else:
        retVal = res.GetTensors()

    try:
        if array.GetName():
            retVal.SetName("gradient of " + array.GetName())
        else:
            retVal.SetName("gradient")
    except AttributeError:
        retVal.SetName("gradient")

    retVal._association = array._association
    retVal._set_dataset(dataset)

    return retVal

def _divergence(array, dataset=None):
    if not dataset:
        dataset = array.dataset
    if not dataset:
        raise RuntimeError('Need a dataset to compute divergence')

    if array.ndim != 2 or array.shape[1] != 3:
        raise RuntimeError('Divergence only works with an array of 3D vectors.'
                           ' Input shape ' + str(array.shape))

    g = _gradient(array, dataset)
    g_np = numpy.asarray(g).reshape(g.shape[0], 3, 3)

    result = numpy.add.reduce(g_np.diagonal(axis1=1, axis2=2), 1)
    return _to_vtk_array(result, array)

def _laplacian(array, dataset=None):
    if not dataset:
        dataset = array.dataset
    if not dataset:
        raise RuntimeError('Need a dataset to compute laplacian')
    ans = _gradient(array, dataset)
    return _divergence(ans, dataset)

def _strain(array, dataset=None):
    from vtkmodules.vtkFiltersGeneral import vtkCellDerivatives

    if not dataset:
        dataset = array.dataset
    if not dataset:
        raise RuntimeError('Need a dataset to compute strain')

    if 2 != array.ndim or 3 != array.shape[1]:
        raise RuntimeError('strain only works with an array of 3D vectors'
                           'Input shape ' + str(array.shape))

    cd = vtkCellDerivatives()
    cd.SetTensorModeToComputeStrain()

    res = _cell_derivatives(array, dataset, 'vectors', cd)

    retVal = res.GetTensors()
    retVal.SetName("strain")

    retVal._association = array._association
    retVal._set_dataset(dataset)

    return retVal

_vorticity = _curl

def _area(dataset):
    return _cell_quality(dataset, "area")

def _aspect(dataset):
    return _cell_quality(dataset, "aspect")

def _aspect_gamma(dataset):
    return _cell_quality(dataset, "aspect_gamma")

def _condition(dataset):
    return _cell_quality(dataset, "condition")

def _diagonal(dataset):
    return _cell_quality(dataset, "diagonal")

def _jacobian(dataset):
    return _cell_quality(dataset, "jacobian")

def _max_angle(dataset):
    return _cell_quality(dataset, "max_angle")

def _min_angle(dataset):
    return _cell_quality(dataset, "min_angle")

def _shear(dataset):
    return _cell_quality(dataset, "shear")

def _skew(dataset):
    return _cell_quality(dataset, "skew")

def _volume(dataset):
    from vtkmodules.vtkFiltersVerdict import vtkCellSizeFilter

    if not dataset:
        raise RuntimeError('Need a dataset to compute volume')

    ds = dataset.NewInstance()
    ds.UnRegister(None)
    ds.CopyStructure(dataset)

    filter = vtkCellSizeFilter()
    filter.SetInputData(ds)
    filter.ComputeVertexCountOff()
    filter.ComputeLengthOff()
    filter.ComputeAreaOff()
    filter.Update()

    ans = filter.GetOutput().GetCellData().GetArray("Volume")
    ans.SetName("CellQuality")
    ans._association = ArrayAssociation.CELL

    return ans

def _surface_normal(dataset):
    from vtkmodules.vtkFiltersCore import vtkPolyDataNormals

    if not dataset:
        raise RuntimeError('Need a dataset to compute surface_normal')

    ds = dataset.NewInstance()
    ds.UnRegister(None)
    ds.CopyStructure(dataset)

    filter = vtkPolyDataNormals()
    filter.SetInputData(ds)
    filter.ComputeCellNormalsOn()
    filter.ComputePointNormalsOff()
    filter.SetFeatureAngle(180)
    filter.SplittingOff()
    filter.ConsistencyOff()
    filter.AutoOrientNormalsOff()
    filter.FlipNormalsOff()
    filter.NonManifoldTraversalOff()
    filter.Update()

    ans = filter.GetOutput().GetCellData().GetNormals()
    ans._association = ArrayAssociation.CELL

    return ans

def _vertex_normal(dataset):
    from vtkmodules.vtkFiltersCore import vtkPolyDataNormals

    if not dataset:
        raise RuntimeError('Need a dataset to compute vertex_normal')

    ds = dataset.NewInstance()
    ds.UnRegister(None)
    ds.CopyStructure(dataset)

    filter = vtkPolyDataNormals()
    filter.SetInputData(ds)
    filter.ComputeCellNormalsOff()
    filter.ComputePointNormalsOn()
    filter.SetFeatureAngle(180)
    filter.SplittingOff()
    filter.ConsistencyOff()
    filter.AutoOrientNormalsOff()
    filter.FlipNormalsOff()
    filter.NonManifoldTraversalOff()
    filter.Update()

    ans = filter.GetOutput().GetPointData().GetNormals()
    ans._association = ArrayAssociation.POINT

    return ans

def _make_vector(ax, ay, az=None):
    if ax is NoneArray or ay is NoneArray:
        return NoneArray

    if len(ax.shape) != 1 or len(ay.shape) != 1 or (az is not None and len(az.shape) != 1):
        raise ValueError("Can only merge 1D arrays")

    if az is None:
        az = numpy.zeros(ax.shape)
    v = numpy.vstack([numpy.asarray(ax), numpy.asarray(ay), numpy.asarray(az)]).transpose().copy()
    vtk_arr = numpy_support.numpy_to_vtk(v)
    if hasattr(ax, '_association'):
        vtk_arr._association = ax._association
    if hasattr(ax, '_set_dataset'):
        ds = ax.dataset if hasattr(ax, 'dataset') else None
        vtk_arr._set_dataset(ds)
    return vtk_arr

# ===========================================================================
# Section 7: Public composite-aware reductions
# ===========================================================================

def sum(array, axis=None, controller=None):
    """Composite + MPI aware sum."""
    class SumImpl:
        def op(self):
            return _sum

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.SUM

        def serial_composite(self, array, axis):
            res = None
            for a in array.arrays:
                if a is not NoneArray:
                    if res is None:
                        res = _sum(a, axis).astype(numpy.float64)
                    else:
                        res += _sum(a, axis)
            return res

        def default(self):
            return numpy.float64(0)

    return global_func(SumImpl(), array, axis, controller)

def max(array, axis=None, controller=None):
    """Composite + MPI aware max."""
    class MaxImpl:
        def op(self):
            return _max

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.MAX

        def serial_composite(self, array, axis):
            res = _apply_func2(_max, array, (axis,))
            clean_list = [a for a in res if a is not NoneArray]
            if not clean_list:
                return None
            return _max(clean_list, axis=0).astype(numpy.float64)

        def default(self):
            return numpy.finfo(numpy.float64).min

    return global_func(MaxImpl(), array, axis, controller)

def min(array, axis=None, controller=None):
    """Composite + MPI aware min."""
    class MinImpl:
        def op(self):
            return _min

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.MIN

        def serial_composite(self, array, axis):
            res = _apply_func2(_min, array, (axis,))
            clean_list = [a for a in res if a is not NoneArray]
            if not clean_list:
                return None
            return _min(clean_list, axis=0).astype(numpy.float64)

        def default(self):
            return numpy.finfo(numpy.float64).max

    return global_func(MinImpl(), array, axis, controller)

def all(array, axis=None, controller=None):
    """Composite + MPI aware all."""
    class AllImpl:
        def op(self):
            return _all

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.LAND

        def serial_composite(self, array, axis):
            res = _apply_func2(_all, array, (axis,))
            clean_list = [a for a in res if a is not NoneArray]
            if not clean_list:
                return None
            return _all(clean_list, axis=0)

        def default(self, max_comps=None):
            if max_comps is not None:
                return numpy.ones(max_comps, dtype=bool)
            return numpy.ones(1, dtype=bool)

    return global_func(AllImpl(), array, axis, controller)

def mean(array, axis=None, controller=None, size=None):
    """Composite + MPI aware mean."""
    if axis is None or axis == 0:
        if size is None:
            size = array_count(array, axis, controller)
        return sum(array, axis, controller) / size
    else:
        if isinstance(array, VTKPartitionedArray):
            return apply_ufunc(_mean, array, (axis,))
        else:
            return _mean(array, axis)

def var(array, axis=None, controller=None):
    """Composite + MPI aware variance."""
    if axis is None or axis == 0:
        size = array_count(array, axis, controller)
        tmp = array - mean(array, axis, controller, size)
        return sum(tmp * tmp, axis, controller) / size
    else:
        if isinstance(array, VTKPartitionedArray):
            return apply_ufunc(_var, array, (axis,))
        else:
            return _var(array, axis)

def std(array, axis=None, controller=None):
    """Composite + MPI aware standard deviation."""
    return numpy.sqrt(var(array, axis, controller))

# ===========================================================================
# Section 8: Public composite-aware VTK algorithms
# ===========================================================================

curl = _make_dsfunc(_curl)
gradient = _make_dsfunc(_gradient)
divergence = _make_dsfunc(_divergence)
laplacian = _make_dsfunc(_laplacian)
strain = _make_dsfunc(_strain)
vorticity = _make_dsfunc(_vorticity)

area = _make_dsfunc2(_area)
aspect = _make_dsfunc2(_aspect)
aspect_gamma = _make_dsfunc2(_aspect_gamma)
condition = _make_dsfunc2(_condition)
diagonal = _make_dsfunc2(_diagonal)
jacobian = _make_dsfunc2(_jacobian)
max_angle = _make_dsfunc2(_max_angle)
min_angle = _make_dsfunc2(_min_angle)
shear = _make_dsfunc2(_shear)
skew = _make_dsfunc2(_skew)
volume = _make_dsfunc2(_volume)

det = _make_ufunc(_det)
determinant = det
eigenvalue = _make_ufunc(_eigenvalue)
eigenvector = _make_ufunc(_eigenvector)
inv = _make_ufunc(_inv)
inverse = inv
trace = _make_ufunc(_trace)

cross = _make_dfunc(_cross)
dot = _make_dfunc(_dot)
matmul = _make_dfunc(_matmul)

mag = _make_ufunc(_mag)
norm = _make_ufunc(_norm)

surface_normal = _make_dsfunc2(_surface_normal)
vertex_normal = _make_dsfunc2(_vertex_normal)

# ===========================================================================
# Section 9: Public utility functions
# ===========================================================================

def make_vector(arrayx, arrayy, arrayz=None):
    """Composite-aware vector construction from 2 or 3 scalar arrays."""
    if isinstance(arrayx, VTKPartitionedArray) and isinstance(arrayy, VTKPartitionedArray) and \
       (isinstance(arrayz, VTKPartitionedArray) or arrayz is None):
        res = []
        if arrayz is None:
            for ax, ay in zip(arrayx.arrays, arrayy.arrays):
                if ax is not NoneArray and ay is not NoneArray:
                    res.append(_make_vector(ax, ay))
                else:
                    res.append(NoneArray)
        else:
            for ax, ay, az in zip(arrayx.arrays, arrayy.arrays, arrayz.arrays):
                if ax is not NoneArray and ay is not NoneArray and az is not NoneArray:
                    res.append(_make_vector(ax, ay, az))
                else:
                    res.append(NoneArray)
        return VTKPartitionedArray(res, dataset=arrayx.dataset)
    else:
        return _make_vector(arrayx, arrayy, arrayz)

def make_mask_from_NaNs(array, ghost_array=NoneArray, is_cell=False):
    """Create a ghost array from NaN values."""
    from ..vtkCommonDataModel import vtkDataSetAttributes
    if is_cell:
        mask_value = vtkDataSetAttributes.HIDDENCELL
    else:
        mask_value = vtkDataSetAttributes.HIDDENPOINT

    isnan = _make_ufunc(numpy.isnan)
    mask = isnan(array).astype(numpy.uint8) * mask_value
    if ghost_array is NoneArray:
        return mask
    return apply_dfunc(numpy.bitwise_or, mask, ghost_array)

def make_point_mask_from_NaNs(dataset, array):
    """Create a ghost array for point data with NaN values."""
    from ..vtkCommonDataModel import vtkDataSetAttributes
    ghosts = dataset.point_data[vtkDataSetAttributes.GhostArrayName()]
    return make_mask_from_NaNs(array, ghosts)

def make_cell_mask_from_NaNs(dataset, array):
    """Create a ghost array for cell data with NaN values."""
    from ..vtkCommonDataModel import vtkDataSetAttributes
    ghosts = dataset.cell_data[vtkDataSetAttributes.GhostArrayName()]
    return make_mask_from_NaNs(array, ghosts, True)

# ===========================================================================
# Section 10: Per-block operations
# ===========================================================================

def _global_per_block(impl, array, axis=None, controller=None):
    if axis is not None and axis > 0:
        return impl.op()(array, axis=axis, controller=controller)
    try:
        dataset = array.dataset
    except AttributeError:
        dataset = None
    if not isinstance(array, VTKPartitionedArray) and (isinstance(array, numpy.ndarray) or hasattr(array, '__array__')):
        from ..vtkCommonDataModel import vtkMultiBlockDataSet
        array = VTKPartitionedArray([array])
        ds = vtkMultiBlockDataSet()
        ds.SetBlock(0, dataset)
        dataset = ds
    results = _apply_func2(impl.op2(), array, (axis,))
    if controller is None and vtkMultiProcessController is not None:
        controller = vtkMultiProcessController.GetGlobalController()
    if controller and controller.IsA("vtkMPIController"):
        from mpi4py import MPI
        comm = vtkMPI4PyCommunicator.ConvertToPython(controller.GetCommunicator())

        res = NoneArray
        for res in results:
            if res is not NoneArray:
                break

        max_dims, size = _reduce_dims(res, comm)

        if size == 0:
            return NoneArray

        ids = []
        lmax_id = numpy.int32(0)
        if dataset is not None:
            it = dataset.NewIterator()
            it.UnRegister(None)
            it.SetSkipEmptyNodes(False)
            while not it.IsDoneWithTraversal():
                _id = it.GetCurrentFlatIndex()
                lmax_id = numpy.max((lmax_id, _id)).astype(numpy.int32)
                if it.GetCurrentDataObject() is not None:
                    ids.append(_id)
                it.GoToNextItem()
        max_id = numpy.array(0, dtype=numpy.int32)
        mpitype = _lookup_mpi_type(numpy.int32)
        comm.Allreduce([lmax_id, mpitype], [max_id, mpitype], MPI.MAX)

        has_ids = numpy.zeros(max_id + 1, dtype=numpy.int32)
        for _id in ids:
            has_ids[_id] = 1
        id_count = numpy.array(has_ids)
        comm.Allreduce([has_ids, mpitype], [id_count, mpitype], MPI.SUM)

        if numpy.all(id_count <= 1):
            return VTKPartitionedArray(results, dataset=dataset)

        reduce_ids = []
        for _id in range(len(id_count)):
            if id_count[_id] > 1:
                reduce_ids.append(_id)

        to_reduce = len(reduce_ids)
        if to_reduce == 0:
            return VTKPartitionedArray(results, dataset=dataset)

        lresults = numpy.empty(size * to_reduce)
        lresults.fill(impl.default())

        if dataset is not None:
            it = dataset.NewIterator()
            it.UnRegister(None)
            ids = []
            while not it.IsDoneWithTraversal():
                ids.append(it.GetCurrentFlatIndex())
                it.GoToNextItem()

        for _id, _res in zip(ids, results):
            success = True
            try:
                loc = reduce_ids.index(_id)
            except ValueError:
                success = False
            if success:
                if _res is not NoneArray:
                    lresults[loc * size:(loc + 1) * size] = _res.flatten()

        rresults = numpy.array(lresults)
        mpitype = _lookup_mpi_type(numpy.double)
        comm.Allreduce([lresults, mpitype], [rresults, mpitype], impl.mpi_op())

        if array is NoneArray:
            return NoneArray

        for i in range(to_reduce):
            _id = reduce_ids[i]
            success = True
            try:
                loc = ids.index(_id)
            except ValueError:
                success = False
            if success:
                if size == 1:
                    results[loc] = numpy.array(rresults[i])
                else:
                    results[loc] = rresults[i * size:(i + 1) * size].reshape(max_dims)

    return VTKPartitionedArray(results, dataset=dataset)

def sum_per_block(array, axis=None, controller=None):
    """Per-block sum with MPI support."""
    class SumPerBlockImpl:
        def op(self):
            return sum

        def op2(self):
            return _sum

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.SUM

        def default(self):
            return numpy.float64(0)

    return _global_per_block(SumPerBlockImpl(), array, axis, controller)

def count_per_block(array, axis=None, controller=None):
    """Per-block count with MPI support."""
    if axis is not None and axis > 0:
        raise ValueError("Only axis=None and axis=0 are supported for count")

    class CountPerBlockImpl:
        def op(self):
            return array_count

        def op2(self):
            return _local_array_count

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.SUM

        def default(self):
            return numpy.float64(0)

    return _global_per_block(CountPerBlockImpl(), array, axis, controller)

def mean_per_block(array, axis=None, controller=None):
    """Per-block mean with MPI support."""
    if axis is None or axis == 0:
        return sum_per_block(array, axis, controller) / count_per_block(array, axis, controller)
    else:
        return sum(array, axis, controller)

def max_per_block(array, axis=None, controller=None):
    """Per-block max with MPI support."""
    class MaxPerBlockImpl:
        def op(self):
            return max

        def op2(self):
            return _max

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.MAX

        def default(self):
            return numpy.finfo(numpy.float64).min

    return _global_per_block(MaxPerBlockImpl(), array, axis, controller)

def min_per_block(array, axis=None, controller=None):
    """Per-block min with MPI support."""
    class MinPerBlockImpl:
        def op(self):
            return min

        def op2(self):
            return _min

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.MIN

        def default(self):
            return numpy.finfo(numpy.float64).max

    return _global_per_block(MinPerBlockImpl(), array, axis, controller)

# ===========================================================================
# Section 11: Dataset utility
# ===========================================================================

def unstructured_from_composite_arrays(points, arrays, controller=None):
    """Create a vtkUnstructuredGrid from VTKPartitionedArrays."""
    try:
        dataset = points.dataset
    except AttributeError:
        dataset = None

    if dataset is None and points is not NoneArray:
        raise ValueError("Expecting a points arrays with an associated dataset.")

    if points is NoneArray:
        cpts = []
    else:
        cpts = points.arrays
    ownership = numpy.zeros(len(cpts), dtype=numpy.int32)
    rank = 0

    if dataset is None:
        ids = []
    else:
        it = dataset.NewIterator()
        it.UnRegister(None)
        ids = numpy.empty(len(cpts), dtype=numpy.int32)
        counter = 0
        while not it.IsDoneWithTraversal():
            _id = it.GetCurrentFlatIndex()
            ids[counter] = _id
            counter += 1
            it.GoToNextItem()

    if controller is None and vtkMultiProcessController is not None:
        controller = vtkMultiProcessController.GetGlobalController()
    if controller and controller.IsA("vtkMPIController"):
        from mpi4py import MPI
        comm = vtkMPI4PyCommunicator.ConvertToPython(controller.GetCommunicator())
        rank = comm.Get_rank()

        lmax_id = numpy.int32(0)
        if dataset is not None:
            it = dataset.NewIterator()
            it.UnRegister(None)
            it.SetSkipEmptyNodes(False)
            while not it.IsDoneWithTraversal():
                _id = it.GetCurrentFlatIndex()
                lmax_id = numpy.max((lmax_id, _id)).astype(numpy.int32)
                it.GoToNextItem()
        max_id = numpy.array(0, dtype=numpy.int32)
        mpitype = _lookup_mpi_type(numpy.int32)
        comm.Allreduce([lmax_id, mpitype], [max_id, mpitype], MPI.MAX)

        lownership = numpy.empty(max_id, dtype=numpy.int32)
        lownership.fill(numpy.iinfo(numpy.int32).max)
        ownership = numpy.empty(max_id, dtype=numpy.int32)

        if dataset is not None:
            it = dataset.NewIterator()
            it.UnRegister(None)
            it.InitTraversal()
            itr = cpts.__iter__()
            while not it.IsDoneWithTraversal():
                _id = it.GetCurrentFlatIndex()
                if next(itr) is not NoneArray:
                    lownership[_id] = rank
                it.GoToNextItem()
        mpitype = _lookup_mpi_type(numpy.int32)
        comm.Allreduce([lownership, mpitype], [ownership, mpitype], MPI.MIN)

    from ..vtkCommonDataModel import vtkUnstructuredGrid
    from ..vtkCommonCore import vtkDoubleArray, vtkPoints
    ugrid = vtkUnstructuredGrid()
    da = vtkDoubleArray()
    da.SetNumberOfComponents(3)
    pts = vtkPoints()
    pts.SetData(da)
    counter = 0
    for pt in cpts:
        if ownership[ids[counter]] == rank:
            pts.InsertNextPoint(tuple(pt))
        counter += 1
    ugrid.SetPoints(pts)

    for ca, name in arrays:
        if ca is not NoneArray:
            da = vtkDoubleArray()
            ncomps = ca.arrays[0].flatten().shape[0]
            da.SetNumberOfComponents(ncomps)
            counter = 0
            for a in ca.arrays:
                if ownership[ids[counter]] == rank:
                    a = a.flatten()
                    for i in range(ncomps):
                        da.InsertNextValue(a[i])
                counter += 1
            if len(a) > 0:
                da.SetName(name)
                ugrid.GetPointData().AddArray(da)
    return ugrid

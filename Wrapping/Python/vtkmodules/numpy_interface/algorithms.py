"""
This module provides a number of algorithms that can be used with
the dataset classes defined in the dataset_adapter module.
See the documentation of the dataset_adapter for some examples.
These algorithms work in serial and in parallel as long as the
data is partitioned according to VTK data parallel execution
guidelines. For details, see the documentation of individual
algorithms.
"""
from __future__ import absolute_import

import numpy
from typing import Union

from . import dataset_adapter as dsa
from . import internal_algorithms as algs
from . import numpy_algorithms as npalgs
from vtkmodules.util.misc import deprecated
try:
    from ..vtkParallelCore import vtkMultiProcessController
    from ..vtkParallelMPI4Py import vtkMPI4PyCommunicator
except ImportError:
    vtkMultiProcessController = None
    vtkMPI4PyCommunicator = None

def _lookup_mpi_type(ntype):
    from mpi4py import MPI
    if ntype == bool:
        typecode = 'b'
    else:
        typecode = numpy.dtype(ntype).char
    try:
        return MPI.__TypeDict__[typecode]
    except AttributeError:
        # handle mpi4py 2.0.0
        return MPI._typedict[typecode]

def _reduce_dims(array, comm):
    from mpi4py import MPI
    dims = numpy.array([0, 0], dtype=numpy.int32)
    if array is not dsa.NoneArray:
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
        size = max_dims[0]*max_dims[1]

    if max_dims[0] == 1:
        max_dims = 1

    return (max_dims, size)

def global_func(impl, array, axis, controller) -> Union[numpy.ndarray, dsa.VTKNoneArray, dsa.VTKCompositeDataArray]:
    """Helper function for reduction-like operation (e.g., sum, min, max) to the given array,
    with support for parallel execution across different nodes."""
    if type(array) == dsa.VTKCompositeDataArray:
        if axis is None or axis == 0:
            res = impl.serial_composite(array, axis)
        else:
            res = npalgs.apply_ufunc(impl.op(), array, (axis,))
    else:
        res = impl.op()(array, axis)
        if res is not dsa.NoneArray:
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

            # All NoneArrays
            if size == 0:
                return dsa.NoneArray

            if res is dsa.NoneArray:
                if numpy.isscalar(max_dims):
                    # Weird trick to make the array look like a scalar
                    max_dims = ()
                res = numpy.empty(max_dims)
                res.fill(impl.default())

            res_recv = numpy.array(res)
            mpi_type = _lookup_mpi_type(res.dtype)
            comm.Allreduce([res, mpi_type], [res_recv, mpi_type], impl.mpi_op())
            if array is dsa.NoneArray:
                return dsa.NoneArray
            res = res_recv

    return res

@deprecated(version="9.6", message="Use np.bitwise_or() instead of algs.bitwise_or().")
def bitwise_or(array1, array2):
    """Implements element by element or (bitwise, | in C/C++) operation.
    If one of the arrays is a NoneArray, this will return the array
    that is not NoneArray, treating NoneArray as 0 in the or operation."""
    return npalgs.bitwise_or(array1, array2)

def make_point_mask_from_NaNs(dataset, array):
    """This method will create a ghost array corresponding to an
    input with NaN values. For each NaN value, the output array will
    have a corresponding value of vtkmodules.vtkCommonDataModel.vtkDataSetAttributes.HIDDENPOINT.
    These values are also combined with any ghost values that the
    dataset may have."""
    from ..vtkCommonDataModel import vtkDataSetAttributes
    ghosts = dataset.PointData[vtkDataSetAttributes.GhostArrayName()]
    return make_mask_from_NaNs(array, ghosts)

def make_cell_mask_from_NaNs(dataset, array):
    """This method will create a ghost array corresponding to an
    input with NaN values. For each NaN value, the output array will
    have a corresponding value of vtkmodules.vtkCommonDataModel.vtkDataSetAttributes.HIDDENCELL.
    These values are also combined with any ghost values that the
    dataset may have."""
    from ..vtkCommonDataModel import vtkDataSetAttributes
    ghosts = dataset.CellData[vtkDataSetAttributes.GhostArrayName()]
    return make_mask_from_NaNs(array, ghosts, True)

def make_mask_from_NaNs(array, ghost_array=dsa.NoneArray, is_cell=False):
    """This method will create a ghost array corresponding to an
    input with NaN values. For each NaN value, the output array will
    have a corresponding value of vtkmodules.vtkCommonDataModel.vtkDataSetAttributes.HIDDENPOINT or
    HIDDENCELL is the is_cell argument is true. If an input ghost_array
    is passed, the array is bitwise_or'ed with it, simply adding
    the new ghost values to it."""
    from ..vtkCommonDataModel import vtkDataSetAttributes
    if is_cell:
        mask_value = vtkDataSetAttributes.HIDDENCELL
    else:
        mask_value = vtkDataSetAttributes.HIDDENPOINT

    return npalgs.bitwise_or(isnan(array).astype(numpy.uint8) * mask_value,
        ghost_array)

@deprecated(version="9.6", message="Use np.sum() instead of algs.sum().")
def sum(array, axis=None, controller=None):
    """Returns the sum of all values along a particular axis (dimension).
    Given an array of m tuples and n components:
    * Default is to return the sum of all values in an array.
    * axis=0: Sum values of all components and return a one tuple,
      n-component array.
    * axis=1: Sum values of all components of each tuple and return an
      m-tuple, 1-component array.

    When called in parallel, this function will sum across processes
    when a controller argument is passed or the global controller is
    defined. To disable parallel summing when running in parallel, pass
    a dummy controller as follows:

    sum(array, controller=vtkmodules.vtkParallelCore.vtkDummyController()).
    """
    return npalgs.sum(array, axis, controller)

@deprecated(version="9.6", message="Use np.max() instead of algs.max().")
def max(array, axis=None, controller=None):
    """Returns the max of all values along a particular axis (dimension).
    Given an array of m tuples and n components:
    * Default is to return the max of all values in an array.
    * axis=0: Return the max values of all tuples and return a
      one tuple, n-component array.
    * axis=1: Return the max values of all components of each tuple
      and return an m-tuple, 1-component array.

    When called in parallel, this function will compute the max across
    processes when a controller argument is passed or the global controller
    is defined. To disable parallel summing when running in parallel, pass a
    dummy controller as follows:

    max(array, controller=vtkmodules.vtkParallelCore.vtkDummyController()).
    """
    return npalgs.max(array, axis, controller)

@deprecated(version="9.6", message="Use np.min() instead of algs.min().")
def min(array, axis=None, controller=None):
    """Returns the min of all values along a particular axis (dimension).
    Given an array of m tuples and n components:
    * Default is to return the min of all values in an array.
    * axis=0: Return the min values of all tuples and return a one
      tuple, n-component array.
    * axis=1: Return the min values of all components of each tuple and
      return an m-tuple, 1-component array.

    When called in parallel, this function will compute the min across processes
    when a controller argument is passed or the global controller is defined.
    To disable parallel summing when running in parallel, pass a dummy controller as follows:

    min(array, controller=vtkmodules.vtkParallelCore.vtkDummyController()).
    """
    return npalgs.min(array, axis, controller)

def _global_per_block(impl, array, axis=None, controller=None):
    if axis is not None and axis > 0:
        return impl.op()(array, axis=axis, controller=controller)
    try:
        dataset = array.DataSet
    except AttributeError:
        dataset = None
    t = type(array)
    if t == dsa.VTKArray or t == numpy.ndarray:
        from ..vtkCommonDataModel import vtkMultiBlockDataSet
        array = dsa.VTKCompositeDataArray([array])
        ds = vtkMultiBlockDataSet()
        ds.SetBlock(0, dataset.VTKObject)
        dataset = ds
    results = npalgs._apply_func2(impl.op2(), array, (axis,))
    if controller is None and vtkMultiProcessController is not None:
        controller = vtkMultiProcessController.GetGlobalController()
    if controller and controller.IsA("vtkMPIController"):
        from mpi4py import MPI
        comm = vtkMPI4PyCommunicator.ConvertToPython(controller.GetCommunicator())

        # First determine the number of components to use
        # for reduction
        res = dsa.NoneArray
        for res in results:
            if res is not dsa.NoneArray:
                break

        max_dims, size = _reduce_dims(res, comm)

        # All NoneArrays
        if size == 0:
            return dsa.NoneArray

        # Next determine the max id to use for reduction
        # operations

        # Get all ids from dataset, including empty ones.
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

        has_ids = numpy.zeros(max_id+1, dtype=numpy.int32)
        for _id in ids:
            has_ids[_id] = 1
        id_count = numpy.array(has_ids)
        comm.Allreduce([has_ids, mpitype], [id_count, mpitype], MPI.SUM)

        if numpy.all(id_count <= 1):
            return dsa.VTKCompositeDataArray(results, dataset=dataset)

        # Now that we know which blocks are shared by more than
        # 1 rank. The ones that have a count of 2 or more.
        reduce_ids = []
        for _id in range(len(id_count)):
            if id_count[_id] > 1:
                reduce_ids.append(_id)

        to_reduce = len(reduce_ids)
        # If not block is shared, short circuit. No need to
        # communicate any more.
        if to_reduce == 0:
            return dsa.VTKCompositeDataArray(results, dataset=dataset)

        # Create the local array that will be used for
        # reduction. Set it to a value that won't effect
        # the reduction.
        lresults = numpy.empty(size*to_reduce)
        lresults.fill(impl.default())

        # Just get non-empty ids. Doing this again in case
        # the traversal above results in a different order.
        # We need the same order since we'll use zip below.
        if dataset is not None:
            it = dataset.NewIterator()
            it.UnRegister(None)
            ids = []
            while not it.IsDoneWithTraversal():
                ids.append(it.GetCurrentFlatIndex())
                it.GoToNextItem()

        # Fill the local array with available values.
        for _id, _res in zip(ids, results):
            success = True
            try:
                loc = reduce_ids.index(_id)
            except ValueError:
                success = False
            if success:
                if _res is not dsa.NoneArray:
                    lresults[loc*size:(loc+1)*size] = _res.flatten()

        # Now do the MPI reduction.
        rresults = numpy.array(lresults)
        mpitype = _lookup_mpi_type(numpy.double)
        comm.Allreduce([lresults, mpitype], [rresults, mpitype], impl.mpi_op())

        if array is dsa.NoneArray:
            return dsa.NoneArray

        # Fill in the reduced values.
        for i in range(to_reduce):
            _id = reduce_ids[i]
            success = True
            try:
                loc = ids.index(_id)
            except ValueError:
                success = False
            if success:
                if size == 1:
                    results[loc] = dsa.VTKArray(rresults[i])
                else:
                    results[loc] = rresults[i*size:(i+1)*size].reshape(max_dims)

    return dsa.VTKCompositeDataArray(results, dataset=dataset)

def sum_per_block(array, axis=None, controller=None):
    """Returns the sum of all values along a particular axis (dimension) for
    each block of an VTKCompositeDataArray.

    Given an array of m tuples and n components:
    * Default is to return the sum of all values in an array.
    * axis=0: Sum values of all components and return a one tuple,
      n-component array.
    * axis=1: Sum values of all components of each tuple and return an
      m-tuple, 1-component array.

    When called in parallel, this function will sum across processes
    when a controller argument is passed or the global controller is
    defined. To disable parallel summing when running in parallel, pass
    a dummy controller as follows:

    sum_per_block(array, controller=vtkmodules.vtkParallelCore.vtkDummyController()).
    """
    class SumPerBlockImpl:
        def op(self):
            return sum

        def op2(self):
            return algs.sum

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.SUM

        def default(self):
            return numpy.float64(0)

    return _global_per_block(SumPerBlockImpl(), array, axis, controller)

def count_per_block(array, axis=None, controller=None):
    """Return the number of elements of each block in a VTKCompositeDataArray
    along an axis.

    - if axis is None, the number of all elements (ntuples * ncomponents) is
    returned.
    - if axis is 0, the number of tuples is returned.
    """

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
    """Returns the mean of all values along a particular axis (dimension)
    for each block of a VTKCompositeDataArray.

    Given an array of m tuples and n components:
    * Default is to return the mean of all values in an array.
    * axis=0: Return the mean values of all components and return a one
      tuple, n-component array.
    * axis=1: Return the mean values of all components of each tuple and
      return an m-tuple, 1-component array.

    When called in parallel, this function will compute the mean across
    processes when a controller argument is passed or the global controller
    is defined. To disable parallel summing when running in parallel, pass a
    dummy controller as follows:

    mean(array, controller=vtkmodules.vtkParallelCore.vtkDummyController()).
    """
    if axis is None or axis == 0:
        return sum_per_block(array, axis, controller) / count_per_block(array, axis, controller)
    else:
        return sum(array, axis, controller)

def max_per_block(array, axis=None, controller=None):
    """Returns the max of all values along a particular axis (dimension)
    for each block of a VTKCompositeDataArray.
    Given an array of m tuples and n components:
    * Default is to return the max of all values in an array.
    * axis=0: Return the max values of all components and return a one
      tuple, n-component array.
    * axis=1: Return the max values of all components of each tuple and return
      an m-tuple, 1-component array.

    When called in parallel, this function will compute the max across
    processes when a controller argument is passed or the global controller
    is defined. To disable parallel summing when running in parallel, pass a
    dummy controller as follows:

    max_per_block(array, controller=vtkmodules.vtkParallelCore.vtkDummyController()).
    """
    class MaxPerBlockImpl:
        def op(self):
            return max

        def op2(self):
            return algs.max

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.MAX

        def default(self):
            return numpy.finfo(numpy.float64).min

    return _global_per_block(MaxPerBlockImpl(), array, axis, controller)

def min_per_block(array, axis=None, controller=None):
    """Returns the min of all values along a particular axis (dimension)
    for each block of a VTKCompositeDataArray.
    Given an array of m tuples and n components:
    * Default is to return the min of all values in an array.
    * axis=0: Return the min values of all components and return a one
      tuple, n-component array.
    * axis=1: Return the min values of all components of each tuple and
      return an m-tuple, 1-component array.

    When called in parallel, this function will compute the min across
    processes when a controller argument is passed or the global controller
    is defined. To disable parallel summing when running in parallel, pass a
    dummy controller as follows:

    min_per_block(array, controller=vtkmodules.vtkParallelCore.vtkDummyController()).
    """
    class MinPerBlockImpl:
        def op(self):
            return min

        def op2(self):
            return algs.min

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.MIN

        def default(self):
            return numpy.finfo(numpy.float64).max

    return _global_per_block(MinPerBlockImpl(), array, axis, controller)

@deprecated(version="9.6", message="Use np.all() instead of algs.all().")
def all(array, axis=None, controller=None):
    """Returns True if all values of an array evaluate to True, returns
    False otherwise.
    This is useful to check if all values of an array match a certain
    condition such as:

    algorithms.all(array > 5)
    """
    return npalgs.all(array, axis, controller)

def _local_array_count(array, axis):

    if array is dsa.NoneArray:
        return numpy.int64(0)
    elif axis is None:
        return numpy.int64(array.size)
    else:
        return numpy.int64(numpy.shape(array)[0])

def array_count(array, axis, controller) -> Union[numpy.int64, numpy.ndarray]:
    """Helper function to count the number of elements in a VTK or NumPy array,
    supporting counting across MPI nodes."""
    if array is dsa.NoneArray:
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

@deprecated(version="9.6", message="Use np.mean() instead of algs.mean().")
def mean(array, axis=None, controller=None, size=None):
    """Returns the mean of all values along a particular axis (dimension).
    Given an array of m tuples and n components:
    * Default is to return the mean of all values in an array.
    * axis=0: Return the mean values of all components and return a one
      tuple, n-component array.
    * axis=1: Return the mean values of all components of each tuple and
      return an m-tuple, 1-component array.

    When called in parallel, this function will compute the mean across
    processes when a controller argument is passed or the global controller
    is defined. To disable parallel summing when running in parallel, pass a
    dummy controller as follows:

    mean(array, controller=vtkmodules.vtkParallelCore.vtkDummyController()).
    """
    return npalgs.mean(array, axis, controller, size)

@deprecated(version="9.6", message="Use np.var() instead of algs.var().")
def var(array, axis=None, controller=None):
    """Returns the variance of all values along a particular axis (dimension).
    Given an array of m tuples and n components:
    * Default is to return the variance of all values in an array.
    * axis=0: Return the variance values of all components and return a one
      tuple, n-component array.
    * axis=1: Return the variance values of all components of each tuple and
      return an m-tuple, 1-component array.

    When called in parallel, this function will compute the variance across
    processes when a controller argument is passed or the global controller
    is defined. To disable parallel summing when running in parallel, pass a
    dummy controller as follows:

    var(array, controller=vtkmodules.vtkParallelCore.vtkDummyController()).
    """
    return npalgs.var(array, axis, controller)

@deprecated(version="9.6", message="Use np.std() instead of algs.std().")
def std(array, axis=None, controller=None):
    """Returns the standard deviation of all values along a particular
    axis (dimension).
    Given an array of m tuples and n components:
    * Default is to return the standard deviation of all values in an array.
    * axis=0: Return the standard deviation values of all components and
      return a one tuple, n-component array.
    * axis=1: Return the standard deviation values of all components of
      each tuple and return an m-tuple, 1-component array.

    When called in parallel, this function will compute the standard deviation
    across processes when a controller argument is passed or the global controller
    is defined. To disable parallel summing when running in parallel, pass a dummy
    controller as follows:

    std(array, controller=vtkmodules.vtkParallelCore.vtkDummyController()).
    """
    return npalgs.std(array, axis, controller)

@deprecated(version="9.6", message="Use np.shape() instead of algs.shape().")
def shape(array):
    "Returns the shape (dimensions) of an array."
    return npalgs.shape(array)

def make_vector(arrayx, arrayy, arrayz=None):
    """Given 2 or 3 scalar arrays, returns a vector array. If only
    2 scalars are provided, the third component will be set to 0."""
    if type(arrayx) == dsa.VTKCompositeDataArray and type(arrayy) == dsa.VTKCompositeDataArray and (type(arrayz) == dsa.VTKCompositeDataArray or arrayz is None):
        res = []
        if arrayz is None:
            for ax, ay in zip(arrayx.Arrays, arrayy.Arrays):
                if ax is not dsa.NoneArray and ay is not dsa.NoneArray:
                    res.append(algs.make_vector(ax, ay))
                else:
                    res.append(dsa.NoneArray)
        else:
            for ax, ay, az in zip(arrayx.Arrays, arrayy.Arrays, arrayz.Arrays):
                if ax is not dsa.NoneArray and ay is not dsa.NoneArray and az is not dsa.NoneArray:
                    res.append(algs.make_vector(ax, ay, az))
                else:
                    res.append(dsa.NoneArray)
        return dsa.VTKCompositeDataArray(res, dataset = arrayx.DataSet)
    else:
        return algs.make_vector(arrayx, arrayy, arrayz)

def unstructured_from_composite_arrays(points, arrays, controller=None):
    """Given a set of VTKCompositeDataArrays, creates a vtkUnstructuredGrid.
    The main goal of this function is to transform the output of XXX_per_block()
    methods to a single dataset that can be visualized and further processed.
    Here arrays is an iterable (e.g. list) of (array, name) pairs. Here is
    an example:

    centroid = mean_per_block(composite_data.Points)
    T = mean_per_block(composite_data.PointData['Temperature'])
    ug = unstructured_from_composite_arrays(centroid, (T, 'Temperature'))

    When called in parallel, this function makes sure that each array in
    the input dataset is represented only on 1 process. This is important
    because methods like mean_per_block() return the same value for blocks
    that are partitioned on all of the participating processes. If the
    same point were to be created across multiple processes in the output,
    filters like histogram would report duplicate values erroneously.
    """

    try:
        dataset = points.DataSet
    except AttributeError:
        dataset = None

    if dataset is None and points is not dsa.NoneArray:
        raise ValueError("Expecting a points arrays with an associated dataset.")

    if points is dsa.NoneArray:
        cpts = []
    else:
        cpts = points.Arrays
    ownership = numpy.zeros(len(cpts), dtype=numpy.int32)
    rank = 0

    # Let's first create a map of array index to composite ids.
    if dataset is None:
        ids = []
    else:
        it = dataset.NewIterator()
        it.UnRegister(None)
        itr = cpts.__iter__()
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

        # Determine the max id to use for reduction
        # operations

        # Get all ids from dataset, including empty ones.
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

        # Now we figure out which processes have which ids
        lownership = numpy.empty(max_id, dtype = numpy.int32)
        lownership.fill(numpy.iinfo(numpy.int32).max)

        ownership = numpy.empty(max_id, dtype = numpy.int32)

        if dataset is not None:
            it = dataset.NewIterator()
            it.UnRegister(None)
            it.InitTraversal()
            itr = cpts.__iter__()
            while not it.IsDoneWithTraversal():
                _id = it.GetCurrentFlatIndex()
                if next(itr) is not dsa.NoneArray:
                    lownership[_id] = rank
                it.GoToNextItem()
        mpitype = _lookup_mpi_type(numpy.int32)
        # The process with the lowest id containing a block will
        # produce the output for that block.
        comm.Allreduce([lownership, mpitype], [ownership, mpitype], MPI.MIN)

    # Iterate over blocks to produce points and arrays
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
        if ca is not dsa.NoneArray:
            da = vtkDoubleArray()
            ncomps = ca.Arrays[0].flatten().shape[0]
            da.SetNumberOfComponents(ncomps)
            counter = 0
            for a in ca.Arrays:
                if ownership[ids[counter]] == rank:
                    a = a.flatten()
                    for i in range(ncomps):
                        da.InsertNextValue(a[i])
                counter += 1
            if len(a) > 0:
                da.SetName(name)
                ugrid.GetPointData().AddArray(da)
    return ugrid

@deprecated(version="9.6", message="Use np.flatnonzero() instead of algs.flatnonzero().")
def flatnonzero(array):
    """
    Return indices that are non-zero in the flattened version of the input array.
    """
    return npalgs.flatnonzero(array)

@deprecated(version="9.6", message="Use np.nonzero() instead of algs.nonzero().")
def nonzero(array):
    """
    Return the indices of the non-zero elements of the input array.
    """
    return npalgs.nonzero(array)

@deprecated(version="9.6", message="Use np.where() instead of algs.where().")
def where(*args):
    """Returns the location (indices) of an array where the given
    expression is true. For scalars, it returns a single array of indices.
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
    return npalgs.where(*args)

isin = deprecated(version="9.6", message="Use np.isin() instead of algs.isin().")(npalgs.isin)
isin.__doc__ = "Test whether each element of a 1-D array is also present in a second array."

isnan = deprecated(version="9.6", message="Use np.isnan() instead of algs.isnan().")(npalgs._make_ufunc(numpy.isnan))
isnan.__doc__ = "Returns a bool array, true if values is nan."

sqrt = deprecated(version="9.6", message="Use np.sqrt() instead of algs.sqrt().")(npalgs._make_ufunc(numpy.sqrt))
sqrt.__doc__ = "Computes square root."

negative = deprecated(version="9.6", message="Use np.negative() instead of algs.negative().")(npalgs._make_ufunc(numpy.negative))
negative.__doc__ = "Numerical negative, element-wise."

reciprocal = deprecated(version="9.6", message="Use np.reciprocal() instead of algs.reciprocal().")(npalgs._make_ufunc(numpy.sqrt))
reciprocal.__doc__ = "Return the reciprocal (1/x) of the argument, element-wise."

square = deprecated(version="9.6", message="Use np.square() instead of algs.square().")(npalgs._make_ufunc(numpy.square))
square.__doc__ = "Return the element-wise square of the input."

exp = deprecated(version="9.6", message="Use np.exp() instead of algs.exp().")(npalgs._make_ufunc(numpy.exp))
exp.__doc__ = "The exponential function."

floor = deprecated(version="9.6", message="Use np.floor() instead of algs.floor().")(npalgs._make_ufunc(numpy.floor))
floor.__doc__ = "Returns the floor of floating point values."

ceil = deprecated(version="9.6", message="Use np.ceil() instead of algs.ceil().")(npalgs._make_ufunc(numpy.ceil))
ceil.__doc__ = "Returns the ceiling of floating point values."

rint = deprecated(version="9.6", message="Use np.rint() instead of algs.rint().")(npalgs._make_ufunc(numpy.rint))
rint.__doc__ = "Round elements of the array to the nearest integer."

sin = deprecated(version="9.6", message="Use np.sin() instead of algs.sin().")(npalgs._make_ufunc(numpy.sin))
sin.__doc__ = "Computes sine of values in radians."

cos = deprecated(version="9.6", message="Use np.cos() instead of algs.cos().")(npalgs._make_ufunc(numpy.cos))
cos.__doc__ = "Computes cosine of values in radians."

tan = deprecated(version="9.6", message="Use np.tan() instead of algs.tan().")(npalgs._make_ufunc(numpy.tan))
tan.__doc__ = "Computes tangent of values in radians."

arcsin = deprecated(version="9.6", message="Use np.arcsin() instead of algs.arcsin().")(npalgs._make_ufunc(numpy.arcsin))
arcsin.__doc__ = "Computes inverse sine."

arccos = deprecated(version="9.6", message="Use np.arccos() instead of algs.arccos().")(npalgs._make_ufunc(numpy.arccos))
arccos.__doc__ = "Computes inverse cosine."

arctan = deprecated(version="9.6", message="Use np.arctan() instead of algs.arctan().")(npalgs._make_ufunc(numpy.arctan))
arctan.__doc__ = "Computes inverse tangent."

arctan2 = deprecated(version="9.6", message="Use np.arctan2() instead of algs.arctan2().")(npalgs._make_ufunc(numpy.arctan2))
arctan2.__doc__ = "Computes inverse tangent using two arguments."

sinh = deprecated(version="9.6", message="Use np.sinh() instead of algs.sinh().")(npalgs._make_ufunc(numpy.sinh))
sinh.__doc__ = "Computes hyperbolic sine."

cosh = deprecated(version="9.6", message="Use np.cosh() instead of algs.cosh().")(npalgs._make_ufunc(numpy.cosh))
cosh.__doc__ = "Computes hyperbolic cosine."

tanh = deprecated(version="9.6", message="Use np.tanh() instead of algs.tanh().")(npalgs._make_ufunc(numpy.tanh))
tanh.__doc__ = "Computes hyperbolic tangent."

arcsinh = deprecated(version="9.6", message="Use np.arcsinh() instead of algs.arcsinh().")(npalgs._make_ufunc(numpy.arcsinh))
arcsinh.__doc__ = "Computes inverse hyperbolic sine."

arccosh = deprecated(version="9.6", message="Use np.arccosh() instead of algs.arccosh().")(npalgs._make_ufunc(numpy.arccosh))
arccosh.__doc__ = "Computes inverse hyperbolic cosine."

arctanh = deprecated(version="9.6", message="Use np.arctanh() instead of algs.arctanh().")(npalgs._make_ufunc(numpy.arctanh))
arctanh.__doc__ = "Computes inverse hyperbolic tangent."

expand_dims = deprecated(version="9.6", message="Use np.expand_dims() instead of algs.expand_dims().")(npalgs.expand_dims)
expand_dims.__doc__ = """Insert a new dimension, corresponding to a given
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

>>> v / algs.expand_dims(algs.mag(v), 1)
VTKArray([[ 0.57735027,  0.57735027,  0.57735027],
       [ 0.57735027,  0.57735027,  0.57735027],
       [ 0.57735027,  0.57735027,  0.57735027],
       [ 0.57735027,  0.57735027,  0.57735027],
       [ 0.57735027,  0.57735027,  0.57735027]])"""

abs = deprecated(version="9.6", message="Use np.abs() instead of algs.abs().")(npalgs._make_ufunc(algs.abs))
abs.__doc__ = "Returns the absolute values of an array of scalars/vectors/tensors."

area = npalgs._make_dsfunc2(algs.area)
area.__doc__ = "Returns the surface area of each 2D cell in a mesh."

aspect = npalgs._make_dsfunc2(algs.aspect)
aspect.__doc__ = "Returns the aspect ratio of each cell in a mesh. See Verdict documentation for details."

aspect_gamma = npalgs._make_dsfunc2(algs.aspect_gamma)
aspect_gamma.__doc__ = "Returns the aspect gamma of each cell in a mesh. This metric compares root-mean-square edge length to volume. See Verdict documentation for details."

condition = npalgs._make_dsfunc2(algs.condition)
condition.__doc__ = "Returns the condition number of each cell in a mesh. See Verdict documentation for details."

cross = npalgs._make_dfunc(algs.cross)
cross.__doc__ = "Return the cross product of two vectors."

curl = npalgs._make_dsfunc(algs.curl)
curl.__doc__ = "Returns the curl a vector field."

divergence = npalgs._make_dsfunc(algs.divergence)
divergence.__doc__ = "Returns the divergence of a vector field."

det = npalgs._make_ufunc(algs.det)
det.__doc__ = "Returns the determinant of 2D matrices."

determinant = npalgs._make_ufunc(algs.determinant)
determinant.__doc__ = "Returns the determinant of 2D matrices."

diagonal = npalgs._make_dsfunc2(algs.diagonal)
diagonal.__doc__ = "Returns the diagonal length of each cell in a dataset. See Verdict documentation for details"

dot = npalgs._make_dfunc(algs.dot)
dot.__doc__ = "Returns the dot product of two vectors."

eigenvalue = npalgs._make_ufunc(algs.eigenvalue)
eigenvalue.__doc__ = "Returns the eigenvalues of 3x3 matrices. Currently only works with symmetric matrices."

eigenvector = npalgs._make_ufunc(algs.eigenvector)
eigenvector.__doc__ = "Returns the eigenvectors of 3x3 matrices. Currently only works with symmetric matrices."

gradient = npalgs._make_dsfunc(algs.gradient)
gradient.__doc__ = "Returns the gradient of scalars or vectors."

inv = npalgs._make_ufunc(algs.inv)
inv.__doc__ = "Returns the inverse of 3x3 matrices."

inverse = npalgs._make_ufunc(algs.inverse)
inverse.__doc__ = "Returns the inverse of 3x3 matrices."

jacobian = npalgs._make_dsfunc2(algs.jacobian)
jacobian.__doc__ = "Returns the Jacobian of a dataset."

laplacian = npalgs._make_dsfunc(algs.laplacian)
laplacian.__doc__ = "Returns the Laplacian of a scalar field."

ln = npalgs._make_ufunc(algs.ln)
ln.__doc__ = "Returns the natural logarithm of its input."

log = deprecated(version="9.6", message="Use np.log() instead of algs.log().")(npalgs._make_ufunc(algs.log))
log.__doc__ = "Returns the natural logarithm of its input."

log10 = deprecated(version="9.6", message="Use np.log10() instead of algs.log10().")(npalgs._make_ufunc(algs.log10))
log10.__doc__ = "Returns the base 10 logarithm of its input."

max_angle = npalgs._make_dsfunc2(algs.max_angle)
max_angle.__doc__ = "Returns the maximum angle of each cell in a dataset. See Verdict documentation for details"

mag = npalgs._make_ufunc(algs.mag)
mag.__doc__ = "Returns the magnitude of vectors."

matmul = npalgs._make_dfunc(algs.matmul)
matmul.__doc__ = "Return the product of the inputs. Inputs can be vectors/tensors."

min_angle = npalgs._make_dsfunc2(algs.min_angle)
min_angle.__doc__ = "Returns the minimum angle of each cell in a dataset."

norm = npalgs._make_ufunc(algs.norm)
norm.__doc__ = "Computes the normalized values of vectors."

shear = npalgs._make_dsfunc2(algs.shear)
shear.__doc__ = "Returns the shear of each cell in a dataset. See Verdict documentation for details."

skew = npalgs._make_dsfunc2(algs.skew)
skew.__doc__ = "Returns the skew of each cell in a dataset. See Verdict documentation for details."

strain = npalgs._make_dsfunc(algs.strain)
strain.__doc__ = "Given a deformation vector, this function computes the infinitesimal (Cauchy) strain tensor. It can also be used to compute strain rate if the input is velocity."

surface_normal = npalgs._make_dsfunc2(algs.surface_normal)
surface_normal.__doc__ = "Returns the surface normal of each cell in a dataset."

trace = npalgs._make_ufunc(algs.trace)
trace.__doc__ = "Returns the trace of square matrices."

volume = npalgs._make_dsfunc2(algs.volume)
volume.__doc__ = "Returns the volume of each cell in a dataset. Use sum to calculate total volume of a dataset."

vorticity = npalgs._make_dsfunc(algs.vorticity)
vorticity.__doc__ = "Given a velocity field, calculates vorticity."

vertex_normal = npalgs._make_dsfunc2(algs.vertex_normal)
vertex_normal.__doc__ = "Returns the normal at each vertex of a dataset, which is defined as the average of the cell normals of all cells containing that vertex."

logical_not = deprecated(version="9.6", message="Use np.logical_not() instead of algs.logical_not().")(npalgs._make_ufunc(numpy.logical_not))
logical_not.__doc__ = "Computes the truth value of NOT x element-wise."

divide = deprecated(version="9.6", message="Use np.divide() instead of algs.divide().")(npalgs._make_dfunc(numpy.divide))
divide.__doc__ = "Element by element division. Both elements can be single values or arrays. Same as /."

multiply = deprecated(version="9.6", message="Use np.multiply() instead of algs.multiply().")(npalgs._make_dfunc(numpy.multiply))
multiply.__doc__ = "Element by element multiplication. Both elements can be single values or arrays. Same as *."

add = deprecated(version="9.6", message="Use np.add() instead of algs.add().")(npalgs._make_dfunc(numpy.add))
add.__doc__ = "Element by element addition. Both elements can be single values or arrays. Same as +."

subtract = deprecated(version="9.6", message="Use np.substract() instead of algs.substract().")(npalgs._make_dfunc(numpy.subtract))
subtract.__doc__ = "Returns the difference of two values element-wise. Same as x - y."

mod = deprecated(version="9.6", message="Use np.mod() instead of algs.mod().")(npalgs._make_dfunc(numpy.mod))
mod.__doc__ = "Computes x1 - floor(x1 / x2) * x2, the result has the same sign as the divisor x2. It is equivalent to the Python modulus operator x1 % x2. Same as remainder."

remainder = deprecated(version="9.6", message="Use np.remainder() instead of algs.remainder().")(npalgs._make_dfunc(numpy.remainder))
remainder.__doc__ = "Computes x1 - floor(x1 / x2) * x2, the result has the same sign as the divisor x2. It is equivalent to the Python modulus operator x1 % x2. Same as mod."

power = deprecated(version="9.6", message="Use np.power() instead of algs.power().")(npalgs._make_dfunc(numpy.power))
power.__doc__ = "First array elements raised to powers from second array, element-wise."

hypot = npalgs._make_dfunc(numpy.hypot)
hypot.__doc__ = "Given the 'legs' of a right triangle, return its hypotenuse."

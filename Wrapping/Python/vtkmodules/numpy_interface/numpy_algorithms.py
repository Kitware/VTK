"""
This internal module provides a number of algorithms designed to be
compatible with and used by numpy. In particular, some algorithms
required to be adapted to handle VTKCompositeDataArray. Note that
the decorated functions (by _override_numpy) are not meant to be
called directly from here but directly through the Numpy API.
"""
import numpy

from . import dataset_adapter as dsa
from . import internal_algorithms as algs

def _apply_func2(func, array, args):
    """Apply a function to each member of a VTKCompositeDataArray.
    Returns a list of arrays.

    Note that this function is mainly for internal use by this module."""
    if array is dsa.NoneArray:
        return []
    res = []
    for a in array.Arrays:
        if a is dsa.NoneArray:
            res.append(dsa.NoneArray)
        else:
            res.append(func(a, *args))
    return res

def apply_ufunc(func, array, args=()):
    """Apply a function to each member of a VTKCompositeDataArray.
    VTKArray and numpy arrays are also supported."""
    if array is dsa.NoneArray:
        return dsa.NoneArray
    elif type(array) == dsa.VTKCompositeDataArray:
        return dsa.VTKCompositeDataArray(_apply_func2(func, array, args), dataset = array.DataSet)
    else:
        return func(array, *args)

def _make_ufunc(ufunc):
    """ Given a ufunc, creates a closure that applies it to each member
    of a VTKCompositeDataArray.

    Note that this function is mainly for internal use by this module."""
    def new_ufunc(array, *args):
        return apply_ufunc(ufunc, array, args)
    return new_ufunc

def apply_dfunc(dfunc, array1, val2):
    """Apply a two argument function to each member of a VTKCompositeDataArray
    and another argument The second argument can be a VTKCompositeDataArray, in
    which case a one-to-one match between arrays is assumed. Otherwise, the
    function is applied to the composite array with the second argument repeated.
    VTKArray and numpy arrays are also supported."""
    if type(array1) == dsa.VTKCompositeDataArray and type(val2) == dsa.VTKCompositeDataArray:
        res = []
        for a1, a2 in zip(array1.Arrays, val2.Arrays):
            if a1 is dsa.NoneArray or a2 is dsa.NoneArray:
                res.append(dsa.NoneArray)
            else:
                l = dsa.reshape_append_ones(a1, a2)
                res.append(dfunc(l[0], l[1]))
        return dsa.VTKCompositeDataArray(res, dataset = array1.DataSet)
    elif type(array1) == dsa.VTKCompositeDataArray:
        res = []
        for a in array1.Arrays :
            if a is dsa.NoneArray:
                res.append(dsa.NoneArray)
            else:
                l = dsa.reshape_append_ones(a, val2)
                res.append(dfunc(l[0], l[1]))
        return dsa.VTKCompositeDataArray(res, dataset = array1.DataSet)
    elif array1 is dsa.NoneArray:
        return dsa.NoneArray
    elif isinstance(val2, numpy.ndarray):
        return dfunc(array1, val2)
    else:
        l = dsa.reshape_append_ones(array1, val2)
        return dfunc(l[0], l[1])

def _make_dfunc(dfunc):
    """ Given a function that requires two arguments, creates a closure that
    applies it to each member of a VTKCompositeDataArray.

    Note that this function is mainly for internal use by this module."""
    def new_dfunc(array1, val2):
        return apply_dfunc(dfunc, array1, val2)
    return new_dfunc

def _make_dsfunc(dsfunc):
    """ Given a function that requires two arguments (one array, one dataset),
    creates a closure that applies it to each member of a VTKCompositeDataArray.
    Note that this function is mainly for internal use by this module."""
    def new_dsfunc(array, ds=None):
        if type(array) == dsa.VTKCompositeDataArray:
            res = []
            for a in array.Arrays:
                if a is dsa.NoneArray:
                    res.append(dsa.NoneArray)
                else:
                    res.append(dsfunc(a, ds))
            return dsa.VTKCompositeDataArray(res, dataset = array.DataSet)
        elif array is dsa.NoneArray:
            return dsa.NoneArray
        else:
            return dsfunc(array, ds)
    return new_dsfunc

def _make_dsfunc2(dsfunc):
    """ Given a function that requires a dataset, creates a closure that
    applies it to each member of a VTKCompositeDataArray.

    Note that this function is mainly for internal use by this module."""
    def new_dsfunc2(ds):
        if type(ds) == dsa.CompositeDataSet:
            res = []
            for dataset in ds:
                res.append(dsfunc(dataset))
            return dsa.VTKCompositeDataArray(res, dataset = ds)
        else:
            return dsfunc(ds)
    return new_dsfunc2

@dsa._override_numpy(numpy.bitwise_or)
def bitwise_or(array1, array2):
    """Implements element by element or (bitwise, | in C/C++) operation.
    If one of the arrays is a NoneArray, this will return the array
    that is not NoneArray, treating NoneArray as 0 in the or operation."""
    if type(array1) == dsa.VTKCompositeDataArray and type(array2) == dsa.VTKCompositeDataArray:
        res = []
        for a1, a2 in zip(array1.Arrays, array2.Arrays):
            l = dsa.reshape_append_ones(a1, a2)
            res.append(bitwise_or(l[0], l[1]))
        return dsa.VTKCompositeDataArray(res, dataset = array1.DataSet)
    elif type(array1) == dsa.VTKCompositeDataArray:
        res = []
        for a in array1.Arrays :
            l = dsa.reshape_append_ones(a, array2)
            res.append(bitwise_or(l[0], l[1]))
        return dsa.VTKCompositeDataArray(res, dataset = array1.DataSet)
    elif array1 is dsa.NoneArray:
        return array2
    elif array2 is dsa.NoneArray:
        return array1
    else:
        l = dsa.reshape_append_ones(array1, array2)
        return numpy.bitwise_or(l[0], l[1])

@dsa._override_numpy(numpy.sum)
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
    from .algorithms import global_func
    class SumImpl:
        def op(self):
            return algs.sum

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.SUM

        def serial_composite(self, array, axis):
            res = None
            arrays = array.Arrays
            for a in arrays:
                if a is not dsa.NoneArray:
                    if res is None:
                        res = algs.sum(a, axis).astype(numpy.float64)
                    else:
                        res += algs.sum(a, axis)
            return res

        def default(self):
            return numpy.float64(0)

    return global_func(SumImpl(), array, axis, controller)

@dsa._override_numpy(numpy.max)
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
    from .algorithms import global_func
    class MaxImpl:
        def op(self):
            return algs.max

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.MAX

        def serial_composite(self, array, axis):
            res = _apply_func2(algs.max, array, (axis,))
            clean_list = []
            for a in res:
                if a is not dsa.NoneArray:
                    clean_list.append(a)
            if clean_list is []:
                return None
            return algs.max(clean_list, axis=0).astype(numpy.float64)

        def default(self):
            return numpy.finfo(numpy.float64).min

    return global_func(MaxImpl(), array, axis, controller)

@dsa._override_numpy(numpy.min)
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
    from .algorithms import global_func
    class MinImpl:
        def op(self):
            return algs.min

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.MIN

        def serial_composite(self, array, axis):
            res = _apply_func2(algs.min, array, (axis,))
            clean_list = []
            for a in res:
                if a is not dsa.NoneArray:
                    clean_list.append(a)
            if clean_list is []:
                return None
            return algs.min(clean_list, axis=0).astype(numpy.float64)

        def default(self):
            return numpy.finfo(numpy.float64).max

    return global_func(MinImpl(), array, axis, controller)

@dsa._override_numpy(numpy.all)
def all(array, axis=None, controller=None):
    """Returns True if all values of an array evaluate to True, returns
    False otherwise.
    This is useful to check if all values of an array match a certain
    condition such as:

    algorithms.all(array > 5)
    """
    from .algorithms import global_func
    class AllImpl:
        def op(self):
            return algs.all

        def mpi_op(self):
            from mpi4py import MPI
            return MPI.LAND

        def serial_composite(self, array, axis):
            res = _apply_func2(algs.all, array, (axis,))
            clean_list = []
            for a in res:
                if a is not dsa.NoneArray:
                    clean_list.append(a)
            if clean_list is []:
                return None
            return algs.all(clean_list, axis=0)

        def default(self, max_comps):
            return numpy.ones(max_comps, dtype=bool)

    return global_func(AllImpl(), array, axis, controller)

@dsa._override_numpy(numpy.mean)
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
    from .algorithms import array_count
    if axis is None or axis == 0:
        if size is None:
            size = array_count(array, axis, controller)
        return sum(array, axis, controller) / size
    else:
        if type(array) == dsa.VTKCompositeDataArray:
            return apply_ufunc(algs.mean, array, (axis,))
        else:
            return algs.mean(array, axis)

@dsa._override_numpy(numpy.var)
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
    from .algorithms import array_count
    if axis is None or axis == 0:
        size = array_count(array, axis, controller)
        tmp = array - mean(array, axis, controller, size)
        return sum(tmp*tmp, axis, controller) / size
    else:
        if type(array) == dsa.VTKCompositeDataArray:
            return apply_ufunc(algs.var, array, (axis,))
        else:
            return algs.var(array, axis)

@dsa._override_numpy(numpy.std)
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
    from .algorithms import sqrt
    return sqrt(var(array, axis, controller))

@dsa._override_numpy(numpy.shape)
def shape(array):
    "Returns the shape (dimensions) of an array."
    if type(array) == dsa.VTKCompositeDataArray:
        shp = None
        for a in array.Arrays:
            if a is not dsa.NoneArray:
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
    elif array is dsa.NoneArray:
        return ()
    else:
        return numpy.shape(array)

@dsa._override_numpy(numpy.flatnonzero)
def flatnonzero(array):
    """
    Return indices that are non-zero in the flattened version of the input array.
    """
    if array is dsa.NoneArray:
        return dsa.NoneArray
    elif type(array) == dsa.VTKCompositeDataArray:
        res = []
        offset = 0
        for arr in array.Arrays:
            if arr is dsa.NoneArray:
                res.append(dsa.NoneArray)
            else:
                res.append(numpy.flatnonzero(arr) + offset)
                offset += arr.size
        return dsa.VTKCompositeDataArray(res, dataset = array.DataSet)
    else:
        return numpy.flatnonzero(array)

@dsa._override_numpy(numpy.nonzero)
def nonzero(array):
    """
    Return the indices of the non-zero elements of the input array.
    """
    if array is dsa.NoneArray:
        return dsa.NoneArray
    elif type(array) == dsa.VTKCompositeDataArray:
        res = []
        offset = 0
        for arr in array.Arrays:
            if arr is not dsa.NoneArray:
                nz_chunk = numpy.nonzero(arr)
                if len(res) == 0:
                    res = [numpy.array([])] * len(nz_chunk)
                for i in range(len(nz_chunk)):
                    res[i] = numpy.concatenate((res[i], nz_chunk[i] + (offset if i == 0 else 0)))
                offset += arr.shape[0]
        return tuple(numpy.asarray(coord, dtype=int) for coord in res)
    else:
        return numpy.nonzero(array)

@dsa._override_numpy(numpy.where)
def where(*args):
    """Returns the location (indices) of an array where the given
    expression is true. For scalars, it returns a single array of indices.
    For vectors and matrices, it returns two arrays: first with tuple indices,
    second with component indices. The output of this method can be used to
    extract the values from the array also by using it as the index of the [] operator.

    For example:

    >>> np.where(np.array([1,2,3]) == 2)
    (array([1]),)

    >>> np.where(np.array([[1,2,3], [2,1,1]]) == 2)
    (array([0, 1]), array([1, 0]))

    >>> a = array([[1,2,3], [2,1,1]])
    >>> indices = np.where(a > 2)
    >>> a[indices]
    array([3])
    """
    if len(args) == 1:
        return numpy.nonzero(args[0])
    else:
        return NotImplementedError("Support for 3 arguments where has not been implemented yet.")

in1d = (dsa._override_numpy(numpy.in1d)(_make_ufunc(numpy.in1d)))
in1d.__doc__ = "Test whether each element of a 1-D array is also present in a second array."

isin = (dsa._override_numpy(numpy.isin)(_make_ufunc(numpy.isin)))
isin.__doc__ = "Test whether each element of a 1-D array is also present in a second array."

expand_dims = dsa._override_numpy(numpy.expand_dims)(_make_dfunc(numpy.expand_dims))
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

>>> v / np.expand_dims(algs.mag(v), 1)
VTKArray([[ 0.57735027,  0.57735027,  0.57735027],
       [ 0.57735027,  0.57735027,  0.57735027],
       [ 0.57735027,  0.57735027,  0.57735027],
       [ 0.57735027,  0.57735027,  0.57735027],
       [ 0.57735027,  0.57735027,  0.57735027]])"""

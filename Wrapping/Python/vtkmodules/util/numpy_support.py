"""This module adds support to easily import and export NumPy
(http://numpy.scipy.org) arrays into/out of VTK arrays.  The code is
loosely based on TVTK (https://svn.enthought.com/enthought/wiki/TVTK).

This code depends on an addition to the VTK data arrays made by Berk
Geveci to make it support Python's buffer protocol (on Feb. 15, 2008).

The main functionality of this module is provided by the two functions:
    numpy_to_vtk,
    vtk_to_numpy.


Caveats:
--------

 - Bit arrays in general do not have a numpy equivalent and are not
   supported.  Char arrays are also not easy to handle and might not
   work as you expect.  Patches welcome.

 - You need to make sure you hold a reference to a Numpy array you want
   to import into VTK.  If not you'll get a segfault (in the best case).
   The same holds in reverse when you convert a VTK array to a numpy
   array -- don't delete the VTK array.


Created by Prabhu Ramachandran in Feb. 2008.
"""

from vtkmodules.vtkCommonCore import (
    vtkDataArray,
    VTK_BIT,
    VTK_CHAR,
    VTK_SIGNED_CHAR,
    VTK_UNSIGNED_CHAR,
    VTK_SHORT,
    VTK_UNSIGNED_SHORT,
    VTK_INT,
    VTK_UNSIGNED_INT,
    VTK_LONG,
    VTK_UNSIGNED_LONG,
    VTK_LONG_LONG,
    VTK_UNSIGNED_LONG_LONG,
    VTK_ID_TYPE,
    VTK_ID_TYPE_IMPL,
    VTK_FLOAT,
    VTK_DOUBLE,
)
import numpy

# This is a mapping from numpy array types to VTK array types.
_np_vtk = {
    'B' : VTK_UNSIGNED_CHAR,
    'b' : VTK_SIGNED_CHAR,
    'H' : VTK_UNSIGNED_SHORT,
    'h' : VTK_SHORT,
    'I' : VTK_UNSIGNED_INT,
    'i' : VTK_INT,
    'L' : VTK_UNSIGNED_LONG,
    'l' : VTK_LONG,
    'Q' : VTK_UNSIGNED_LONG_LONG,
    'q' : VTK_LONG_LONG,
    'F' : VTK_FLOAT, # complex64
    'f' : VTK_FLOAT,
    'D' : VTK_DOUBLE, # complex128
    'd' : VTK_DOUBLE,
}

# This is a mapping from VTK array types to numpy types
_vtk_np = { v : numpy.dtype(c).type for c,v in _np_vtk.items() }
_vtk_np[VTK_CHAR] = _vtk_np[VTK_SIGNED_CHAR]
_vtk_np[VTK_BIT] = _vtk_np[VTK_UNSIGNED_CHAR]
_vtk_np[VTK_ID_TYPE] = _vtk_np[VTK_ID_TYPE_IMPL]

def get_vtk_array_type(numpy_array_type):
    """Returns a VTK typecode given a numpy array."""
    if not isinstance(numpy_array_type, numpy.dtype):
        numpy_array_type = numpy.dtype(numpy_array_type)
    try:
        return _np_vtk[numpy_array_type.char]
    except KeyError:
        for key, vtk_type in _np_vtk.items():
            if numpy.issubdtype(numpy_array_type, key):
                return vtk_type
    raise TypeError(
        'Could not find a suitable VTK type for %s' % (str(numpy_array_type)))

def get_vtk_to_numpy_typemap():
    """Returns the VTK array type to numpy array type mapping."""
    return _vtk_np

def get_numpy_array_type(vtk_array_type):
    """Returns a numpy array typecode given a VTK array type."""
    return _vtk_np[vtk_array_type]

def create_vtk_array(vtk_arr_type):
    """Internal function used to create a VTK data array from another
    VTK array given the VTK array type.
    """
    return vtkDataArray.CreateDataArray(vtk_arr_type)

def numpy_to_vtk(num_array, deep=0, array_type=None):
    """Converts a real numpy Array to a VTK array object.

    This function only works for real arrays.
    Complex arrays are NOT handled.  It also works for multi-component
    arrays.  However, only 1, and 2 dimensional arrays are supported.
    This function is very efficient, so large arrays should not be a
    problem.

    If the second argument is set to 1, the array is deep-copied from
    from numpy. This is not as efficient as the default behavior
    (shallow copy) and uses more memory but detaches the two arrays
    such that the numpy array can be released.

    WARNING: You must maintain a reference to the passed numpy array, if
    the numpy data is gc'd and VTK will point to garbage which will in
    the best case give you a segfault.

    Parameters:

    num_array
      a 1D or 2D, real numpy array.

    """

    z = numpy.asarray(num_array)
    if not z.flags.contiguous:
        z = numpy.ascontiguousarray(z)

    shape = z.shape
    assert z.flags.contiguous, 'Only contiguous arrays are supported.'
    assert len(shape) < 3, \
           "Only arrays of dimensionality 2 or lower are allowed!"
    assert not numpy.issubdtype(z.dtype, numpy.dtype(complex).type), \
           "Complex numpy arrays cannot be converted to vtk arrays."\
           "Use real() or imag() to get a component of the array before"\
           " passing it to vtk."

    # First create an array of the right type by using the typecode.
    if array_type:
        vtk_typecode = array_type
    else:
        vtk_typecode = get_vtk_array_type(z.dtype)
    result_array = create_vtk_array(vtk_typecode)

    # Fixup shape in case its empty or scalar.
    try:
        testVar = shape[0]
    except:
        shape = (0,)

    # Find the shape and set number of components.
    if len(shape) == 1:
        result_array.SetNumberOfComponents(1)
    else:
        result_array.SetNumberOfComponents(shape[1])

    # We don't need to call result_array.SetNumberOfTuples(shape[0])
    # because we will use result_array.SetVoidPointer
    # which takes care of setting the NumberOfTuples
    # Calling SetNumberOfTuples will result in a memory allocation
    # that will be deleted on SetVoidPointer.

    # Ravel the array appropriately.
    arr_dtype = get_numpy_array_type(vtk_typecode)
    if numpy.issubdtype(z.dtype, arr_dtype) or \
       z.dtype == numpy.dtype(arr_dtype):
        z_flat = numpy.ravel(z)
    else:
        z_flat = numpy.ravel(z).astype(arr_dtype)
        # z_flat is now a standalone object with no references from the caller.
        # As such, it will drop out of this scope and cause memory issues if we
        # do not deep copy its data.
        deep = 1

    # Point the VTK array to the numpy data.  The last argument (1)
    # tells the array not to deallocate.
    result_array.SetVoidArray(z_flat, len(z_flat), 1)
    if deep:
        copy = result_array.NewInstance()
        copy.DeepCopy(result_array)
        result_array = copy
    else:
        # Store the numpy reference on the buffer rather than the array.
        # This ensures the numpy array stays alive as long as the buffer exists,
        # which is important when the buffer is shared via buffer protocol or
        # when the array reallocates (copy-on-reallocate creates a new buffer).
        result_array.GetBuffer()._numpy_reference = z
    return result_array

def numpy_to_vtkIdTypeArray(num_array, deep=0):
    """Converts a numpy array into a vtkIdTypeArray.

    ValueError is raised if the numpy array type is incompatible with vtkIdType.
    """
    expected_dtype = numpy.dtype(_vtk_np[VTK_ID_TYPE])
    if num_array.dtype == expected_dtype:
        return numpy_to_vtk(num_array, deep, VTK_ID_TYPE)
    raise ValueError(f"Expecting a {expected_dtype} array, got {num_array.dtype} instead.")

def vtk_to_numpy(vtk_array):
    """Converts a VTK data array to a numpy array.

    Given a subclass of vtkDataArray, this function returns an
    appropriate numpy array containing the same data -- it actually
    points to the same data.

    Parameters

    vtk_array
      The VTK data array to be converted.

    """
    typ = vtk_array.GetDataType()
    assert typ in get_vtk_to_numpy_typemap().keys(), \
           "Unsupported array type %s"%typ

    shape = vtk_array.GetNumberOfTuples(), \
            vtk_array.GetNumberOfComponents()

    # Get the data via the buffer interface
    dtype = get_numpy_array_type(typ)

    # Implicit VTK arrays (vtkImplicitArray<BackendT>) compute values on
    # the fly and don't expose the C-level buffer protocol (tp_as_buffer).
    # Python 3.12+ added a __buffer__ dunder (PEP 688) that our mixin
    # implements, but on Python < 3.12 numpy.frombuffer cannot reach it.
    # Use __array__ when available — it works on all Python versions and
    # handles both implicit and explicit arrays correctly.
    if hasattr(vtk_array, '__array__'):
        result = numpy.asarray(vtk_array)
        if shape[1] == 1:
            shape = (shape[0], )
        return result.reshape(shape)

    try:
        if typ != VTK_BIT:
            result = numpy.frombuffer(vtk_array, dtype=dtype)
        else:
            result = numpy.unpackbits(vtk_array, count=shape[0])
    except ValueError:
        # http://mail.scipy.org/pipermail/numpy-tickets/2011-August/005859.html
        # numpy 1.5.1 (and maybe earlier) has a bug where if frombuffer is
        # called with an empty buffer, it throws ValueError exception. This
        # handles that issue.
        if shape[0] == 0:
            # create an empty array with the given shape.
            return numpy.empty(shape, dtype=dtype)
        # Implicit arrays (vtkImplicitArray<BackendT>) compute their values
        # on the fly and do not expose the buffer protocol. Materialize to
        # an AOS copy and try again.
        aos = vtk_array.ToAOSDataArray()
        if aos is None or aos is vtk_array:
            raise
        if typ != vtkConstants.VTK_BIT:
            result = numpy.frombuffer(aos, dtype=dtype)
        else:
            result = numpy.unpackbits(aos, count=shape[0])
    if shape[1] == 1:
        shape = (shape[0], )
    return result.reshape(shape)

def vtk_soa_to_numpy(vtk_array):
    """Convert a vtkSOADataArrayTemplate in SOA mode to per-component numpy arrays.

    Returns a list of 1-D numpy arrays (one per component), each
    zero-copy sharing memory with the underlying VTK buffer.

    Parameters
    ----------
    vtk_array : vtkSOADataArrayTemplate or vtkScaledSOADataArrayTemplate

    Returns
    -------
    list of numpy.ndarray
        One 1-D array per component, each of length GetNumberOfTuples().

    Raises
    ------
    TypeError
        If the array does not support per-component buffer access.
    """
    if not hasattr(vtk_array, 'GetComponentBuffer'):
        raise TypeError(
            "vtk_soa_to_numpy requires a vtkSOADataArrayTemplate or "
            "vtkScaledSOADataArrayTemplate, "
            f"got {type(vtk_array).__name__}")

    n_comps = vtk_array.GetNumberOfComponents()
    # GetComponentBuffer returns a vtkAbstractBuffer which supports the Python
    # buffer protocol, so we can use numpy.asarray directly for zero-copy.
    return [numpy.asarray(vtk_array.GetComponentBuffer(c))
            for c in range(n_comps)]

def numpy_to_vtk_soa(arrays, name=""):
    """Create a vtkSOADataArrayTemplate from a list of per-component numpy arrays.

    Each element of *arrays* must be a contiguous 1-D numpy array of the same
    dtype and length.  The VTK array will share memory with the numpy arrays
    (zero-copy), so the caller must keep the numpy arrays alive.

    Parameters
    ----------
    arrays : list of numpy.ndarray
        One 1-D array per component.
    name : str, optional
        Name to assign to the resulting VTK array.

    Returns
    -------
    vtkSOADataArrayTemplate
        A VTK SOA array backed by the provided numpy buffers.
    """
    from vtkmodules.vtkCommonCore import vtkSOADataArrayTemplate

    if not arrays:
        raise ValueError("arrays must be a non-empty list of numpy arrays")

    n_tuples = len(arrays[0])
    dtype = arrays[0].dtype
    for i, a in enumerate(arrays):
        if len(a) != n_tuples:
            raise ValueError(
                f"All component arrays must have the same length; "
                f"component 0 has {n_tuples}, component {i} has {len(a)}")
        if a.dtype != dtype:
            raise ValueError(
                f"All component arrays must have the same dtype; "
                f"component 0 is {dtype}, component {i} is {a.dtype}")

    vtk_array = vtkSOADataArrayTemplate[dtype.type]()
    vtk_array.SetNumberOfComponents(len(arrays))
    vtk_array.SetNumberOfTuples(n_tuples)

    # Ensure contiguity and keep references to the actual buffers passed to VTK
    contiguous_arrays = [numpy.ascontiguousarray(arr) for arr in arrays]
    for comp, arr in enumerate(contiguous_arrays):
        vtk_array.SetArray(comp, arr, n_tuples, True, True)

    if name:
        vtk_array.SetName(name)

    # Store references on individual component buffers for memory safety.
    # This ensures numpy arrays stay alive even if the VTK array reallocates
    # (copy-on-reallocate pattern creates new buffers but old ones stay valid).
    for comp, arr in enumerate(contiguous_arrays):
        buf = vtk_array.GetComponentBuffer(comp)
        if buf is not None:
            buf._numpy_reference = arr

    # Keep array-level reference for backward compatibility
    vtk_array._numpy_refs = contiguous_arrays

    return vtk_array

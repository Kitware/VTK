"""VTKNoneArray — numpy-compatible void array for parallel composite operations.

VTKNoneArray is used to represent a "void" array.  An instance
of this class (NoneArray) is returned instead of None when an
array that doesn't exist in a DataSetAttributes is requested.
All operations on the NoneArray return NoneArray.  The main reason
for this is to support operations in parallel where one of the
processes may be working on an empty dataset.  In such cases,
the process is still expected to evaluate a whole expression because
some of the functions may perform bulk MPI communication.  None
cannot be used in these instances because it cannot properly override
operators such as __add__, __sub__ etc.

The singleton instance is ``NoneArray``, importable as::

    from vtkmodules.numpy_interface.vtk_none_array import NoneArray
"""
import numpy


class VTKNoneArray:
    """Singleton void array — every operation returns itself."""

    # ---- singleton ----------------------------------------------------------
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
        return cls._instance

    # ---- core properties (match other array classes) ------------------------
    @property
    def dataset(self):
        return None

    @dataset.setter
    def dataset(self, value):
        pass

    @property
    def association(self):
        return None

    @association.setter
    def association(self, value):
        pass

    @property
    def shape(self):
        return (0,)

    @property
    def ndim(self):
        return 1

    @property
    def size(self):
        return 0

    @property
    def dtype(self):
        return numpy.dtype(numpy.float64)

    @property
    def nbytes(self):
        return 0

    @property
    def T(self):
        return self

    @property
    def value(self):
        return None

    # ---- numpy protocol -----------------------------------------------------
    def __array__(self, dtype=None, copy=None):
        return numpy.empty(0, dtype=dtype or numpy.float64)

    def __buffer__(self, flags):
        return memoryview(numpy.empty(0, dtype=numpy.float64))

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        return self

    def __array_function__(self, func, types, args, kwargs):
        return self

    # ---- indexing -----------------------------------------------------------
    def __getitem__(self, key):
        return self

    def __setitem__(self, key, value):
        pass

    # ---- operators (all return self = NoneArray) ----------------------------
    # arithmetic
    def __add__(self, other):       return self
    def __radd__(self, other):      return self
    def __sub__(self, other):       return self
    def __rsub__(self, other):      return self
    def __mul__(self, other):       return self
    def __rmul__(self, other):      return self
    def __truediv__(self, other):   return self
    def __rtruediv__(self, other):  return self
    def __floordiv__(self, other):  return self
    def __rfloordiv__(self, other): return self
    def __pow__(self, other):       return self
    def __rpow__(self, other):      return self
    def __mod__(self, other):       return self
    def __rmod__(self, other):      return self
    def __lshift__(self, other):    return self
    def __rlshift__(self, other):   return self
    def __rshift__(self, other):    return self
    def __rrshift__(self, other):   return self
    def __and__(self, other):       return self
    def __rand__(self, other):      return self
    def __or__(self, other):        return self
    def __ror__(self, other):       return self
    def __xor__(self, other):       return self
    def __rxor__(self, other):      return self

    # comparison
    def __lt__(self, other):  return self
    def __le__(self, other):  return self
    def __eq__(self, other):  return self is other
    def __ne__(self, other):  return self is not other
    def __ge__(self, other):  return self
    def __gt__(self, other):  return self

    # unary
    def __neg__(self):   return self
    def __pos__(self):   return self
    def __abs__(self):   return self
    def __invert__(self): return self

    # ---- reductions (all return self) ---------------------------------------
    def sum(self, axis=None, **kwargs):      return self
    def mean(self, axis=None, **kwargs):     return self
    def min(self, axis=None, **kwargs):      return self
    def max(self, axis=None, **kwargs):      return self
    def std(self, axis=None, **kwargs):      return self
    def var(self, axis=None, **kwargs):      return self
    def any(self, axis=None, **kwargs):      return self
    def all(self, axis=None, **kwargs):      return self
    def prod(self, axis=None, **kwargs):     return self
    def argmin(self, axis=None, **kwargs):   return self
    def argmax(self, axis=None, **kwargs):   return self
    def cumsum(self, axis=None, **kwargs):   return self
    def cumprod(self, axis=None, **kwargs):  return self

    # ---- shape / layout methods (all return self) ---------------------------
    def astype(self, dtype):           return self
    def reshape(self, *args, **kw):    return self
    def flatten(self, order='C'):      return self
    def ravel(self, order='C'):        return self
    def copy(self, order='C'):         return self
    def squeeze(self, axis=None):      return self
    def transpose(self, *axes):        return self
    def tolist(self):                  return []
    def clip(self, *args, **kw):       return self
    def round(self, decimals=0, **kw): return self
    def sort(self, axis=0, **kw):      return self
    def dot(self, other):              return self

    # ---- container protocol -------------------------------------------------
    def __len__(self):
        return 0

    def __iter__(self):
        return iter([])

    def __contains__(self, item):
        return False

    def __bool__(self):
        return False

    def __repr__(self):
        return "NoneArray"

    def __str__(self):
        return "NoneArray"

    # ---- hash (needed since we define __eq__) --------------------------------
    def __hash__(self):
        return id(self)


NoneArray = VTKNoneArray()

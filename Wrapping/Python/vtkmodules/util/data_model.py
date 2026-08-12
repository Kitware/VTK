"""This module provides classes that allow numpy style access
to VTK datasets. See examples at bottom.
"""

import sys
from contextlib import suppress
from vtkmodules.vtkCommonCore import vtkPoints, vtkAbstractArray, vtkDataArray
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkDataObject,
    vtkFieldData,
    vtkDataSetAttributes,
    vtkPointData,
    vtkCellData,
    vtkDataObject,
    vtkImageData,
    vtkMultiBlockDataSet,
    vtkPolyData,
    vtkStructuredGrid,
    vtkRectilinearGrid,
    vtkUnstructuredGrid,
    vtkOverlappingAMR,
    vtkPartitionedDataSet,
    vtkPartitionedDataSetCollection,
)

import weakref

NUMPY_AVAILABLE = False

with suppress(ImportError):
    import numpy
    from vtkmodules.util import numpy_support

    NUMPY_AVAILABLE = True


class _NumpyInterface:
    """Lazily resolved handles into ``vtkmodules.numpy_interface``.

    These cannot be imported at module scope.  This module is imported by
    vtkmodules' override hook while ``vtkCommonDataModel`` is still
    initializing, and ``vtkmodules.numpy_interface.utils`` imports
    ``vtkCommonDataModel`` itself.  So whenever something under
    ``numpy_interface`` is the first thing to pull in
    ``vtkCommonDataModel`` -- ``import vtkmodules.util.functions`` is the
    common way -- that module is only half initialized by the time we get
    here, and importing from it raises ImportError.

    That ImportError used to be swallowed along with the optional numpy
    dependency, silently leaving ``NUMPY_AVAILABLE`` False for the rest of
    the process.  Array assignments then became no-ops and lookups returned
    None, with no diagnostic.  Resolving on first use instead means the
    import cycle has unwound by the time these names are needed.
    """

    __slots__ = ()

    _resolved = False
    NoneArray = None
    VTKPartitionedArray = None
    VTKPartitionedPoints = None

    @classmethod
    def get(cls):
        if not cls._resolved:
            from vtkmodules.numpy_interface.utils import NoneArray
            from vtkmodules.numpy_interface.vtk_partitioned_array import (
                VTKPartitionedArray,
                VTKPartitionedPoints,
            )

            cls.NoneArray = NoneArray
            cls.VTKPartitionedArray = VTKPartitionedArray
            cls.VTKPartitionedPoints = VTKPartitionedPoints
            cls._resolved = True
        return cls


def __getattr__(name):
    """Keep ``data_model.NoneArray`` and friends working for outside callers."""
    if name in ("NoneArray", "VTKPartitionedArray", "VTKPartitionedPoints"):
        if not NUMPY_AVAILABLE:
            raise AttributeError(
                "%s requires numpy, which is not available" % name
            )
        return getattr(_NumpyInterface.get(), name)
    raise AttributeError("module %s has no attribute %r" % (__name__, name))


class FieldDataBase(object):
    """Python-friendly interface for vtkFieldData and its subclasses.

    Provides dict-like access to arrays by name or index, with
    automatic metadata (``dataset``, ``association``) propagation.
    Arrays retrieved via ``[]`` or ``get_array()`` are returned with
    their owning dataset and association already set.

    This class is the base for ``FieldData``, ``DataSetAttributes``,
    ``PointData``, and ``CellData`` overrides.
    """
    def __init__(self, *args):
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip init to avoid clobbering state.
        if args and isinstance(args[0], str):
            return
        self.association = None
        self.dataset = None

    def __getitem__(self, idx):
        """Implements the [] operator. Accepts an array name or index."""
        return self.get_array(idx)

    def __setitem__(self, name, value):
        """Implements the [] operator. Accepts an array name or index."""
        return self.set_array(name, value)

    def get_array(self, idx):
        "Given an index or name, returns a VTK array with metadata set."
        if isinstance(idx, int) and idx >= self.GetNumberOfArrays():
            raise IndexError("array index out of range")
        vtkarray = super().GetArray(idx)

        if not NUMPY_AVAILABLE:
            return vtkarray if vtkarray else self.GetAbstractArray(idx)

        if not vtkarray:
            vtkarray = self.GetAbstractArray(idx)
            if vtkarray:
                return vtkarray
            return _NumpyInterface.get().NoneArray
        # All standard VTK data arrays have mixin overrides applied
        # (VTKAOSArray, VTKSOAArray, VTKConstantArray) so they
        # are already numpy-compatible.  Just set metadata and return.
        if hasattr(vtkarray, '_set_dataset'):
            vtkarray._set_dataset(self.dataset)
            vtkarray._association = self.association
        return vtkarray

    def __contains__(self, aname):
        """Returns true if the container contains arrays
        with the given name, false otherwise"""
        return self.HasArray(aname)

    def keys(self):
        """Returns the names of the arrays as a list."""
        kys = []
        narrays = self.GetNumberOfArrays()
        for i in range(narrays):
            name = self.GetAbstractArray(i).GetName()
            if name:
                kys.append(name)
        return tuple(kys)

    def values(self):
        """Returns the arrays as a tuple."""
        vals = []
        narrays = self.GetNumberOfArrays()
        for i in range(narrays):
            a = self.get_array(i)
            if a.GetName():
                vals.append(a)
        return tuple(vals)

    def items(self):
        """Returns a tuple of pairs (name, array)"""
        pairs = []
        narrays = self.GetNumberOfArrays()
        for i in range(narrays):
            arr = self.get_array(i)
            name = arr.GetName()
            if name:
                pairs.append((name, arr))
        return tuple(pairs)

    def set_array(self, name, narray):
        """Appends a new array to the dataset attributes."""
        if not NUMPY_AVAILABLE:
            if isinstance(narray, vtkAbstractArray):
                narray.SetName(name)
                self.AddArray(narray)
            return

        if narray is _NumpyInterface.get().NoneArray:
            # if NoneArray, nothing to do.
            return

        # VTK data arrays with mixin (VTKAOSArray, VTKSOAArray,
        # VTKConstantArray, VTKAffineArray): shallow copy to avoid
        # mutating the original array's name/dataset/association.
        if isinstance(narray, vtkDataArray) and hasattr(narray, '_set_dataset'):
            arr = narray.NewInstance()
            # NewInstance() on implicit arrays (vtkConstantArray,
            # vtkAffineArray) returns the wrong C++ type.  Fall back
            # to creating a new Python-level instance of the correct
            # override class so that the mixin is preserved.
            if not isinstance(arr, type(narray)):
                arr = type(narray).__new__(type(narray))
            arr.ShallowCopy(narray)
            arr.SetName(name)
            arr._set_dataset(self.dataset)
            arr._association = self.association
            self.AddArray(arr)
            return

        if self.association == vtkDataObject.POINT:
            arrLength = self.dataset.GetNumberOfPoints()
        elif self.association == vtkDataObject.CELL:
            arrLength = self.dataset.GetNumberOfCells()
        elif (
            self.association == vtkDataObject.ROW
            and self.dataset.GetNumberOfColumns() > 0
        ):
            arrLength = self.dataset.GetNumberOfRows()
        else:
            if not isinstance(narray, numpy.ndarray):
                arrLength = 1
            else:
                arrLength = narray.shape[0]

        # Fixup input array length:
        if (
            not isinstance(narray, numpy.ndarray) or numpy.ndim(narray) == 0
        ):  # Scalar input
            dtype = narray.dtype if isinstance(narray, numpy.ndarray) else type(narray)
            tmparray = numpy.empty(arrLength, dtype=dtype)
            tmparray.fill(narray)
            narray = tmparray
        elif narray.shape[0] != arrLength:  # Vector input
            components = 1
            for l in narray.shape:
                components *= l
            tmparray = numpy.empty((arrLength, components), dtype=narray.dtype)
            tmparray[:] = narray.flatten()
            narray = tmparray

        shape = narray.shape

        if len(shape) == 3:
            # Array of matrices. We need to make sure the order  in memory is right.
            # If column order (c order), transpose. VTK wants row order (fortran
            # order). The deep copy later will make sure that the array is contiguous.
            # If row order but not contiguous, transpose so that the deep copy below
            # does not happen.
            size = narray.dtype.itemsize
            if (narray.strides[1] / size == 3 and narray.strides[2] / size == 1) or (
                narray.strides[1] / size == 1
                and narray.strides[2] / size == 3
                and not narray.flags.contiguous
            ):
                narray = narray.transpose(0, 2, 1)

        # If array is not contiguous, make a deep copy that is contiguous
        if not narray.flags.contiguous:
            narray = numpy.ascontiguousarray(narray)

        # Flatten array of matrices to array of vectors
        if len(shape) == 3:
            narray = narray.reshape(shape[0], shape[1] * shape[2])

        # Convert numpy array to a VTK AOS array.  With the mixin override
        # the result is already a VTKAOSArray.
        arr = numpy_support.numpy_to_vtk(narray)
        arr.SetName(name)
        self.AddArray(arr)

    def __eq__(self, other: object) -> bool:
        """Test dict-like equivalency."""
        # here we check if other is the same class or a subclass of self.
        if not isinstance(other, type(self)):
            return False

        if self is other:
            return True

        """
        If numpy is not available, only check for identity without comparing contents of the data arrays
        """
        if not NUMPY_AVAILABLE:
            return False

        if set(self.keys()) != set(other.keys()):
            return False

        # verify the value of the arrays
        for key, value in other.items():
            if not numpy.array_equal(value, self[key]):
                return False

        return True

    def __iter__(self):
        return iter(self.keys())

    def __len__(self):
        return self.GetNumberOfArrays()

    def to_pandas(self):
        """Convert to a :class:`pandas.DataFrame`.

        Single-component arrays become columns directly.
        Multi-component arrays are split into columns named
        ``name_0``, ``name_1``, etc.
        """
        pd = sys.modules.get("pandas", None)
        if pd is None:
            raise RuntimeError("You must import pandas before calling to_pandas().")

        data = {}
        for name, arr in self.items():
            if not isinstance(arr, vtkDataArray):
                continue
            np_arr = numpy.asarray(arr)
            ncomp = arr.GetNumberOfComponents()
            if ncomp == 1 or np_arr.ndim == 1:
                data[name] = np_arr.ravel()
            else:
                for j in range(ncomp):
                    data[f"{name}_{j}"] = np_arr[:, j]
        return pd.DataFrame(data)

    def from_pandas(self, df):
        """Populate from a :class:`pandas.DataFrame`.

        Each column becomes a single-component array.
        Existing arrays are removed first.
        """
        if "pandas" not in sys.modules:
            raise RuntimeError("You must import pandas before calling from_pandas().")

        self.Initialize()
        for name in df.columns:
            self.set_array(str(name), df[name].to_numpy())

    def to_xarray(self):
        """Convert to an :class:`xarray.Dataset`.

        Single-component arrays get dimension ``("index",)``.
        Multi-component arrays get dimensions
        ``("index", "component")``.
        """
        xr = sys.modules.get("xarray", None)
        if xr is None:
            raise RuntimeError("You must import xarray before calling to_xarray().")

        data_vars = {}
        for name, arr in self.items():
            if not isinstance(arr, vtkDataArray):
                continue
            np_arr = numpy.asarray(arr)
            ncomp = arr.GetNumberOfComponents()
            if ncomp == 1 or np_arr.ndim == 1:
                data_vars[name] = ("index", np_arr.ravel())
            else:
                data_vars[name] = (("index", "component"), np_arr)
        return xr.Dataset(data_vars)

    def from_xarray(self, ds):
        """Populate from an :class:`xarray.Dataset`.

        Each variable becomes an array.
        Existing arrays are removed first.
        """
        if "xarray" not in sys.modules:
            raise RuntimeError("You must import xarray before calling from_xarray().")

        self.Initialize()
        for name in ds.data_vars:
            self.set_array(str(name), ds[name].values)

@vtkFieldData.override
class FieldData(FieldDataBase, vtkFieldData):
    pass


class DataSetAttributesBase(FieldDataBase):
    pass


@vtkDataSetAttributes.override
class DataSetAttributes(DataSetAttributesBase, vtkDataSetAttributes):
    def __eq__(self, other: object) -> bool:
        """Test dict-like equivalency."""
        if not super().__eq__(other):
            return False

        for attr in [
            "GetScalars",
            "GetVectors",
            "GetNormals",
            "GetTangents",
            "GetTCoords",
            "GetTensors",
            "GetGlobalIds",
            "GetPedigreeIds",
            "GetRationalWeights",
            "GetHigherOrderDegrees",
            "GetProcessIds",
        ]:
            self_attr = getattr(self, attr)()
            other_attr = getattr(other, attr)()
            if self_attr and other_attr:
                if self_attr.GetName() != other_attr.GetName():
                    return False
            elif self_attr != other_attr:
                return False

        return True


@vtkPointData.override
class PointData(DataSetAttributesBase, vtkPointData):
    pass


@vtkCellData.override
class CellData(DataSetAttributesBase, vtkCellData):
    pass


class CompositeDataSetAttributesIterator(object):
    def __init__(self, cdsa):
        self._cdsa = cdsa
        if cdsa:
            self._itr = iter(cdsa.keys())
        else:
            self._itr = None

    def __iter__(self):
        return self

    def __next__(self):
        if not self._cdsa:
            raise StopIteration

        name = next(self._itr)
        return self._cdsa[name]

    def next(self):
        return self.__next__()


class CompositeDataSetAttributes(object):
    """This is a python friendly wrapper for vtkDataSetAttributes for composite
    datasets. Since composite datasets themselves don't have attribute data, but
    the attribute data is associated with the leaf nodes in the composite
    dataset, this class simulates a DataSetAttributes interface by taking a
    union of DataSetAttributes associated with all leaf nodes."""

    def __init__(self, dataset, association):
        self.dataset = dataset
        self.association = association
        self.array_names = []
        self.arrays = {}

        # build the set of arrays available in the composite dataset. Since
        # composite datasets can have partial arrays, we need to iterate over
        # all non-null blocks in the dataset.
        self.__determine_arraynames()

    def __determine_arraynames(self):
        array_set = set()
        array_list = []
        for dataset in self.dataset:
            dsa = dataset.GetAttributesAsFieldData(self.association)
            for array_name in dsa.keys():
                if array_name not in array_set:
                    array_set.add(array_name)
                    array_list.append(array_name)
        self.array_names = array_list

    def modified(self):
        """Rescans the contained dataset to update the
        internal list of arrays."""
        self.__determine_arraynames()

    def __contains__(self, aname):
        """Returns true if the container contains arrays
        with the given name, false otherwise"""
        return aname in self.array_names

    def keys(self):
        """Returns the names of the arrays as a tuple."""
        return tuple(self.array_names)

    def values(self):
        """Returns all the arrays as a tuple."""
        arrays = []
        for array in self:
            arrays.append(array)
        return tuple(arrays)

    def items(self):
        """Returns (name, array) pairs as a tuple."""
        items = []
        for name in self.keys():
            items.append((name, self[name]))
        return tuple(items)

    def __getitem__(self, idx):
        """Implements the [] operator. Accepts an array name."""
        return self.get_array(idx)

    def __setitem__(self, name, narray):
        """Implements the [] operator. Accepts an array name."""
        return self.set_array(name, narray)

    def set_array(self, name, narray):
        """Appends a new array to the composite dataset attributes."""
        if not NUMPY_AVAILABLE:
            # don't know how to handle composite dataset attribute when numpy not around
            raise NotImplementedError("Only available with numpy")

        npi = _NumpyInterface.get()
        if narray is npi.NoneArray:
            # if NoneArray, nothing to do.
            return

        added = False
        if not isinstance(narray, npi.VTKPartitionedArray):  # Scalar input
            for ds in self.dataset:
                ds.GetAttributesAsFieldData(self.association).set_array(name, narray)
                added = True
            if added:
                self.array_names.append(name)
                # don't add the narray since it's a scalar. GetArray() will create a
                # VTKPartitionedArray on-demand.
        else:
            for ds, array in zip(self.dataset, narray.arrays):
                if array is not None:
                    ds.GetAttributesAsFieldData(self.association).set_array(name, array)
                    added = True
            if added:
                self.array_names.append(name)
                self.arrays[name] = weakref.ref(narray)

    def get_array(self, idx):
        """Given a name, returns a VTKPartitionedArray."""
        arrayname = idx

        if not NUMPY_AVAILABLE:
            # don't know how to handle composite dataset attribute when numpy not around
            raise NotImplementedError("Only available with numpy")

        npi = _NumpyInterface.get()
        if arrayname not in self.array_names:
            return npi.NoneArray
        if arrayname not in self.arrays or self.arrays[arrayname]() is None:
            array = npi.VTKPartitionedArray(
                dataset=self.dataset, name=arrayname, association=self.association
            )
            self.arrays[arrayname] = weakref.ref(array)
        else:
            array = self.arrays[arrayname]()
        return array

    def __iter__(self):
        """Iterators on keys"""
        return iter(self.array_names)

    def __len__(self):
        return len(self.array_names)

class DataSet(object):
    """Python-friendly interface for VTK dataset types.

    Adds ``point_data``, ``cell_data``, and ``field_data`` properties
    that return ``DataSetAttributes`` instances with dict-like access
    to arrays.  Arrays retrieved from these properties carry metadata
    (``dataset``, ``association``) automatically.

    This is the base class for ``PointSet``, ``ImageData``,
    ``RectilinearGrid``, and their subclasses.

    Examples
    --------
    ::

        pd = vtk.vtkPolyData()
        # ... populate pd ...
        velocity = pd.point_data["velocity"]    # VTKAOSArray
        pd.cell_data["pressure"] = numpy_array  # set from numpy
    """
    def __init__(self, *args, **kwargs) -> None:
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip init to avoid clobbering state.
        if args and isinstance(args[0], str):
            return
        self._numpy_attrs = []

    @property
    def point_data(self):
        pd = super().GetPointData()
        pd.dataset = self
        pd.association = self.POINT
        return pd

    @point_data.setter
    def point_data(self, arrays):
        """Set point arrays from a dict of ``{name: array}``.

        Each value can be a numpy array or a VTK data array.
        Existing point arrays are removed first.
        """
        pd = self.point_data
        pd.Initialize()
        for name, arr in arrays.items():
            pd.set_array(name, arr)

    @property
    def cell_data(self):
        cd = super().GetCellData()
        cd.dataset = self
        cd.association = self.CELL
        return cd

    @cell_data.setter
    def cell_data(self, arrays):
        """Set cell arrays from a dict of ``{name: array}``.

        Each value can be a numpy array or a VTK data array.
        Existing cell arrays are removed first.
        """
        cd = self.cell_data
        cd.Initialize()
        for name, arr in arrays.items():
            cd.set_array(name, arr)

    @property
    def field_data(self):
        fd = super().GetFieldData()
        if fd:
            fd.dataset = self
            fd.association = self.FIELD
        return fd

    @field_data.setter
    def field_data(self, arrays):
        """Set field arrays from a dict of ``{name: array}``.

        Each value can be a numpy array or a VTK data array.
        Existing field arrays are removed first.
        """
        fd = self.field_data
        fd.Initialize()
        for name, arr in arrays.items():
            fd.set_array(name, arr)

    @property
    def points(self):
        """Returns the ``vtkPoints`` owning the coordinate array.

        The underlying data array (``points.data``) is annotated with
        ``dataset``/``association`` metadata so it can be passed to
        algorithms that expect numpy-interface arrays.
        """
        pts = super().GetPoints()
        if pts is not None:
            d = pts.GetData()
            if d is not None and hasattr(d, '_set_dataset'):
                d._set_dataset(self)
                d._association = vtkDataObject.POINT
        return pts

    @points.setter
    def points(self, pts):
        self.SetPoints(pts)

    def __eq__(self, other: object) -> bool:
        """Test equivalency between data objects."""
        if not isinstance(self, type(other)):
            return False

        if self is other:
            return True

        """
        If numpy is not available, only check for identity without comparing contents of the data arrays
        """
        if not NUMPY_AVAILABLE:
            return False

        for attr in self._numpy_attrs:
            if hasattr(self, attr):
                if not numpy.array_equal(getattr(self, attr), getattr(other, attr)):
                    return False

        for attr in ["field_data", "point_data", "cell_data"]:
            if getattr(self, attr) != getattr(other, attr):
                return False

        return True

    def convert_to_unstructured_grid(self):
        from vtkmodules.vtkFiltersCore import vtkExtractCells

        ecells = vtkExtractCells()
        ecells.SetInputData(self)
        ecells.ExtractAllCellsOn()
        ecells.Update()
        return ecells.GetOutput()


class PointSet(DataSet):
    """DataSet subclass for point-containing datasets.

    Uses the auto-generated ``points`` wrapper property, which returns
    a ``vtkPoints`` object.  The numpy-indexable coordinate array is
    available as ``dataset.points.data``.  Build a ``vtkPoints`` from
    array-like data with the ``vtkPoints(data=...)`` constructor.
    """
    def __init__(self, *args, **kwargs) -> None:
        DataSet.__init__(self, *args, **kwargs)

    def __eq__(self, other: object) -> bool:
        if not super().__eq__(other):
            return False
        if not NUMPY_AVAILABLE:
            return True
        self_pts = self.points
        other_pts = other.points
        if (self_pts is None) != (other_pts is None):
            return False
        if self_pts is None:
            return True
        return numpy.array_equal(self_pts.data, other_pts.data)


@vtkUnstructuredGrid.override
class UnstructuredGrid(PointSet, vtkUnstructuredGrid):
    """Python-friendly ``vtkUnstructuredGrid`` with a writable ``cells``.

    Assignment takes a ``(cell_type, vtkCellArray)`` tuple where
    ``cell_type`` is either an ``int`` (uniform) or a
    ``vtkUnsignedCharArray`` (per-cell types).
    """
    def __init__(self, *args, **kwargs):
        PointSet.__init__(self, *args, **kwargs)
        vtkUnstructuredGrid.__init__(self, **kwargs)

    @property
    def cells(self):
        return self.GetCells()

    @cells.setter
    def cells(self, value):
        cell_type, ca = value
        self.SetCells(cell_type, ca)


@vtkImageData.override
class ImageData(DataSet, vtkImageData):
    """Python-friendly ``vtkImageData`` with numpy access.

    Adds ``point_data``, ``cell_data``, and ``field_data`` properties
    for dict-like array access.
    """
    def __init__(self, *args, **kwargs):
        DataSet.__init__(self, *args, **kwargs)
        vtkImageData.__init__(self, **kwargs)


@vtkPolyData.override
class PolyData(PointSet, vtkPolyData):
    """Python-friendly ``vtkPolyData`` with numpy access.

    Adds ``point_data``, ``cell_data``, and ``points`` properties.
    """
    def __init__(self, *args, **kwargs) -> None:
        PointSet.__init__(self, *args, **kwargs)
        vtkPolyData.__init__(self, **kwargs)


@vtkRectilinearGrid.override
class RectilinearGrid(DataSet, vtkRectilinearGrid):
    """Python-friendly ``vtkRectilinearGrid`` with numpy access.

    Adds ``x_coordinates``, ``y_coordinates``, and ``z_coordinates``
    properties that can be get/set with numpy arrays or VTK arrays.
    """
    def __init__(self, *args, **kwargs) -> None:
        DataSet.__init__(self, *args, **kwargs)
        vtkRectilinearGrid.__init__(self, **kwargs)
        if args and isinstance(args[0], str):
            return
        self._numpy_attrs.extend(["x_coordinates", "y_coordinates", "z_coordinates"])

    @property
    def x_coordinates(self):
        pts = self.GetXCoordinates()

        if not NUMPY_AVAILABLE:
            return pts

        if not pts:
            return None
        return pts

    @x_coordinates.setter
    def x_coordinates(self, points):
        if isinstance(points, vtkDataArray):
            self.SetXCoordinates(points)
            return

        if not NUMPY_AVAILABLE:
            raise ValueError("Expect vtkDataArray")

        pts = numpy_support.numpy_to_vtk(points)
        pts.SetName("x_coords")
        self.SetXCoordinates(pts)

    @property
    def y_coordinates(self):
        pts = self.GetYCoordinates()

        if not NUMPY_AVAILABLE:
            return pts

        if not pts:
            return None
        return pts

    @y_coordinates.setter
    def y_coordinates(self, points):
        if isinstance(points, vtkDataArray):
            self.SetYCoordinates(points)
            return

        if not NUMPY_AVAILABLE:
            raise ValueError("Expect vtkDataArray")

        pts = numpy_support.numpy_to_vtk(points)
        pts.SetName("y_coords")
        self.SetYCoordinates(pts)

    @property
    def z_coordinates(self):
        pts = self.GetZCoordinates()

        if not NUMPY_AVAILABLE:
            return pts

        if not pts:
            return None
        return pts

    @z_coordinates.setter
    def z_coordinates(self, points):
        if isinstance(points, vtkDataArray):
            self.SetZCoordinates(points)
            return

        if not NUMPY_AVAILABLE:
            raise ValueError("Expect vtkDataArray")

        pts = numpy_support.numpy_to_vtk(points)
        pts.SetName("z_coords")
        self.SetZCoordinates(pts)


class CompositeDataIterator(object):
    """Wrapper for a vtkCompositeDataIterator class to satisfy
    the python iterator protocol. This iterator iterates
    over non-empty leaf nodes. To iterate over empty or
    non-leaf nodes, use the vtkCompositeDataIterator directly.
    """

    def __init__(self, cds):
        self.Iterator = cds.NewIterator()
        if self.Iterator:
            self.Iterator.UnRegister(None)
            self.Iterator.GoToFirstItem()

    def __iter__(self):
        return self

    def __next__(self):
        if not self.Iterator:
            raise StopIteration

        if self.Iterator.IsDoneWithTraversal():
            raise StopIteration
        retVal = self.Iterator.GetCurrentDataObject()
        self.Iterator.GoToNextItem()
        return retVal

    def next(self):
        return self.__next__()

    def __getattr__(self, name):
        """Returns attributes from the vtkCompositeDataIterator."""
        return getattr(self.Iterator, name)


class CompositeDataSetBase(object):
    """Python-friendly interface for composite VTK datasets.

    Provides ``point_data``, ``cell_data``, ``field_data``, and
    ``points`` properties that return ``CompositeDataSetAttributes``
    or ``VTKPartitionedArray`` instances spanning all leaf datasets.
    Iteration yields the non-empty leaf ``vtkDataObject`` instances.

    This is the base class for ``vtkMultiBlockDataSet``,
    ``vtkPartitionedDataSet``, ``vtkPartitionedDataSetCollection``,
    and ``vtkOverlappingAMR`` overrides.
    """

    def __init__(self, *args, **kwargs):
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip init to avoid clobbering state.
        if args and isinstance(args[0], str):
            return
        self._PointData = None
        self._CellData = None
        self._FieldData = None
        self._Points = None

    def __iter__(self):
        "Creates an iterator for the contained datasets."
        return CompositeDataIterator(self)

    def get_attributes(self, type):
        """Returns the attributes specified by the type as a
        CompositeDataSetAttributes instance."""
        return CompositeDataSetAttributes(self, type)

    @property
    def point_data(self):
        "Returns the point data as a DataSetAttributes instance."
        if self._PointData is None or self._PointData() is None:
            pdata = self.get_attributes(vtkDataObject.POINT)
            self._PointData = weakref.ref(pdata)
        return self._PointData()

    @property
    def cell_data(self):
        "Returns the cell data as a DataSetAttributes instance."
        if self._CellData is None or self._CellData() is None:
            cdata = self.get_attributes(vtkDataObject.CELL)
            self._CellData = weakref.ref(cdata)
        return self._CellData()

    @property
    def field_data(self):
        "Returns the field data as a DataSetAttributes instance."
        if self._FieldData is None or self._FieldData() is None:
            fdata = self.get_attributes(vtkDataObject.FIELD)
            self._FieldData = weakref.ref(fdata)
        return self._FieldData()

    @property
    def points(self):
        "Returns the points as a VTKPartitionedPoints instance."
        if not NUMPY_AVAILABLE:
            # don't know how to handle composite dataset when numpy not around
            raise NotImplementedError("Only available with numpy")

        if self._Points is None or self._Points() is None:
            pts = []
            for ds in self:
                try:
                    _pts = ds.points
                except AttributeError:
                    _pts = None
                pts.append(_pts)
            npi = _NumpyInterface.get()
            if len(pts) == 0 or all(p is None for p in pts):
                cpts = npi.NoneArray
            else:
                cpts = npi.VTKPartitionedPoints(pts)
            self._Points = weakref.ref(cpts)
        return self._Points()


@vtkPartitionedDataSet.override
class PartitionedDataSet(CompositeDataSetBase, vtkPartitionedDataSet):
    def append(self, dataset):
        self.SetPartition(self.GetNumberOfPartitions(), dataset)

@vtkPartitionedDataSetCollection.override
class PartitionedDataSetCollection(CompositeDataSetBase, vtkPartitionedDataSetCollection):
    def append(self, dataset):
        self.SetPartitionedDataSet(self.GetNumberOfPartitionedDataSets(), dataset)

@vtkOverlappingAMR.override
class OverlappingAMR(CompositeDataSetBase, vtkOverlappingAMR):
    pass

@vtkMultiBlockDataSet.override
class MultiBlockDataSet(CompositeDataSetBase, vtkMultiBlockDataSet):
    pass

@vtkStructuredGrid.override
class StructuredGrid(PointSet, vtkStructuredGrid):
    """Python-friendly ``vtkStructuredGrid`` with numpy access.

    Adds ``x_coordinates``, ``y_coordinates``, and ``z_coordinates``
    read-only properties that extract coordinate components from the
    point array and reshape them to the grid dimensions.
    """
    def __init__(self, *args, **kwargs):
        PointSet.__init__(self, *args, **kwargs)
        vtkStructuredGrid.__init__(self, **kwargs)

    @property
    def x_coordinates(self):
        if not NUMPY_AVAILABLE:
            raise NotImplementedError("Only available with numpy")

        dims = [0,0,0]
        self.GetDimensions(dims)
        return self.points.data[:, 0].reshape(dims, order="F")

    @property
    def y_coordinates(self):
        if not NUMPY_AVAILABLE:
            raise NotImplementedError("Only available with numpy")

        dims = [0,0,0]
        self.GetDimensions(dims)
        return self.points.data[:, 1].reshape(dims, order="F")

    @property
    def z_coordinates(self):
        if not NUMPY_AVAILABLE:
            raise NotImplementedError("Only available with numpy")
        dims = [0,0,0]
        self.GetDimensions(dims)
        return self.points.data[:, 2].reshape(dims, order="F")


@vtkCellArray.override
class CellArray(vtkCellArray):
    """Python-friendly ``vtkCellArray`` with a convenient constructor.

    Accepts ``offsets`` and ``connectivity`` as keyword arguments::

        ca = vtkCellArray(offsets=[0, 3, 6], connectivity=[0, 1, 2, 3, 4, 5])

    Both can be numpy arrays, lists, or ``vtkDataArray`` objects.
    """

    def __init__(self, *args, **kwargs):
        if args and isinstance(args[0], str):
            return
        offsets = kwargs.pop("offsets", None)
        connectivity = kwargs.pop("connectivity", None)
        vtkCellArray.__init__(self, *args, **kwargs)
        if (offsets is None) != (connectivity is None):
            raise ValueError(
                "offsets and connectivity must both be provided"
            )
        if offsets is not None:
            self._set_data(offsets, connectivity)

    def _set_data(self, offsets, connectivity):
        if isinstance(offsets, vtkDataArray) and isinstance(connectivity, vtkDataArray):
            self.SetData(offsets, connectivity)
            return
        if NUMPY_AVAILABLE:
            import numpy as np
            if not isinstance(offsets, vtkDataArray):
                offsets = numpy_support.numpy_to_vtk(
                    np.asarray(offsets, dtype=np.int64)
                )
            if not isinstance(connectivity, vtkDataArray):
                connectivity = numpy_support.numpy_to_vtk(
                    np.asarray(connectivity, dtype=np.int64)
                )
            self.SetData(offsets, connectivity)
        else:
            raise TypeError(
                "numpy is required to convert array-like arguments"
            )

    def __repr__(self):
        n = self.GetNumberOfCells()
        return "vtkCellArray(%d cells)" % n


@vtkPoints.override
class Points(vtkPoints):
    """Python-friendly ``vtkPoints`` with a convenient constructor.

    Accepts point data via the ``data`` keyword argument — as a numpy
    array, list of lists, or ``vtkDataArray``::

        pts = vtkPoints(data=np.array([[0,0,0], [1,0,0]]))
        pts = vtkPoints(data=[[0,0,0], [1,0,0]])
        pts = vtkPoints(data=vtk_data_array)
    """

    def __init__(self, *args, **kwargs):
        if args and isinstance(args[0], str):
            return
        data = kwargs.pop("data", None)
        vtkPoints.__init__(self, *args, **kwargs)
        if data is not None:
            self._set_data(data)

    def _set_data(self, data):
        if isinstance(data, vtkDataArray):
            self.SetData(data)
            return
        if NUMPY_AVAILABLE:
            import numpy as np
            arr = np.asarray(data, dtype=np.float64)
            if arr.ndim == 1:
                if len(arr) % 3 != 0:
                    raise ValueError(
                        "1-D array length must be a multiple of 3"
                    )
                arr = arr.reshape(-1, 3)
            vtk_arr = numpy_support.numpy_to_vtk(arr)
            vtk_arr.SetName("points")
            self.SetData(vtk_arr)
        else:
            for pt in data:
                self.InsertNextPoint(*pt)

    def __repr__(self):
        n = self.GetNumberOfPoints()
        return "vtkPoints(%d points)" % n


# -----------------------------------------------------------------------------
# Handle pickle registration
# -----------------------------------------------------------------------------
with suppress(ImportError):
    import copyreg
    from vtkmodules.util.pickle_support import serialize_VTK_data_object

    copyreg.pickle(PolyData, serialize_VTK_data_object)
    copyreg.pickle(UnstructuredGrid, serialize_VTK_data_object)
    copyreg.pickle(ImageData, serialize_VTK_data_object)
    copyreg.pickle(PartitionedDataSet, serialize_VTK_data_object)
    copyreg.pickle(StructuredGrid, serialize_VTK_data_object)

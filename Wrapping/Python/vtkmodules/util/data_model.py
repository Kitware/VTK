"""This module provides classes that allow numpy style access
to VTK datasets. See examples at bottom.
"""

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
    from vtkmodules.numpy_interface import dataset_adapter as dsa

    NUMPY_AVAILABLE = True


class FieldDataBase(object):
    def __init__(self):
        self.association = None
        self.dataset = None

    def __getitem__(self, idx):
        """Implements the [] operator. Accepts an array name or index."""
        return self.get_array(idx)

    def __setitem__(self, name, value):
        """Implements the [] operator. Accepts an array name or index."""
        return self.set_array(name, value)

    def get_array(self, idx):
        "Given an index or name, returns a VTKArray."
        if isinstance(idx, int) and idx >= self.GetNumberOfArrays():
            raise IndexError("array index out of range")
        vtkarray = super().GetArray(idx)

        if not NUMPY_AVAILABLE:
            return vtkarray if vtkarray else self.GetAbstractArray(idx)

        if not vtkarray:
            vtkarray = self.GetAbstractArray(idx)
            if vtkarray:
                return vtkarray
            return dsa.NoneArray
        array = dsa.vtkDataArrayToVTKArray(vtkarray, self.dataset)
        array.Association = self.association
        return array

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

        if narray is dsa.NoneArray:
            # if NoneArray, nothing to do.
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

        # this handle the case when an input array is directly appended on the
        # output. We want to make sure that the array added to the output is not
        # referring to the input dataset.
        copy = dsa.VTKArray(narray)
        try:
            copy.VTKObject = narray.VTKObject
        except AttributeError:
            pass
        arr = dsa.numpyTovtkDataArray(copy, name)
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
        self.DataSet = dataset
        self.Association = association
        self.ArrayNames = []
        self.Arrays = {}

        # build the set of arrays available in the composite dataset. Since
        # composite datasets can have partial arrays, we need to iterate over
        # all non-null blocks in the dataset.
        self.__determine_arraynames()

    def __determine_arraynames(self):
        array_set = set()
        array_list = []
        for dataset in self.DataSet:
            dsa = dataset.GetAttributesAsFieldData(self.Association)
            for array_name in dsa.keys():
                if array_name not in array_set:
                    array_set.add(array_name)
                    array_list.append(array_name)
        self.ArrayNames = array_list

    def modified(self):
        """Rescans the contained dataset to update the
        internal list of arrays."""
        self.__determine_arraynames()

    def __contains__(self, aname):
        """Returns true if the container contains arrays
        with the given name, false otherwise"""
        return aname in self.ArrayNames

    def keys(self):
        """Returns the names of the arrays as a tuple."""
        return tuple(self.ArrayNames)

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

        if narray is dsa.NoneArray:
            # if NoneArray, nothing to do.
            return

        added = False
        if not isinstance(narray, dsa.VTKCompositeDataArray):  # Scalar input
            for ds in self.DataSet:
                ds.GetAttributesAsFieldData(self.Association).set_array(name, narray)
                added = True
            if added:
                self.ArrayNames.append(name)
                # don't add the narray since it's a scalar. GetArray() will create a
                # VTKCompositeArray on-demand.
        else:
            for ds, array in zip(self.DataSet, narray.Arrays):
                if array is not None:
                    ds.GetAttributesAsFieldData(self.Association).set_array(name, array)
                    added = True
            if added:
                self.ArrayNames.append(name)
                self.Arrays[name] = weakref.ref(narray)

    def get_array(self, idx):
        """Given a name, returns a VTKCompositeArray."""
        arrayname = idx

        if not NUMPY_AVAILABLE:
            # don't know how to handle composite dataset attribute when numpy not around
            raise NotImplementedError("Only available with numpy")

        if arrayname not in self.ArrayNames:
            return dsa.NoneArray
        if arrayname not in self.Arrays or self.Arrays[arrayname]() is None:
            array = dsa.VTKCompositeDataArray(
                dataset=self.DataSet, name=arrayname, association=self.Association
            )
            self.Arrays[arrayname] = weakref.ref(array)
        else:
            array = self.Arrays[arrayname]()
        return array

    def __iter__(self):
        """Iterators on keys"""
        return iter(self.ArrayNames)

    def __len__(self):
        return len(self.ArrayNames)

# class DataSet(DataObjectBase):
class DataSet(object):
    def __init__(self, **kwargs) -> None:
        self._numpy_attrs = []

    @property
    def point_data(self):
        pd = super().GetPointData()
        pd.dataset = self
        pd.association = self.POINT
        return pd

    @property
    def cell_data(self):
        cd = super().GetCellData()
        cd.dataset = self
        cd.association = self.CELL
        return cd

    @property
    def field_data(self):
        fd = super().GetFieldData()
        if fd:
            fd.dataset = self
            fd.association = self.FIELD
        return fd

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
    def __init__(self, **kwargs) -> None:
        DataSet.__init__(self, **kwargs)
        self._numpy_attrs.append("points")

    @property
    def points(self):
        pts = self.GetPoints()

        if not NUMPY_AVAILABLE:
            return pts

        if not pts or not pts.GetData():
            return None
        return dsa.vtkDataArrayToVTKArray(pts.GetData())

    @points.setter
    def points(self, points):
        if isinstance(points, vtkPoints):
            self.SetPoints(points)
            return

        if not NUMPY_AVAILABLE:
            raise ValueError("Expect vtkPoints")

        pts = dsa.numpyTovtkDataArray(points, "points")
        vtkpts = vtkPoints()
        vtkpts.SetData(pts)
        self.SetPoints(vtkpts)


@vtkUnstructuredGrid.override
class UnstructuredGrid(PointSet, vtkUnstructuredGrid):
    def __init__(self, **kwargs):
        PointSet.__init__(self, **kwargs)
        vtkUnstructuredGrid.__init__(self, **kwargs)

    @property
    def cells(self):
        ca = self.GetCells()
        conn_vtk = ca.GetConnectivityArray()
        offsets_vtk = ca.GetOffsetsArray()
        ct_vtk = self.GetCellTypesArray()

        if not NUMPY_AVAILABLE:
            return {
                "connectivity": conn_vtk,
                "offsets": offsets_vtk,
                "cell_types": ct_vtk,
            }

        conn = dsa.vtkDataArrayToVTKArray(conn_vtk)
        offsets = dsa.vtkDataArrayToVTKArray(offsets_vtk)
        ct = dsa.vtkDataArrayToVTKArray(ct_vtk)
        return {"connectivity": conn, "offsets": offsets, "cell_types": ct}

    @cells.setter
    def cells(self, cells):
        ca = vtkCellArray()

        if not NUMPY_AVAILABLE:
            ca.SetData(cells["offsets"], cells["connectivity"])
            self.SetCells(cells["cell_types"], ca)
            return

        conn_vtk = dsa.numpyTovtkDataArray(cells["connectivity"])
        offsets_vtk = dsa.numpyTovtkDataArray(cells["offsets"])
        cell_types_vtk = dsa.numpyTovtkDataArray(cells["cell_types"])
        ca.SetData(offsets_vtk, conn_vtk)
        self.SetCells(cell_types_vtk, ca)


@vtkImageData.override
class ImageData(DataSet, vtkImageData):
    def __init__(self, **kwargs):
        DataSet.__init__(self, **kwargs)
        vtkImageData.__init__(self, **kwargs)


@vtkPolyData.override
class PolyData(PointSet, vtkPolyData):
    def __init__(self, **kwargs) -> None:
        PointSet.__init__(self, **kwargs)
        vtkPolyData.__init__(self, **kwargs)
        self._numpy_attrs.extend(["verts", "lines", "strips", "polys"])

    @property
    def verts_arrays(self):
        ca = self.GetVerts()
        conn_vtk = ca.GetConnectivityArray()
        offsets_vtk = ca.GetOffsetsArray()

        if not NUMPY_AVAILABLE:
            return {
                "connectivity": conn_vtk,
                "offsets": offsets_vtk,
            }

        conn = dsa.vtkDataArrayToVTKArray(conn_vtk)
        offsets = dsa.vtkDataArrayToVTKArray(offsets_vtk)
        return {"connectivity": conn, "offsets": offsets}

    @property
    def lines_arrays(self):
        ca = self.GetLines()
        conn_vtk = ca.GetConnectivityArray()
        offsets_vtk = ca.GetOffsetsArray()

        if not NUMPY_AVAILABLE:
            return {
                "connectivity": conn_vtk,
                "offsets": offsets_vtk,
            }

        conn = dsa.vtkDataArrayToVTKArray(conn_vtk)
        offsets = dsa.vtkDataArrayToVTKArray(offsets_vtk)
        return {"connectivity": conn, "offsets": offsets}

    @property
    def strips_arrays(self):
        ca = self.GetStrips()
        conn_vtk = ca.GetConnectivityArray()
        offsets_vtk = ca.GetOffsetsArray()

        if not NUMPY_AVAILABLE:
            return {
                "connectivity": conn_vtk,
                "offsets": offsets_vtk,
            }

        conn = dsa.vtkDataArrayToVTKArray(conn_vtk)
        offsets = dsa.vtkDataArrayToVTKArray(offsets_vtk)
        return {"connectivity": conn, "offsets": offsets}

    @property
    def polys_arrays(self):
        ca = self.GetPolys()
        conn_vtk = ca.GetConnectivityArray()
        offsets_vtk = ca.GetOffsetsArray()

        if not NUMPY_AVAILABLE:
            return {
                "connectivity": conn_vtk,
                "offsets": offsets_vtk,
            }

        conn = dsa.vtkDataArrayToVTKArray(conn_vtk)
        offsets = dsa.vtkDataArrayToVTKArray(offsets_vtk)
        return {"connectivity": conn, "offsets": offsets}


@vtkRectilinearGrid.override
class RectilinearGrid(DataSet, vtkRectilinearGrid):
    def __init__(self, **kwargs) -> None:
        DataSet.__init__(self, **kwargs)
        vtkRectilinearGrid.__init__(self, **kwargs)
        self._numpy_attrs.extend(["x_coordinates", "y_coordinates", "z_coordinates"])

    @property
    def x_coordinates(self):
        pts = self.GetXCoordinates()

        if not NUMPY_AVAILABLE:
            return pts

        if not pts:
            return None
        return dsa.vtkDataArrayToVTKArray(pts)

    @x_coordinates.setter
    def x_coordinates(self, points):
        if isinstance(points, vtkDataArray):
            self.SetXCoordinates(points)
            return

        if not NUMPY_AVAILABLE:
            raise ValueError("Expect vtkDataArray")

        pts = dsa.numpyTovtkDataArray(points, "x_coords")
        self.SetXCoordinates(pts)

    @property
    def y_coordinates(self):
        pts = self.GetYCoordinates()

        if not NUMPY_AVAILABLE:
            return pts

        if not pts:
            return None
        return dsa.vtkDataArrayToVTKArray(pts)

    @y_coordinates.setter
    def y_coordinates(self, points):
        if isinstance(points, vtkDataArray):
            self.SetYCoordinates(points)
            return

        if not NUMPY_AVAILABLE:
            raise ValueError("Expect vtkDataArray")

        pts = dsa.numpyTovtkDataArray(points, "y_coords")
        self.SetYCoordinates(pts)

    @property
    def z_coordinates(self):
        pts = self.GetZCoordinates()

        if not NUMPY_AVAILABLE:
            return pts

        if not pts:
            return None
        return dsa.vtkDataArrayToVTKArray(pts)

    @z_coordinates.setter
    def z_coordinates(self, points):
        if isinstance(points, vtkDataArray):
            self.SetZCoordinates(points)
            return

        if not NUMPY_AVAILABLE:
            raise ValueError("Expect vtkDataArray")

        pts = dsa.numpyTovtkDataArray(points, "z_coords")
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
    """A wrapper for vtkCompositeData and subclasses that makes it easier
    to access Point/Cell/Field data as VTKCompositeDataArrays. It also
    provides a Python type iterator."""

    def __init__(self, **kwargs):
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
        "Returns the points as a VTKCompositeDataArray instance."
        if not NUMPY_AVAILABLE:
            # don't know how to handle composite dataset when numpy not around
            raise NotImplementedError("Only available with numpy")

        if self._Points is None or self._Points() is None:
            pts = []
            for ds in self:
                try:
                    _pts = ds.Points
                except AttributeError:
                    _pts = None

                if _pts is None:
                    pts.append(dsa.NoneArray)
                else:
                    pts.append(_pts)
            if len(pts) == 0 or all([a is dsa.NoneArray for a in pts]):
                cpts = dsa.NoneArray
            else:
                cpts = dsa.VTKCompositeDataArray(pts, dataset=self)
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
    def __init__(self, **kwargs):
        PointSet.__init__(self, **kwargs)
        vtkStructuredGrid.__init__(self, **kwargs)

    @property
    def x_coordinates(self):
        if not NUMPY_AVAILABLE:
            raise NotImplementedError("Only available with numpy")

        dims = [0,0,0]
        self.GetDimensions(dims)
        return self.points[:, 0].reshape(dims, order="F")

    @property
    def y_coordinates(self):
        if not NUMPY_AVAILABLE:
            raise NotImplementedError("Only available with numpy")

        dims = [0,0,0]
        self.GetDimensions(dims)
        return self.points[:, 1].reshape(dims, order="F")

    @property
    def z_coordinates(self):
        if not NUMPY_AVAILABLE:
            raise NotImplementedError("Only available with numpy")
        dims = [0,0,0]
        self.GetDimensions(dims)
        return self.points[:, 2].reshape(dims, order="F")


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

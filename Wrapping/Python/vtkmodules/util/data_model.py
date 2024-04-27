"""This module provides classes that allow numpy style access
to VTK datasets. See examples at bottom.
"""

from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkDataObject,
    vtkFieldData,
    vtkDataSetAttributes,
    vtkPointData,
    vtkCellData,
    vtkDataObject,
    vtkImageData,
    vtkPolyData,
    vtkUnstructuredGrid,
    vtkPartitionedDataSet
)

from vtkmodules.numpy_interface import dataset_adapter as dsa
import numpy
import weakref

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
        if not vtkarray:
            vtkarray = self.GetAbstractArray(idx)
            if vtkarray:
                return vtkarray
            return dsa.NoneArray
        array = dsa.vtkDataArrayToVTKArray(vtkarray, self.dataset)
        array.Association = self.association
        return array

    def keys(self):
        """Returns the names of the arrays as a list."""
        kys = []
        narrays = self.GetNumberOfArrays()
        for i in range(narrays):
            name = self.GetAbstractArray(i).GetName()
            if name:
                kys.append(name)
        return kys

    def values(self):
        """Returns the arrays as a list."""
        vals = []
        narrays = self.GetNumberOfArrays()
        for i in range(narrays):
            a = self.get_array(i)
            if a.GetName():
                vals.append(a)
        return vals

    def set_array(self, name, narray):
        """Appends a new array to the dataset attributes."""
        if narray is dsa.NoneArray:
            # if NoneArray, nothing to do.
            return

        if self.association == vtkDataObject.POINT:
            arrLength = self.dataset.GetNumberOfPoints()
        elif self.association == vtkDataObject.CELL:
            arrLength = self.dataset.GetNumberOfCells()
        elif self.association == vtkDataObject.ROW \
          and self.dataset.GetNumberOfColumns() > 0:
            arrLength = self.dataset.GetNumberOfRows()
        else:
            if not isinstance(narray, numpy.ndarray):
                arrLength = 1
            else:
                arrLength = narray.shape[0]

        # Fixup input array length:
        if not isinstance(narray, numpy.ndarray) or numpy.ndim(narray) == 0: # Scalar input
            dtype = narray.dtype if isinstance(narray, numpy.ndarray) else type(narray)
            tmparray = numpy.empty(arrLength, dtype=dtype)
            tmparray.fill(narray)
            narray = tmparray
        elif narray.shape[0] != arrLength: # Vector input
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
            if (narray.strides[1]/size == 3 and narray.strides[2]/size == 1) or \
                (narray.strides[1]/size == 1 and narray.strides[2]/size == 3 and \
                 not narray.flags.contiguous):
                narray  = narray.transpose(0, 2, 1)

        # If array is not contiguous, make a deep copy that is contiguous
        if not narray.flags.contiguous:
            narray = numpy.ascontiguousarray(narray)

        # Flatten array of matrices to array of vectors
        if len(shape) == 3:
            narray = narray.reshape(shape[0], shape[1]*shape[2])

        # this handle the case when an input array is directly appended on the
        # output. We want to make sure that the array added to the output is not
        # referring to the input dataset.
        copy = dsa.VTKArray(narray)
        try:
            copy.VTKObject = narray.VTKObject
        except AttributeError: pass
        arr = dsa.numpyTovtkDataArray(copy, name)
        self.AddArray(arr)

@vtkFieldData.override
class vtkFieldData(FieldDataBase, vtkFieldData):
    pass

class DataSetAttributesBase(FieldDataBase):
    pass

@vtkDataSetAttributes.override
class DataSetAttributes(DataSetAttributesBase, vtkDataSetAttributes):
    pass

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
            dsa = dataset.GetAttributes(self.Association)
            for array_name in dsa.keys():
                if array_name not in array_set:
                    array_set.add(array_name)
                    array_list.append(array_name)
        self.ArrayNames = array_list

    def modified(self):
        """Rescans the contained dataset to update the
        internal list of arrays."""
        self.__determine_arraynames()

    def keys(self):
        """Returns the names of the arrays as a list."""
        return self.ArrayNames

    def __getitem__(self, idx):
        """Implements the [] operator. Accepts an array name."""
        return self.get_array(idx)

    def __setitem__(self, name, narray):
        """Implements the [] operator. Accepts an array name."""
        return self.set_array(name, narray)

    def set_array(self, name, narray):
        """Appends a new array to the composite dataset attributes."""
        if narray is dsa.NoneArray:
            # if NoneArray, nothing to do.
            return

        added = False
        if not isinstance(narray, dsa.VTKCompositeDataArray): # Scalar input
            for ds in self.DataSet:
                ds.GetAttributes(self.Association).set_array(name, narray)
                added = True
            if added:
                self.ArrayNames.append(name)
                # don't add the narray since it's a scalar. GetArray() will create a
                # VTKCompositeArray on-demand.
        else:
            for ds, array in zip(self.DataSet, narray.Arrays):
                if array is not None:
                    ds.GetAttributes(self.Association).set_array(name, array)
                    added = True
            if added:
                self.ArrayNames.append(name)
                self.Arrays[name] = weakref.ref(narray)

    def get_array(self, idx):
        """Given a name, returns a VTKCompositeArray."""
        arrayname = idx
        if arrayname not in self.ArrayNames:
            return dsa.NoneArray
        if arrayname not in self.Arrays or self.Arrays[arrayname]() is None:
            array = dsa.VTKCompositeDataArray(
                dataset = self.DataSet, name = arrayname, association = self.Association)
            self.Arrays[arrayname] = weakref.ref(array)
        else:
            array = self.Arrays[arrayname]()
        return array

    def __iter__(self):
        "Creates an iterator for the contained arrays."
        return CompositeDataSetAttributesIterator(self)

#class DataSet(DataObjectBase):
class DataSet(object):
    @property
    def point_data(self):
        pd = super().GetPointData()
        pd.dataset = self
        # TODO: temporary hack
        pd.dataset.VTKObject = pd.dataset
        pd.association = self.POINT
        return pd

    @property
    def cell_data(self):
        cd = super().GetCellData()
        cd.dataset = self
        cd.association = self.CELL
        return cd

    def convert_to_unstructured_grid(self):
        from vtkmodules.vtkFiltersCore import vtkExtractCells

        ecells = vtkExtractCells()
        ecells.SetInputData(self)
        ecells.ExtractAllCellsOn()
        ecells.Update()
        return ecells.GetOutput()

class PointSet(DataSet):
    @property
    def points(self):
        pts = self.GetPoints()
        if not pts or not pts.GetData():
            return None
        return dsa.vtkDataArrayToVTKArray(pts.GetData())

    @points.setter
    def points(self, points):
        pts = dsa.numpyTovtkDataArray(points, "points")
        vtkpts = vtkPoints()
        vtkpts.SetData(pts)
        self.SetPoints(vtkpts)

@vtkUnstructuredGrid.override
class vtkUnstructuredGrid(PointSet, vtkUnstructuredGrid):
    @property
    def cells(self):
        ca = self.GetCells()
        conn_vtk = ca.GetConnectivityArray()
        conn = dsa.vtkDataArrayToVTKArray(conn_vtk)
        offsets_vtk = ca.GetOffsetsArray()
        offsets = dsa.vtkDataArrayToVTKArray(offsets_vtk)
        ct_vtk = self.GetCellTypesArray()
        ct = dsa.vtkDataArrayToVTKArray(ct_vtk)
        return { 'connectivity' : conn, 'offsets' : offsets , 'cell_types' : ct}

    @cells.setter
    def cells(self, cells):
        ca = vtkCellArray()
        conn_vtk = dsa.numpyTovtkDataArray(cells['connectivity'])
        offsets_vtk = dsa.numpyTovtkDataArray(cells['offsets'])
        cell_types_vtk = dsa.numpyTovtkDataArray(cells['cell_types'])
        print(cells['cell_types'][1])
        ca.SetData(offsets_vtk, conn_vtk)
        self.SetCells(cell_types_vtk, ca)

@vtkImageData.override
class vtkImageData(DataSet, vtkImageData):
    pass

@vtkPolyData.override
class vtkPolyData(PointSet, vtkPolyData):
    @property
    def polygons(self):
        ca = self.GetPolys()
        conn_vtk = ca.GetConnectivityArray()
        conn = dsa.vtkDataArrayToVTKArray(conn_vtk)
        offsets_vtk = ca.GetOffsetsArray()
        offsets = dsa.vtkDataArrayToVTKArray(offsets_vtk)
        return { 'connectivity' : conn, 'offsets' : offsets }

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
        super().__init__(**kwargs)

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
            cdata = self.get_attributes(DataObject.CELL)
            self._CellData = weakref.ref(cdata)
        return self._CellData()

    @property
    def field_data(self):
        "Returns the field data as a DataSetAttributes instance."
        if self._FieldData is None or self._FieldData() is None:
            fdata = self.get_attributes(DataObject.FIELD)
            self._FieldData = weakref.ref(fdata)
        return self._FieldData()

    @property
    def points(self):
        "Returns the points as a VTKCompositeDataArray instance."
        if self._Points is None or self._Points() is None:
            pts = []
            for ds in self:
                try:
                    _pts = ds.Points
                except AttributeError:
                    _pts = None

                if _pts is None:
                    pts.append(NoneArray)
                else:
                    pts.append(_pts)
            if len(pts) == 0 or all([a is NoneArray for a in pts]):
                cpts = NoneArray
            else:
                cpts = dsa.VTKCompositeDataArray(pts, dataset=self)
            self._Points = weakref.ref(cpts)
        return self._Points()

@vtkPartitionedDataSet.override
class vtkPartitionedDataSet(CompositeDataSetBase, vtkPartitionedDataSet):
    def append(self, dataset):
        self.SetPartition(self.GetNumberOfPartitions(), dataset)

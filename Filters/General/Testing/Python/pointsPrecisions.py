import unittest

from vtk.vtkCommonCore import vtkPoints, vtkDoubleArray, vtkIdList
from vtk.vtkCommonDataModel import vtkPlane,\
                                   vtkUnstructuredGrid,\
                                   vtkStructuredGrid,\
                                   vtkPolyData
from vtk.vtkFiltersGeneral import vtkClipDataSet
from vtk.vtkFiltersGeneral import vtkTableBasedClipDataSet
import vtk.util.vtkConstants as vtk_const

class FiltersLosingPrecisionBase:
    def test_clip(self):
        p = vtkPlane()
        p.SetOrigin(0,0,0)
        p.SetNormal(1,0,0)
        f = vtkClipDataSet()
        f.SetInputData(self.cell)
        f.SetClipFunction(p)
        f.SetValue(0)
        f.Update()
        self.assertEquals(f.GetOutput().GetPoints().GetDataType(), vtk_const.VTK_DOUBLE)

    def test_tablebasedclip(self):
        p = vtkPlane()
        p.SetOrigin(0,0,0)
        p.SetNormal(1,0,0)
        f = vtkTableBasedClipDataSet()
        f.SetInputData(self.cell)
        f.SetClipFunction(p)
        f.SetValue(0)
        f.Update()
        self.assertEquals(f.GetOutput().GetPoints().GetDataType(), vtk_const.VTK_DOUBLE)

class TestUnstructuredGridFiltersLosingPrecision(unittest.TestCase, FiltersLosingPrecisionBase):
    def setUp(self):
        self.cell = vtkUnstructuredGrid()
        pts = vtkPoints()
        pts.SetDataTypeToDouble()
        pts.InsertNextPoint(-1.0, -1.0, -1.0)
        pts.InsertNextPoint( 1.0, -1.0, -1.0)
        pts.InsertNextPoint( 1.0,  1.0, -1.0)
        pts.InsertNextPoint(-1.0,  1.0, -1.0)
        pts.InsertNextPoint(-1.0, -1.0,  1.0)
        pts.InsertNextPoint( 1.0, -1.0,  1.0)
        pts.InsertNextPoint( 1.0,  1.0,  1.0)
        pts.InsertNextPoint(-1.0,  1.0,  1.0)
        self.cell.SetPoints(pts)
        self.cell.Allocate(1,1)
        ids = vtkIdList()
        for i in range(8):
            ids.InsertId(i,i)
        self.cell.InsertNextCell(vtk_const.VTK_HEXAHEDRON, ids)
        scalar = vtkDoubleArray()
        scalar.SetName('scalar')
        scalar.SetNumberOfTuples(8)
        scalar.SetValue(0, 0.0)
        scalar.SetValue(1, 0.0)
        scalar.SetValue(2, 0.0)
        scalar.SetValue(3, 0.0)
        scalar.SetValue(4, 1.0)
        scalar.SetValue(5, 1.0)
        scalar.SetValue(6, 1.0)
        scalar.SetValue(7, 1.0)
        self.cell.GetPointData().SetScalars(scalar)

class TestStructuredGridFiltersLosingPrecision(unittest.TestCase, FiltersLosingPrecisionBase):
    def setUp(self):
        self.cell = vtkStructuredGrid()
        pts = vtkPoints()
        pts.SetDataTypeToDouble()
        pts.InsertNextPoint(-1.0, -1.0, -1.0)
        pts.InsertNextPoint( 1.0, -1.0, -1.0)
        pts.InsertNextPoint( 1.0,  1.0, -1.0)
        pts.InsertNextPoint(-1.0,  1.0, -1.0)
        pts.InsertNextPoint(-1.0, -1.0,  1.0)
        pts.InsertNextPoint( 1.0, -1.0,  1.0)
        pts.InsertNextPoint( 1.0,  1.0,  1.0)
        pts.InsertNextPoint(-1.0,  1.0,  1.0)
        self.cell.SetDimensions(2,2,2)
        self.cell.SetPoints(pts)
        scalar = vtkDoubleArray()
        scalar.SetName('scalar')
        scalar.SetNumberOfTuples(8)
        scalar.SetValue(0, 0.0)
        scalar.SetValue(1, 0.0)
        scalar.SetValue(2, 0.0)
        scalar.SetValue(3, 0.0)
        scalar.SetValue(4, 1.0)
        scalar.SetValue(5, 1.0)
        scalar.SetValue(6, 1.0)
        scalar.SetValue(7, 1.0)
        self.cell.GetPointData().SetScalars(scalar)

class TestPolyDataFiltersLosingPrecision(unittest.TestCase, FiltersLosingPrecisionBase):
    def setUp(self):
        self.cell = vtkPolyData()
        pts = vtkPoints()
        pts.SetDataTypeToDouble()
        pts.InsertNextPoint(-1.0, -1.0, -1.0)
        pts.InsertNextPoint( 1.0, -1.0, -1.0)
        pts.InsertNextPoint( 1.0,  1.0, -1.0)
        pts.InsertNextPoint(-1.0,  1.0, -1.0)
        self.cell.SetPoints(pts)
        self.cell.Allocate(1,1)
        ids = vtkIdList()
        for i in range(4):
            ids.InsertId(i,i)
        self.cell.InsertNextCell(vtk_const.VTK_QUAD, ids)
        scalar = vtkDoubleArray()
        scalar.SetName('scalar')
        scalar.SetNumberOfTuples(8)
        scalar.SetValue(0, 0.0)
        scalar.SetValue(1, 0.0)
        scalar.SetValue(2, 1.0)
        scalar.SetValue(3, 1.0)
        self.cell.GetPointData().SetScalars(scalar)

if __name__ == '__main__':
    unittest.main()

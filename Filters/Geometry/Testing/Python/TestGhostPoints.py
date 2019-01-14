import sys
import vtk
from vtk.test import Testing

class TestGhostPoints(Testing.vtkTest):
    def doLinear(self, ghosts, ncells):
        pts = vtk.vtkPoints()
        pts.SetNumberOfPoints(4)
        pts.InsertPoint(0, (0, 0, 0))
        pts.InsertPoint(1, (1, 0, 0))
        pts.InsertPoint(2, (0.5, 1, 0))
        pts.InsertPoint(3, (0.5, 0.5, 1))

        te = vtk.vtkTetra()
        ptIds = te.GetPointIds()
        for i in range(4):
            ptIds.SetId(i, i)

        grid = vtk.vtkUnstructuredGrid()
        grid.Allocate(1, 1)
        grid.InsertNextCell(te.GetCellType(), te.GetPointIds())
        grid.SetPoints(pts)
        grid.GetPointData().AddArray(ghosts)

        dss = vtk.vtkDataSetSurfaceFilter()
        dss.SetInputData(grid)
        dss.Update()
        self.assertEqual(dss.GetOutput().GetNumberOfCells(), ncells)

    def testLinearDuplicate(self):
        duplicate = vtk.vtkDataSetAttributes.DUPLICATEPOINT
        ghosts = vtk.vtkUnsignedCharArray()
        ghosts.SetName(vtk.vtkDataSetAttributes.GhostArrayName())
        ghosts.SetNumberOfTuples(4)
        ghosts.SetValue(0, duplicate)
        ghosts.SetValue(1, duplicate)
        ghosts.SetValue(2, duplicate)
        ghosts.SetValue(3, 0)

        self.doLinear(ghosts, 3)

    def testLinearHidden(self):
        hidden = vtk.vtkDataSetAttributes.HIDDENPOINT
        ghosts = vtk.vtkUnsignedCharArray()
        ghosts.SetName(vtk.vtkDataSetAttributes.GhostArrayName())
        ghosts.SetNumberOfTuples(4)
        ghosts.SetValue(0, hidden)
        ghosts.SetValue(1, hidden)
        ghosts.SetValue(2, hidden)
        ghosts.SetValue(3, 0)

        self.doLinear(ghosts, 0)

    def doNonLinear(self, ghosts, ncells):
        pts = vtk.vtkPoints()
        pts.SetNumberOfPoints(10)
        pts.InsertPoint(0, (0, 0, 0))
        pts.InsertPoint(1, (1, 0, 0))
        pts.InsertPoint(2, (0.5, 1, 0))
        pts.InsertPoint(3, (0.5, 0.5, 1))
        pts.InsertPoint(4, (0.5, 0, 0))
        pts.InsertPoint(5, (1.25, 0.5, 0))
        pts.InsertPoint(6, (0.25, 0.5, 0))
        pts.InsertPoint(7, (0.25, 0.25, 0.5))
        pts.InsertPoint(8, (0.75, 0.25, 0.5))
        pts.InsertPoint(9, (0.5, 0.75, 0.5))

        te = vtk.vtkQuadraticTetra()
        ptIds = te.GetPointIds()
        for i in range(10):
            ptIds.SetId(i, i)

        grid = vtk.vtkUnstructuredGrid()
        grid.Allocate(1, 1)
        grid.InsertNextCell(te.GetCellType(), te.GetPointIds())
        grid.SetPoints(pts)
        grid.GetPointData().AddArray(ghosts)

        ugg = vtk.vtkUnstructuredGridGeometryFilter()
        ugg.SetInputData(grid)

        dss = vtk.vtkDataSetSurfaceFilter()
        dss.SetNonlinearSubdivisionLevel(2)
        dss.SetInputConnection(ugg.GetOutputPort())
        dss.Update()
        self.assertEqual(dss.GetOutput().GetNumberOfCells(), ncells)

    def testNonLinearDuplicate(self):
        duplicate = vtk.vtkDataSetAttributes.DUPLICATEPOINT

        ghosts = vtk.vtkUnsignedCharArray()
        ghosts.SetName(vtk.vtkDataSetAttributes.GhostArrayName())
        ghosts.SetNumberOfTuples(10)
        ghosts.SetValue(0, duplicate)
        ghosts.SetValue(1, duplicate)
        ghosts.SetValue(2, duplicate)
        ghosts.SetValue(3, 0)
        ghosts.SetValue(4, duplicate)
        ghosts.SetValue(5, duplicate)
        ghosts.SetValue(6, duplicate)
        ghosts.SetValue(7, 0)
        ghosts.SetValue(8, 0)
        ghosts.SetValue(9, 0)

        self.doNonLinear(ghosts, 48)

    def testNonLinearHidden(self):
        hidden = vtk.vtkDataSetAttributes.HIDDENPOINT

        ghosts = vtk.vtkUnsignedCharArray()
        ghosts.SetName(vtk.vtkDataSetAttributes.GhostArrayName())
        ghosts.SetNumberOfTuples(10)
        ghosts.SetValue(0, hidden)
        ghosts.SetValue(1, hidden)
        ghosts.SetValue(2, hidden)
        ghosts.SetValue(3, 0)
        ghosts.SetValue(4, hidden)
        ghosts.SetValue(5, hidden)
        ghosts.SetValue(6, hidden)
        ghosts.SetValue(7, 0)
        ghosts.SetValue(8, 0)
        ghosts.SetValue(9, 0)

        self.doNonLinear(ghosts, 0)

if __name__ == "__main__":
    Testing.main([(TestGhostPoints, 'test')])

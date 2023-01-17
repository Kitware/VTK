from vtkmodules.vtkCommonCore import (
    vtkIdList,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    VTK_POLYHEDRON,
    vtkPlane,
    vtkUnstructuredGrid,
)
from vtkmodules.vtkFiltersCore import vtkCutter
from vtkmodules.vtkIOXML import (
    vtkXMLPolyDataWriter,
    vtkXMLUnstructuredGridWriter,
)
from vtkmodules.test import Testing

class TestPolyhedronCutter(Testing.vtkTest):
    def setUp(self):
        self.vtk_grid = vtkUnstructuredGrid()

        # vertices of the non-convex polyhedron
        vtk_points = vtkPoints()

        vtk_points.InsertNextPoint(0.0, 0.0, 0.0)
        vtk_points.InsertNextPoint(4.0, 0.0, 0.0)
        vtk_points.InsertNextPoint(4.0, 2.0, 0.0)
        vtk_points.InsertNextPoint(3.0, 2.0, 0.0)
        vtk_points.InsertNextPoint(3.0, 1.0, 0.0)
        vtk_points.InsertNextPoint(1.0, 1.0, 0.0)
        vtk_points.InsertNextPoint(1.0, 2.0, 0.0)
        vtk_points.InsertNextPoint(0.0, 2.0, 0.0)

        vtk_points.InsertNextPoint(0.0, 0.0, 1.0)
        vtk_points.InsertNextPoint(4.0, 0.0, 1.0)
        vtk_points.InsertNextPoint(4.0, 2.0, 1.0)
        vtk_points.InsertNextPoint(3.0, 2.0, 1.0)
        vtk_points.InsertNextPoint(3.0, 1.0, 1.0)
        vtk_points.InsertNextPoint(1.0, 1.0, 1.0)
        vtk_points.InsertNextPoint(1.0, 2.0, 1.0)
        vtk_points.InsertNextPoint(0.0, 2.0, 1.0)
        self.vtk_grid.SetPoints(vtk_points)

        # connectivity of the polyhedron
        ids = vtkIdList()
        ids.InsertNextId(10) # no. of faces

        ids.InsertNextId(8)  # face 1 no. of verts
        ids.InsertNextId(0)
        ids.InsertNextId(1)
        ids.InsertNextId(2)
        ids.InsertNextId(3)
        ids.InsertNextId(4)
        ids.InsertNextId(5)
        ids.InsertNextId(6)
        ids.InsertNextId(7)

        ids.InsertNextId(8)  # face 2 no. of verts
        ids.InsertNextId(8)
        ids.InsertNextId(9)
        ids.InsertNextId(10)
        ids.InsertNextId(11)
        ids.InsertNextId(12)
        ids.InsertNextId(13)
        ids.InsertNextId(14)
        ids.InsertNextId(15)

        ids.InsertNextId(4)
        ids.InsertNextId(0)
        ids.InsertNextId(1)
        ids.InsertNextId(9)
        ids.InsertNextId(8)

        ids.InsertNextId(4)
        ids.InsertNextId(1)
        ids.InsertNextId(2)
        ids.InsertNextId(10)
        ids.InsertNextId(9)

        ids.InsertNextId(4)
        ids.InsertNextId(2)
        ids.InsertNextId(3)
        ids.InsertNextId(11)
        ids.InsertNextId(10)

        ids.InsertNextId(4)
        ids.InsertNextId(3)
        ids.InsertNextId(4)
        ids.InsertNextId(12)
        ids.InsertNextId(11)

        ids.InsertNextId(4)
        ids.InsertNextId(4)
        ids.InsertNextId(5)
        ids.InsertNextId(13)
        ids.InsertNextId(12)

        ids.InsertNextId(4)
        ids.InsertNextId(5)
        ids.InsertNextId(6)
        ids.InsertNextId(14)
        ids.InsertNextId(13)

        ids.InsertNextId(4)
        ids.InsertNextId(6)
        ids.InsertNextId(7)
        ids.InsertNextId(15)
        ids.InsertNextId(14)

        ids.InsertNextId(4)
        ids.InsertNextId(7)
        ids.InsertNextId(0)
        ids.InsertNextId(8)
        ids.InsertNextId(15)

        self.vtk_grid.InsertNextCell(VTK_POLYHEDRON, ids)

        writer = vtkXMLUnstructuredGridWriter()
        writer.SetInputData(self.vtk_grid)
        writer.SetFileName('my_grid.vtu')
        writer.Write()

    def testCutterTriangles(self):
        cutter = vtkCutter()
        cutter.GenerateTrianglesOn() # triangle generation is enabled!
        cutter_function = vtkPlane()
        cutter_function.SetOrigin(0.0, 0.0, 0.5)
        cutter_function.SetNormal(0.0, 0.0, 1.0)
        cutter.SetCutFunction(cutter_function)
        cutter.SetInputData(self.vtk_grid)
        cutter.Update()
        surface = cutter.GetOutput()

        writer = vtkXMLPolyDataWriter()
        writer.SetInputData(surface)
        writer.SetFileName('surface_tri.vtp')
        writer.Write()

        self.assertTrue(surface.GetNumberOfCells() > 1)

    def testCutterNoTriangles(self):
        cutter = vtkCutter()
        cutter.GenerateTrianglesOff() # triangle generation is disabled!
        cutter_function = vtkPlane()
        cutter_function.SetOrigin(0.0, 0.0, 0.5)
        cutter_function.SetNormal(0.0, 0.0, 1.0)
        cutter.SetCutFunction(cutter_function)
        cutter.SetInputData(self.vtk_grid)
        cutter.Update()
        surface = cutter.GetOutput()

        writer = vtkXMLPolyDataWriter()
        writer.SetInputData(surface)
        writer.SetFileName('surface_poly.vtp')
        writer.Write()

        self.assertEqual(surface.GetNumberOfCells(), 1)

if __name__ == "__main__":
    Testing.main([(TestPolyhedronCutter, 'test')])

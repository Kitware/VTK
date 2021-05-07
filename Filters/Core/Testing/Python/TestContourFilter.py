#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestContourFilter(Testing.vtkTest):
    def setUp(self):
      poly = vtk.vtkPolyData()
      pts=vtk.vtkPoints()
      scalars=vtk.vtkDoubleArray()
      scalars.SetName('scalars')
      count=0
      for xyz in [[0,0,0],[1,0,0],[1,1,0],[0,1,0]]:
          pts.InsertNextPoint(xyz)
      for value in [0,1,2,3]:
          scalars.InsertNextValue(value)
      poly.SetPoints(pts)
      poly.GetPointData().SetScalars(scalars)
      polys=vtk.vtkCellArray()
      polys.InsertNextCell(3,[0,1,2])
      polys.InsertNextCell(3,[1,2,3])
      poly.SetPolys(polys)
      print(poly)

      self.cf = vtk.vtkContourFilter()
      self.cf.SetInputData(poly)
      self.cf.SetValue(0,0.5)
      self.cf.SetValue(1,2.5)
      self.cf.DebugOn()

    def testWithoutScalarTree(self):
      self.assertEqual(self.cf.GetScalarTree(),None)
      self.cf.Update()
      self.assertEqual(self.cf.GetScalarTree(),None)
      self.assertEqual(self.cf.GetOutput().GetNumberOfLines(),2)

    def testWithScalarTree(self):
      self.cf.UseScalarTreeOn()
      self.assertEqual(self.cf.GetScalarTree(),None)
      self.cf.Update()
      self.assertNotEqual(self.cf.GetScalarTree(),None)
      self.assertEqual(self.cf.GetOutput().GetNumberOfLines(),2)

if __name__ == "__main__":
  Testing.main([(TestContourFilter, 'test')])

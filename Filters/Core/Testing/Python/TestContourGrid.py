#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestContourGrid(Testing.vtkTest):
  def testAll(self):
    reader = vtk.vtkExodusIIReader()
    reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/disk_out_ref.ex2")
    reader.SetPointResultArrayStatus("Temp",1);
    reader.Update ();

    input = reader.GetOutput().GetBlock(0).GetBlock(0);

    cf = vtk.vtkContourFilter()
    cf.SetInputData(input)
    cf.SetValue(0,400)
    cf.SetInputArrayToProcess(0, 0, 0, 0, "Temp");
    cf.GenerateTrianglesOff()
    cf.Update()
    self.failUnlessEqual(cf.GetOutput().GetNumberOfPoints(),1047)
    self.failUnlessEqual(cf.GetOutput().GetNumberOfCells(),1028)

    cf.GenerateTrianglesOn()
    cf.Update()
    self.failUnlessEqual(cf.GetOutput().GetNumberOfPoints(),1047)
    self.failUnlessEqual(cf.GetOutput().GetNumberOfCells(),2056)

if __name__ == "__main__":
  Testing.main([(TestContourGrid, 'test')])

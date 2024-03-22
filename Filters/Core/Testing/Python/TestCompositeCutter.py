#!/usr/bin/env python

# This tests vtkCompositeCutter

from vtkmodules.vtkCommonDataModel import vtkPlane
from vtkmodules.vtkFiltersCore import vtkCompositeCutter
from vtkmodules.vtkIOAMR import vtkAMREnzoReader
from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

class TestCompositeCutter(Testing.vtkTest):
  def testAMR(self):
    filename = VTK_DATA_ROOT +"/Data/AMR/Enzo/DD0010/moving7_0010.hierarchy"

    reader = vtkAMREnzoReader()
    reader.SetFileName(filename)
    reader.SetMaxLevel(10)
    reader.SetCellArrayStatus("TotalEnergy", 1)

    plane = vtkPlane()
    plane.SetOrigin(0.5, 0.5, 0.5)
    plane.SetNormal(1, 0, 0)

    cutter = vtkCompositeCutter()
    cutter.SetCutFunction(plane)
    cutter.SetInputConnection(reader.GetOutputPort())
    cutter.Update()

    slice = cutter.GetOutputDataObject(0)
    self.assertEqual(slice.GetNumberOfCells(), 862)

if __name__ == "__main__":
    Testing.main([(TestCompositeCutter, 'test')])

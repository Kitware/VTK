#!/usr/bin/env python

# This tests vtkAMRExtractLevel

import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

class TestAMRExtractLevel(Testing.vtkTest):
  def testAMR(self):
    filename= VTK_DATA_ROOT +"/Data/AMR/Enzo/DD0010/moving7_0010.hierarchy"
    level = 1

    reader = vtk.vtkAMREnzoReader()
    reader.SetFileName(filename);
    reader.SetMaxLevel(10);
    reader.SetCellArrayStatus("TotalEnergy",1)

    filter = vtk.vtkExtractLevel()
    filter.AddLevel(level);

    filter.SetInputConnection(reader.GetOutputPort())
    filter.Update()

    amr  = reader.GetOutputDataObject(0)
    out = filter.GetOutputDataObject(0)
    self.assertEqual(out.GetNumberOfBlocks(), amr.GetNumberOfDataSets(level))

if __name__ == "__main__":
    Testing.main([(TestAMRExtractLevel, 'test')])

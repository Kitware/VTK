#!/usr/bin/env python

# This tests vtkAMRResampleFilter

import vtk
from vtk.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

def NumCells(out):
  n =0;
  for i in range(out.GetNumberOfLevels()):
    for j in range(out.GetNumberOfDataSets(i)):
      m = out.GetDataSet(i,j).GetNumberOfCells()
      #print (i,j,m)
      n = n+ m
  return n

class TestAMRSliceFilter(Testing.vtkTest):
  def testMe(self):
    filename= VTK_DATA_ROOT +"/Data/AMR/Enzo/DD0010/moving7_0010.hierarchy"
    datafieldname = "TotalEnergy"

    reader = vtk.vtkAMREnzoReader()
    reader.SetFileName(filename);
    reader.SetMaxLevel(10);
    reader.SetCellArrayStatus(datafieldname,1)

    filter = vtk.vtkAMRSliceFilter()
    filter.SetInputConnection(reader.GetOutputPort())
    filter.SetNormal(1);
    filter.SetOffSetFromOrigin(0.5);
    filter.SetMaxResolution(10);
    filter.Update()
    out = filter.GetOutputDataObject(0);
    self.assertEqual(NumCells(out),456)
    out.Audit();

if __name__ == "__main__":
  Testing.main([(TestAMRSliceFilter, 'test')])

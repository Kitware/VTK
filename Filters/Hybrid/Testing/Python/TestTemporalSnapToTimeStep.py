#!/usr/bin/env python

# This tests vtkTemporalSnapToTimeStep

import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

def Nearest(a):
  i = int(a)
  if a-i <= 0.5:
    return i
  else:
    return i+1;

class TestTemporalSnapToTimeStep(Testing.vtkTest):
  def test(self):
    source = vtk.vtkTemporalFractal()
    source.DiscreteTimeStepsOn();

    shift = vtk.vtkTemporalSnapToTimeStep()
    shift.SetInputConnection(source.GetOutputPort())

    for i in range(4):
      inTime = i*0.5+0.1
      shift.UpdateTimeStep(inTime)
      self.assertEqual(shift.GetOutputDataObject(0).GetInformation().Has(vtk.vtkDataObject.DATA_TIME_STEP()),True)
      outTime = shift.GetOutputDataObject(0).GetInformation().Get(vtk.vtkDataObject.DATA_TIME_STEP())
      self.assertEqual(outTime==Nearest(inTime),True);


if __name__ == "__main__":
    Testing.main([(TestTemporalSnapToTimeStep, 'test')])

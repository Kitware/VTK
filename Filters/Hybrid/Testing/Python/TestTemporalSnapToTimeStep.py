#!/usr/bin/env python

# This tests vtkTemporalSnapToTimeStep

from vtkmodules.vtkCommonDataModel import vtkDataObject
from vtkmodules.vtkFiltersHybrid import (
    vtkTemporalFractal,
    vtkTemporalSnapToTimeStep,
)
from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

def Nearest(a):
  i = int(a)
  if a-i <= 0.5:
    return i
  else:
    return i+1;

class TestTemporalSnapToTimeStep(Testing.vtkTest):
  def test(self):
    source = vtkTemporalFractal()
    source.DiscreteTimeStepsOn();

    shift = vtkTemporalSnapToTimeStep()
    shift.SetInputConnection(source.GetOutputPort())

    for i in range(4):
      inTime = i*0.5+0.1
      shift.UpdateTimeStep(inTime)
      self.assertEqual(shift.GetOutputDataObject(0).GetInformation().Has(vtkDataObject.DATA_TIME_STEP()),True)
      outTime = shift.GetOutputDataObject(0).GetInformation().Get(vtkDataObject.DATA_TIME_STEP())
      self.assertEqual(outTime==Nearest(inTime),True);


if __name__ == "__main__":
    Testing.main([(TestTemporalSnapToTimeStep, 'test')])

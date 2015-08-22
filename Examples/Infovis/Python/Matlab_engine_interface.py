#!/usr/bin/env python

# Python example script that uses the vtkMatlabEngineInterface to perform
# a calculation (sin(x)^2 + cos(x)^2 = 1) on VTK array data, and pass the
# result back to VTK.

# VTK must be built with VTK_USE_MATLAB_MEX turned on for this example to work!
from __future__ import print_function
from vtk import *
import math

if __name__ == "__main__":

  # Create an instance of the Matlab Engine.  Note, menginterface is not a VTK pipeline object.
  menginterface = vtkMatlabEngineInterface()

  # Create two arrays of doubles in VTK.  y contains sin(x)^2
  x = vtkDoubleArray()
  y = vtkDoubleArray()
  for d in range(0, 100):
    x.InsertNextValue(d);
    y.InsertNextValue(math.sin(d)**2)

  # Copy the x and y to Matlab with the same variable names
  menginterface.PutVtkDataArray("x", x)
  menginterface.PutVtkDataArray("y", y)

  # Calculate cos(x)^2 + sin(x)^2 = 1 in Matlab.
  menginterface.EvalString("y = y + cos(x).^2")

  # Copy y back to VTK as variable result
  result = menginterface.GetVtkDataArray("y")

  # Display contents of result, should be all ones.
  print("\n\nContents of result array copied to VTK from Matlab\n\n")
  for i in range(result.GetNumberOfTuples()):
    t = result.GetTuple1(i)
    print('result[%d] = %6.4f' % (i,t))


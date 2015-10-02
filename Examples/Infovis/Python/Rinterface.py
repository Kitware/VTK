#!/usr/bin/env python

# Python example script that uses the vtkRIinterface to create an instance of the
# R interpreter and pass it some data, modify the data in R, and pass the result
# back to VTK.

# VTK must be built with VTK_USE_GNU_R turned on for this example to work!
from __future__ import print_function
from vtk import *
import math

if __name__ == "__main__":

  # Create a character buffer to store R output echoed to the terminal
  Routput_buffer = 1000*' '

  # Create an instance of the R interpreter.  Note, rinterface is not a VTK pipeline object.
  rinterface = vtkRInterface()

  # Create an array of 10 doubles in VTK and fill it with some data
  darray = vtkDoubleArray()
  for d in range(0, 10):
    darray.InsertNextValue(math.sqrt(d));

  # Tell R to store its terminal output in our python buffer
  rinterface.OutputBuffer(Routput_buffer, len(Routput_buffer))

  # Copy the array of doubles into the R interpreter as a matrix called d, with 10 rows and 1 column.
  rinterface.AssignVTKDataArrayToRVariable(darray, "d")

  # Execute R command to echo contents of d to the terminal.
  rinterface.EvalRscript("d",1)

  # Execute a command on the R interpreter to create a matrix b with 10 rows and 1 column.
  rinterface.EvalRscript("b = matrix(sqrt(10:19),10,1)",1)

  # Execute a command on the R interpreter to column append b to d.
  rinterface.EvalRscript("d = cbind(d,b)",1);

  # Copy matrix d from R back to VTK as an array of doubles in bdarray.
  bdarray = rinterface.AssignRVariableToVTKDataArray("d")

  # Display the contents of bdarray.
  print("\n\nContents of bdarray copied to VTK from R\n\n")
  for i in range(bdarray.GetNumberOfTuples()):
    t = bdarray.GetTuple2(i)
    print('%6.4f   %6.4f' % (t[0], t[1]))

  # Display the contents of R output echoed to the terminal.
  print("\n\nOutput of R interpreter\n\n")
  print(Routput_buffer)


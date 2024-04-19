#!/usr/bin/env python

# Test reading of XML file with timesteps
from vtkmodules.vtkIOXML import vtkXMLUnstructuredGridReader
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

reader = vtkXMLUnstructuredGridReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/cube-with-time.vtu")
reader.Update()

# Ensure the reader identifies the timesteps in the file
assert reader.GetNumberOfTimeSteps() == 8

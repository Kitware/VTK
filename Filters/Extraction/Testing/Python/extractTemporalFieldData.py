#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

reader = vtk.vtkExodusIIReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/can.ex2")
reader.UpdateInformation()

for i in range(reader.GetNumberOfObjectArrays(reader.GLOBAL)):
    name = reader.GetObjectArrayName(reader.GLOBAL, i)
    reader.SetObjectArrayStatus(reader.GLOBAL, name, 1)

extractTFD = vtk.vtkExtractTemporalFieldData()
extractTFD.SetInputConnection(reader.GetOutputPort())
extractTFD.Update()

data = extractTFD.GetOutputDataObject(0)
assert data.IsA("vtkMultiBlockDataSet")

block = data.GetBlock(0).GetBlock(0)
assert block.IsA("vtkTable")
assert block.GetNumberOfRows() == reader.GetNumberOfTimeSteps()
assert block.GetNumberOfColumns() > 0

extractTFD.HandleCompositeDataBlocksIndividuallyOff()
extractTFD.Update()

data = extractTFD.GetOutputDataObject(0)
assert data.IsA("vtkTable")
assert data.GetNumberOfRows() == reader.GetNumberOfTimeSteps()
assert data.GetNumberOfColumns() > 0

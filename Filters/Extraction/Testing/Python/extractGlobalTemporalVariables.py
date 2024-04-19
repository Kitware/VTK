#!/usr/bin/env python
from vtkmodules.vtkFiltersExtraction import vtkExtractExodusGlobalTemporalVariables
from vtkmodules.vtkIOExodus import vtkExodusIIReader
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

reader = vtkExodusIIReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/can.ex2")
reader.UpdateInformation()

for i in range(reader.GetNumberOfObjectArrays(reader.GLOBAL)):
    name = reader.GetObjectArrayName(reader.GLOBAL, i)
    reader.SetObjectArrayStatus(reader.GLOBAL, name, 1)

extractTFD = vtkExtractExodusGlobalTemporalVariables()
extractTFD.SetInputConnection(reader.GetOutputPort())
extractTFD.Update()

data = extractTFD.GetOutputDataObject(0)
assert data.IsA("vtkTable")
assert data.GetNumberOfRows() == reader.GetNumberOfTimeSteps()
assert data.GetNumberOfColumns() > 0

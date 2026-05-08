from vtkmodules.vtkIOEnSight import vtkGenericEnSightReader
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

filePath = VTK_DATA_ROOT + "/Data/EnSight/numeric_path/numeric_path.case"
reader = vtkGenericEnSightReader()
reader.SetCaseFileName(filePath)

if not reader.CanReadFile(filePath):
  print("Cannot read file: " + filePath)
  exit(-1)

reader.Update()

print(reader.GetOutput())

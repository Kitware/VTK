import math
from vtkmodules.vtkIOEnSight import vtkGenericEnSightReader
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

reader = vtkGenericEnSightReader()
reader.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/UndefAndPartialAscii/grid.case")
reader.ReadAllVariablesOn()
reader.Update()

case = reader.GetOutput()
block0 = case.GetBlock(0)
mass = block0.GetCellData().GetArray("mass")

assert mass is not None and \
        mass.GetRange()[0] == 0.0 and mass.GetRange()[1] == 3.0 and \
        math.isnan(mass.GetValue(2))

pres = block0.GetPointData().GetArray("pres")
assert pres is not None and \
        pres.GetRange()[0] == 4.0 and pres.GetRange()[1] == 6.0 and \
        pres.GetValue(2) == 4 and pres.GetValue(4) == 6

for i in range(0, 10):
    if i == 2 or i == 4: continue
    assert math.isnan(pres.GetValue(i))

#!/usr/bin/env python

from vtkmodules.vtkIOXML import vtkXMLUnstructuredGridReader
from vtkmodules.vtkFiltersTemporal import vtkForceStaticMesh
from vtkmodules.vtkFiltersGeneral import vtkRandomAttributeGenerator
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

inputData = vtkXMLUnstructuredGridReader()
inputData.SetFileName(VTK_DATA_ROOT + "/Data/cube-with-time.vtu")

scalars = vtkRandomAttributeGenerator()
scalars.SetInputConnection(inputData.GetOutputPort())
scalars.GenerateAllDataOff()
scalars.GeneratePointScalarsOn()
scalars.SetDataTypeToDouble()
scalars.SetComponentRange(0, 30)

staticMesh = vtkForceStaticMesh()
staticMesh.SetInputConnection(scalars.GetOutputPort())
staticMesh.Update()

staticMesh.GetOutputDataObject(0).GetCellData().SetActiveScalars("RandomPointScalars")

# Check mesh stay the same at different time steps
initMeshTime = staticMesh.GetOutputDataObject(0).GetMeshMTime()
staticMesh.UpdateTimeStep(3)
newMeshTime = staticMesh.GetOutputDataObject(0).GetMeshMTime()
if not initMeshTime == newMeshTime:
    print("Error: static mesh has different mesh times")

mapper = vtkDataSetMapper()
mapper.SetInputConnection(staticMesh.GetOutputPort())
mapper.UseLookupTableScalarRangeOff()
mapper.SetScalarVisibility(1)
mapper.SetScalarModeToDefault()
mapper.SetScalarRange(0, 30)

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetRepresentationToSurface()

ren1 = vtkRenderer()
ren1.SetBackground(0, 0, 0)
ren1.AddActor(actor)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(300, 300)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

renWin.Render()

# enable user interface interactor
# iren SetUserMethod {wm deiconify .vtkInteract}
iren.Initialize()
# --- end of script --

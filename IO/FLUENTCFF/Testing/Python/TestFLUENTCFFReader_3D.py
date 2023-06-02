#!/usr/bin/env python
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOFLUENTCFF import vtkFLUENTCFFReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCompositePolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.vtkCommonDataModel import vtkPlane
from vtkmodules.vtkFiltersCore import vtkPlaneCutter
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Read some Fluent CFF data HDF5 form
r = vtkFLUENTCFFReader()
r.SetFileName(VTK_DATA_ROOT + "/Data/mesh_3ddp.cas.h5")
r.DisableAllCellArrays()
r.SetCellArrayStatus("phase_1-SV_VOF", 1)

# The cut plane
plane = vtkPlane()
plane.SetOrigin(0, 0, 0)
plane.SetNormal(0, 1, 0)

cut0 = vtkPlaneCutter()
cut0.SetInputConnection(r.GetOutputPort())
cut0.SetPlane(plane)
cut0.ComputeNormalsOff()

g = vtkGeometryFilter()
g.SetInputConnection(cut0.GetOutputPort())

FluentMapper = vtkCompositePolyDataMapper()
FluentMapper.SetInputConnection(g.GetOutputPort())
FluentMapper.SetScalarModeToUseCellFieldData()
FluentMapper.SelectColorArray("phase_1-SV_VOF")
FluentMapper.SetScalarRange(0, 1)
FluentActor = vtkActor()
FluentActor.GetProperty().EdgeVisibilityOn()
FluentActor.GetProperty().SetEdgeColor(1,1,1)
FluentActor.SetMapper(FluentMapper)

ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(FluentActor)
renWin.SetSize(300,300)
renWin.Render()
ren1.ResetCamera()
ren1.GetActiveCamera().SetPosition(0.3, -1.92217, 0)
ren1.GetActiveCamera().SetFocalPoint(0.3, 0, 0)
ren1.GetActiveCamera().SetViewUp(0, 0, 1)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.ResetCameraClippingRange()

iren.Initialize()
# --- end of script --

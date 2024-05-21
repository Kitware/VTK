#!/usr/bin/env python
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOGeometry import vtkFLUENTReader
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
from vtkmodules.vtkFiltersExtraction import vtkExtractBlock
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

# Read some Fluent UCD data in ASCII form
r = vtkFLUENTReader()
r.SetFileName(VTK_DATA_ROOT + "/Data/room.cas")
r.EnableAllCellArrays()

extractBlock = vtkExtractBlock()
extractBlock.AddIndex(1)
extractBlock.SetInputConnection(r.GetOutputPort())

g = vtkGeometryFilter()
g.SetInputConnection(extractBlock.GetOutputPort())

fluentMapper = vtkCompositePolyDataMapper()
fluentMapper.SetInputConnection(g.GetOutputPort())
fluentMapper.SetScalarModeToUseCellFieldData()
fluentMapper.SelectColorArray("PRESSURE")
fluentMapper.SetScalarRange(-31, 44)

fluentActor = vtkActor()
fluentActor.SetMapper(fluentMapper)

ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(fluentActor)
renWin.SetSize(300,300)
renWin.Render()
ren1.ResetCamera()
ren1.GetActiveCamera().SetPosition(2, 2, 11)
ren1.GetActiveCamera().SetFocalPoint(2, 2, 0)
ren1.GetActiveCamera().SetViewUp(0, 1, 0)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.ResetCameraClippingRange()

iren.Initialize()
# --- end of script --

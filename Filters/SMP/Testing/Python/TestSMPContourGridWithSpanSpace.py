#!/usr/bin/env python
from vtkmodules.vtkCommonExecutionModel import vtkSpanSpace
from vtkmodules.vtkFiltersCore import vtkAssignAttribute
from vtkmodules.vtkFiltersSMP import vtkSMPContourGrid
from vtkmodules.vtkIOExodus import vtkExodusIIReader
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
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Debugging parameters
sze = 300

reader = vtkExodusIIReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/disk_out_ref.ex2")
reader.UpdateInformation()
reader.SetPointResultArrayStatus("CH4", 1)
reader.Update()

tree = vtkSpanSpace()

contour = vtkSMPContourGrid()
#contour = vtkContourGrid()
contour.SetInputConnection(reader.GetOutputPort())
contour.SetInputArrayToProcess(0, 0, 0, vtkAssignAttribute.POINT_DATA, "CH4")
contour.SetValue(0, 0.000718448)
contour.MergePiecesOff()
contour.UseScalarTreeOn()
contour.SetScalarTree(tree)
contour.Update()

mapper = vtkCompositePolyDataMapper()
mapper.SetInputConnection(contour.GetOutputPort())
mapper.ScalarVisibilityOff()

actor = vtkActor()
actor.SetMapper(mapper)

# Create graphics stuff
#
ren1 = vtkRenderer()
ren1.AddActor(actor)
ren1.ResetCamera()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(sze, sze)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

renWin.Render()
iren.Start()

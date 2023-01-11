#!/usr/bin/env python
from vtkmodules.vtkFiltersSources import vtkDiskSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
disk = vtkDiskSource()
disk.SetInnerRadius(1.0)
disk.SetOuterRadius(2.0)
disk.SetRadialResolution(1)
disk.SetCircumferentialResolution(20)
diskMapper = vtkPolyDataMapper()
diskMapper.SetInputConnection(disk.GetOutputPort())
diskActor = vtkActor()
diskActor.SetMapper(diskMapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(diskActor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(200,200)
# Get handles to some useful objects
#
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --

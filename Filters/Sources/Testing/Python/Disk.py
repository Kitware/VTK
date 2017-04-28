#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
disk = vtk.vtkDiskSource()
disk.SetInnerRadius(1.0)
disk.SetOuterRadius(2.0)
disk.SetRadialResolution(1)
disk.SetCircumferentialResolution(20)
diskMapper = vtk.vtkPolyDataMapper()
diskMapper.SetInputConnection(disk.GetOutputPort())
diskActor = vtk.vtkActor()
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

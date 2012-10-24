#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.StereoCapableWindowOn()
renWin.SetWindowName("vtk - Mace")
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
renWin.SetStereoTypeToDresden()
renWin.StereoRenderOn()
# create a sphere source and actor
#
sphere = vtk.vtkSphereSource()
sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereMapper.GlobalImmediateModeRenderingOn()
sphereActor = vtk.vtkLODActor()
sphereActor.SetMapper(sphereMapper)
# create the spikes using a cone source and the sphere source
#
cone = vtk.vtkConeSource()
glyph = vtk.vtkGlyph3D()
glyph.SetInputConnection(sphere.GetOutputPort())
glyph.SetSourceConnection(cone.GetOutputPort())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.25)
spikeMapper = vtk.vtkPolyDataMapper()
spikeMapper.SetInputConnection(glyph.GetOutputPort())
spikeActor = vtk.vtkLODActor()
spikeActor.SetMapper(spikeMapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(sphereActor)
ren1.AddActor(spikeActor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)
# render the image
#
ren1.ResetCamera()
cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()
# default arguments added so that the protoype matches
# as required in Python when the test is translated.
def TkCheckAbort (a=0,b=0,__vtk__temp0=0,__vtk__temp1=0):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)
        pass

renWin.AddObserver("AbortCheckEvent",TkCheckAbort)
# prevent the tk window from showing up then start the event loop
mat = vtk.vtkMatrix4x4()
spikeActor.SetUserMatrix(mat)
renWin.Render()
# --- end of script --

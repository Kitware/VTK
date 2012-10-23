#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
aPlane = vtk.vtkPlaneSource()
aPlane.SetCenter(-100,-100,-100)
aPlane.SetOrigin(-100,-100,-100)
aPlane.SetPoint1(-90,-100,-100)
aPlane.SetPoint2(-100,-90,-100)
aPlane.SetNormal(0,-1,1)
imageIn = vtk.vtkPNMReader()
imageIn.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/earth.ppm")
texture = vtk.vtkTexture()
texture.SetInputConnection(imageIn.GetOutputPort())
texturePlane = vtk.vtkTextureMapToPlane()
texturePlane.SetInputConnection(aPlane.GetOutputPort())
texturePlane.AutomaticPlaneGenerationOn()
planeMapper = vtk.vtkPolyDataMapper()
planeMapper.SetInputConnection(texturePlane.GetOutputPort())
texturedPlane = vtk.vtkActor()
texturedPlane.SetMapper(planeMapper)
texturedPlane.SetTexture(texture)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(texturedPlane)
#ren1 SetBackground 1 1 1
renWin.SetSize(200,200)
renWin.Render()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --

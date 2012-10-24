#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create pipeline
#
reader = vtk.vtkPolyDataReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/vtk.vtk")
# Read a ruler texture
r = vtk.vtkPNGReader()
r.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/ruler.png")
atext = vtk.vtkTexture()
atext.SetInputConnection(r.GetOutputPort())
atext.InterpolateOn()
# produce some ribbons
ribbon = vtk.vtkRibbonFilter()
ribbon.SetInputConnection(reader.GetOutputPort())
ribbon.SetWidth(0.1)
ribbon.SetGenerateTCoordsToUseLength()
ribbon.SetTextureLength(1.0)
ribbon.UseDefaultNormalOn()
ribbon.SetDefaultNormal(0,0,1)
ribbonMapper = vtk.vtkPolyDataMapper()
ribbonMapper.SetInputConnection(ribbon.GetOutputPort())
ribbonActor = vtk.vtkActor()
ribbonActor.SetMapper(ribbonMapper)
ribbonActor.GetProperty().SetColor(1,1,0)
ribbonActor.SetTexture(atext)
# produce some tubes
tuber = vtk.vtkTubeFilter()
tuber.SetInputConnection(reader.GetOutputPort())
tuber.SetRadius(0.1)
tuber.SetNumberOfSides(12)
tuber.SetGenerateTCoordsToUseLength()
tuber.SetTextureLength(0.5)
tuber.CappingOn()
tubeMapper = vtk.vtkPolyDataMapper()
tubeMapper.SetInputConnection(tuber.GetOutputPort())
tubeActor = vtk.vtkActor()
tubeActor.SetMapper(tubeMapper)
tubeActor.GetProperty().SetColor(1,1,0)
tubeActor.SetTexture(atext)
tubeActor.AddPosition(5,0,0)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(ribbonActor)
ren1.AddActor(tubeActor)
ren1.SetBackground(1,1,1)
renWin.SetSize(900,350)
ren1.SetBackground(1,1,1)
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(4)
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
threshold = 15
# --- end of script --

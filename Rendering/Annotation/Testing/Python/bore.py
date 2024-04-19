#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkTubeFilter
from vtkmodules.vtkIOLegacy import vtkPolyDataReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingAnnotation import vtkArcPlotter
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create arc plots
# get the interactor ui
camera = vtkCamera()
# read the bore
bore = vtkPolyDataReader()
bore.SetFileName(VTK_DATA_ROOT + "/Data/bore.vtk")
tuber = vtkTubeFilter()
tuber.SetInputConnection(bore.GetOutputPort())
tuber.SetNumberOfSides(6)
tuber.SetRadius(15)
mapBore = vtkPolyDataMapper()
mapBore.SetInputConnection(tuber.GetOutputPort())
mapBore.ScalarVisibilityOff()
boreActor = vtkActor()
boreActor.SetMapper(mapBore)
boreActor.GetProperty().SetColor(0,0,0)
# create the arc plots
#
track1 = vtkPolyDataReader()
track1.SetFileName(VTK_DATA_ROOT + "/Data/track1.binary.vtk")
ap = vtkArcPlotter()
ap.SetInputConnection(track1.GetOutputPort())
ap.SetCamera(camera)
ap.SetRadius(250.0)
ap.SetHeight(200.0)
ap.UseDefaultNormalOn()
ap.SetDefaultNormal(1,1,0)
mapArc = vtkPolyDataMapper()
mapArc.SetInputConnection(ap.GetOutputPort())
arcActor = vtkActor()
arcActor.SetMapper(mapArc)
arcActor.GetProperty().SetColor(0,1,0)
track2 = vtkPolyDataReader()
track2.SetFileName(VTK_DATA_ROOT + "/Data/track2.binary.vtk")
ap2 = vtkArcPlotter()
ap2.SetInputConnection(track2.GetOutputPort())
ap2.SetCamera(camera)
ap2.SetRadius(450.0)
ap2.SetHeight(200.0)
ap2.UseDefaultNormalOn()
ap2.SetDefaultNormal(1,1,0)
mapArc2 = vtkPolyDataMapper()
mapArc2.SetInputConnection(ap2.GetOutputPort())
arcActor2 = vtkActor()
arcActor2.SetMapper(mapArc2)
arcActor2.GetProperty().SetColor(0,0,1)
track3 = vtkPolyDataReader()
track3.SetFileName(VTK_DATA_ROOT + "/Data/track3.binary.vtk")
ap3 = vtkArcPlotter()
ap3.SetInputConnection(track3.GetOutputPort())
ap3.SetCamera(camera)
ap3.SetRadius(250.0)
ap3.SetHeight(50.0)
ap3.SetDefaultNormal(1,1,0)
mapArc3 = vtkPolyDataMapper()
mapArc3.SetInputConnection(ap3.GetOutputPort())
arcActor3 = vtkActor()
arcActor3.SetMapper(mapArc3)
arcActor3.GetProperty().SetColor(1,0,1)
# Create graphics objects
# Create the rendering window renderer and interactive renderer
ren1 = vtkRenderer()
ren1.SetActiveCamera(camera)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetMultiSamples(0)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer set the background and size
ren1.AddActor(boreActor)
ren1.AddActor(arcActor)
ren1.AddActor(arcActor2)
ren1.AddActor(arcActor3)
ren1.SetBackground(1,1,1)
renWin.SetSize(230,500)
camera.SetClippingRange(14144,32817)
camera.SetFocalPoint(-1023,680,5812)
camera.SetPosition(15551,-2426,19820)
camera.SetViewUp(-0.651889,-0.07576,0.754521)
camera.SetViewAngle(20)
renWin.Render()
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --

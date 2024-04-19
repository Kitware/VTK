#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkPlane,
    vtkPlaneCollection,
)
from vtkmodules.vtkFiltersCore import vtkStripper
from vtkmodules.vtkFiltersGeneral import vtkClipClosedSurface
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
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

#
# Demonstrate the use of clipping and capping on polyhedral data
#
# create a sphere and clip it
#
sphere = vtkSphereSource()
sphere.SetRadius(1)
sphere.SetPhiResolution(10)
sphere.SetThetaResolution(10)
plane1 = vtkPlane()
plane1.SetOrigin(0.3,0.3,0.3)
plane1.SetNormal(-1,-1,-1)
plane2 = vtkPlane()
plane2.SetOrigin(0.5,0,0)
plane2.SetNormal(-1,0,0)
planes = vtkPlaneCollection()
planes.AddItem(plane1)
planes.AddItem(plane2)
# stripper just increases coverage
stripper = vtkStripper()
stripper.SetInputConnection(sphere.GetOutputPort())
clipper = vtkClipClosedSurface()
clipper.SetInputConnection(stripper.GetOutputPort())
clipper.SetClippingPlanes(planes)
clipperOutline = vtkClipClosedSurface()
clipperOutline.SetInputConnection(stripper.GetOutputPort())
clipperOutline.SetClippingPlanes(planes)
clipperOutline.GenerateFacesOff()
clipperOutline.GenerateOutlineOn()
sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(clipper.GetOutputPort())
clipperOutlineMapper = vtkPolyDataMapper()
clipperOutlineMapper.SetInputConnection(clipperOutline.GetOutputPort())
clipActor = vtkActor()
clipActor.SetMapper(sphereMapper)
clipActor.GetProperty().SetColor(0.8,0.05,0.2)
clipOutlineActor = vtkActor()
clipOutlineActor.SetMapper(clipperOutlineMapper)
clipOutlineActor.GetProperty().SetColor(0,1,0)
clipOutlineActor.SetPosition(0.001,0.001,0.001)
# create an outline
outline = vtkOutlineFilter()
outline.SetInputConnection(sphere.GetOutputPort())
outline.GenerateFacesOn()
outlineClip = vtkClipClosedSurface()
outlineClip.SetClippingPlanes(planes)
outlineClip.SetInputConnection(outline.GetOutputPort())
outlineClip.GenerateFacesOff()
outlineClip.GenerateOutlineOn()
outlineClip.SetScalarModeToColors()
outlineClip.SetClipColor(0,1,0)
outlineMapper = vtkDataSetMapper()
outlineMapper.SetInputConnection(outlineClip.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.SetPosition(0.001,0.001,0.001)
# Create graphics stuff
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetMultiSamples(0)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(clipActor)
ren1.AddActor(clipOutlineActor)
ren1.AddActor(outlineActor)
ren1.SetBackground(1,1,1)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(30)
ren1.GetActiveCamera().Dolly(1.2)
ren1.ResetCameraClippingRange()
renWin.SetSize(300,300)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --

#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkPlane,
    vtkPlaneCollection,
)
from vtkmodules.vtkFiltersGeneral import (
    vtkClipClosedSurface,
    vtkImageMarchingCubes,
)
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkIOImage import vtkVolume16Reader
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
# create pipeline
#
v16 = vtkVolume16Reader()
v16.SetDataDimensions(64,64)
v16.GetOutput().SetOrigin(0.0,0.0,0.0)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
v16.SetImageRange(1,93)
v16.SetDataSpacing(3.2,3.2,1.5)
v16.Update()
iso = vtkImageMarchingCubes()
iso.SetInputConnection(v16.GetOutputPort())
iso.SetValue(0,1150)
iso.SetInputMemoryLimit(1000)
topPlane = vtkPlane()
topPlane.SetNormal(0,0,1)
topPlane.SetOrigin(0,0,0.5)
botPlane = vtkPlane()
botPlane.SetNormal(0,0,-1)
botPlane.SetOrigin(0,0,137.0)
sagPlane = vtkPlane()
sagPlane.SetNormal(1,0,0)
sagPlane.SetOrigin(100.8,0,0)
capPlanes = vtkPlaneCollection()
capPlanes.AddItem(topPlane)
capPlanes.AddItem(botPlane)
capPlanes.AddItem(sagPlane)
clip = vtkClipClosedSurface()
clip.SetClippingPlanes(capPlanes)
clip.SetInputConnection(iso.GetOutputPort())
clip.SetBaseColor(0.9804,0.9216,0.8431)
clip.SetClipColor(1.0,1.0,1.0)
clip.SetActivePlaneColor(1.0,1.0,0.8)
clip.SetActivePlaneId(2)
clip.SetScalarModeToColors()
clip.GenerateOutlineOn()
clip.GenerateFacesOn()
isoMapper = vtkPolyDataMapper()
isoMapper.SetInputConnection(clip.GetOutputPort())
isoMapper.ScalarVisibilityOn()
isoActor = vtkActor()
isoActor.SetMapper(isoMapper)
outline = vtkOutlineFilter()
outline.SetInputConnection(v16.GetOutputPort())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.VisibilityOff()
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(isoActor)
ren1.SetBackground(0.2,0.3,0.4)
renWin.SetSize(200,200)
ren1.ResetCamera()
ren1.GetActiveCamera().Elevation(90)
ren1.GetActiveCamera().SetViewUp(0,0,-1)
ren1.GetActiveCamera().Azimuth(270)
ren1.ResetCameraClippingRange()
renWin.Render()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --

#!/usr/bin/env python

# Demonstrate how to use the vtkBoxWidget to control volume rendering
# within the interior of the widget.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Load a volume, use the widget to control what's volume
# rendered. Basically the idea is that the vtkBoxWidget provides a box
# which clips the volume rendering.
v16 = vtk.vtkVolume16Reader()
v16.SetDataDimensions(64, 64)
v16.GetOutput().SetOrigin(0.0, 0.0, 0.0)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix(VTK_DATA_ROOT+ "/Data/headsq/quarter")
v16.SetImageRange(1, 93)
v16.SetDataSpacing(3.2, 3.2, 1.5)

tfun = vtk.vtkPiecewiseFunction()
tfun.AddPoint(70.0, 0.0)
tfun.AddPoint(599.0, 0)
tfun.AddPoint(600.0, 0)
tfun.AddPoint(1195.0, 0)
tfun.AddPoint(1200, .2)
tfun.AddPoint(1300, .3)
tfun.AddPoint(2000, .3)
tfun.AddPoint(4095.0, 1.0)

ctfun = vtk.vtkColorTransferFunction()
ctfun.AddRGBPoint(0.0, 0.5, 0.0, 0.0)
ctfun.AddRGBPoint(600.0, 1.0, 0.5, 0.5)
ctfun.AddRGBPoint(1280.0, 0.9, 0.2, 0.3)
ctfun.AddRGBPoint(1960.0, 0.81, 0.27, 0.1)
ctfun.AddRGBPoint(4095.0, 0.5, 0.5, 0.5)

compositeFunction = vtk.vtkVolumeRayCastCompositeFunction()

volumeMapper = vtk.vtkVolumeRayCastMapper()
volumeMapper.SetInput(v16.GetOutput())
volumeMapper.SetVolumeRayCastFunction(compositeFunction)

volumeProperty = vtk.vtkVolumeProperty()
volumeProperty.SetColor(ctfun)
volumeProperty.SetScalarOpacity(tfun)
volumeProperty.SetInterpolationTypeToLinear()
volumeProperty.ShadeOn()

newvol = vtk.vtkVolume()
newvol.SetMapper(volumeMapper)
newvol.SetProperty(volumeProperty)

outline = vtk.vtkOutlineFilter()
outline.SetInput(v16.GetOutput())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# The SetInteractor method is how 3D widgets are associated with the
# render window interactor. Internally, SetInteractor sets up a bunch
# of callbacks using the Command/Observer mechanism (AddObserver()).
boxWidget = vtk.vtkBoxWidget()
boxWidget.SetInteractor(iren)
boxWidget.SetPlaceFactor(1.0)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
ren.AddVolume(newvol)

ren.SetBackground(0, 0, 0)
renWin.SetSize(300, 300)

# When interaction starts, the requested frame rate is increased.
def StartInteraction(obj, event):
    global renWin
    renWin.SetDesiredUpdateRate(10)

# When interaction ends, the requested frame rate is decreased to
# normal levels. This causes a full resolution render to occur.
def EndInteraction(obj, event):
    global renWin
    renWin.SetDesiredUpdateRate(0.001)

# The implicit function vtkPlanes is used in conjunction with the
# volume ray cast mapper to limit which portion of the volume is
# volume rendered.
planes = vtk.vtkPlanes()
def ClipVolumeRender(obj, event):
    global planes, volumeMapper
    obj.GetPlanes(planes)
    volumeMapper.SetClippingPlanes(planes)
 

# Place the interactor initially. The output of the reader is used to
# place the box widget.
boxWidget.SetInput(v16.GetOutput())
boxWidget.PlaceWidget()
boxWidget.InsideOutOn()
boxWidget.AddObserver("StartInteractionEvent", StartInteraction)
boxWidget.AddObserver("InteractionEvent", ClipVolumeRender)
boxWidget.AddObserver("EndInteractionEvent", EndInteraction)

outlineProperty = boxWidget.GetOutlineProperty()
outlineProperty.SetRepresentationToWireframe()
outlineProperty.SetAmbient(1.0)
outlineProperty.SetAmbientColor(1, 1, 1)
outlineProperty.SetLineWidth(3)

selectedOutlineProperty = boxWidget.GetSelectedOutlineProperty()
selectedOutlineProperty.SetRepresentationToWireframe()
selectedOutlineProperty.SetAmbient(1.0)
selectedOutlineProperty.SetAmbientColor(1, 0, 0)
selectedOutlineProperty.SetLineWidth(3)


iren.Initialize()
renWin.Render()
iren.Start()

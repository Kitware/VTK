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
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline
#
v16 = vtk.vtkVolume16Reader()
v16.SetDataDimensions(64,64)
v16.GetOutput().SetOrigin(0.0,0.0,0.0)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
v16.SetImageRange(1,93)
v16.SetDataSpacing(3.2,3.2,1.5)
v16.Update()
iso = vtk.vtkImageMarchingCubes()
iso.SetInputConnection(v16.GetOutputPort())
iso.SetValue(0,1150)
iso.SetInputMemoryLimit(1000)
isoMapper = vtk.vtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.ScalarVisibilityOff()
isoMapper.ImmediateModeRenderingOn()
isoActor = vtk.vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(antique_white)
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(v16.GetOutputPort())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
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
ren1.GetActiveCamera().Azimuth(180)
ren1.ResetCameraClippingRange()
renWin.Render()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --

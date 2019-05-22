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

# create pipeline
#
v16 = vtk.vtkVolume16Reader()
v16.SetDataDimensions(64, 64)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
v16.SetImageRange(1, 93)
v16.SetDataSpacing(3.2, 3.2, 1.5)
v16.Update()

iso = vtk.vtkMarchingCubes()
iso.SetInputData(v16.GetOutput())
iso.ComputeNormalsOn()
iso.SetValue(0, 1150)

isoMapper = vtk.vtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.ScalarVisibilityOff()

isoActor = vtk.vtkActor()
isoActor.SetMapper(isoMapper)

# Add the actor to the renderer, set the background and size
#
ren1.AddActor(isoActor)
ren1.SetBackground(0.2, 0.3, 0.4)

renWin.SetSize(200, 200)

ren1.ResetCamera()
ren1.GetActiveCamera().Elevation(90)
ren1.GetActiveCamera().SetViewUp(0, 0, -1)
ren1.GetActiveCamera().Azimuth(180)
ren1.ResetCameraClippingRange()

renWin.Render()

# render the image
#
iren.Initialize()
#iren.Start()

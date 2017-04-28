#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


# create a rendering window
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)

renWin.SetSize(200, 200)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

wavelet = vtk.vtkRTAnalyticSource()
wavelet.SetWholeExtent(-100, 100, -100, 100, 0, 0)
wavelet.SetCenter(0, 0, 0)
wavelet.SetMaximum(255)
wavelet.SetStandardDeviation(.5)
wavelet.SetXFreq(60)
wavelet.SetYFreq(30)
wavelet.SetZFreq(40)
wavelet.SetXMag(10)
wavelet.SetYMag(18)
wavelet.SetZMag(5)
wavelet.SetSubsampleRate(1)

warp = vtk.vtkWarpScalar()
warp.SetInputConnection(wavelet.GetOutputPort())

mapper = vtk.vtkDataSetMapper()
mapper.SetInputConnection(warp.GetOutputPort())
mapper.SetScalarRange(75, 290)

actor = vtk.vtkActor()
actor.SetMapper(mapper)

renderer = vtk.vtkRenderer()
renderer.AddActor(actor)
renderer.ResetCamera()
renderer.GetActiveCamera().Elevation(-10)

renWin.AddRenderer(renderer)

# render the image
#
iren.Initialize()
#iren.Start()

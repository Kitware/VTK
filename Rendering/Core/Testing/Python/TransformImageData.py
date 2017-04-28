#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create a rendering window
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.SetSize(200,200)
wavelet = vtk.vtkRTAnalyticSource()
wavelet.SetWholeExtent(-100,100,-100,100,0,0)
wavelet.SetCenter(0,0,0)
wavelet.SetMaximum(255)
wavelet.SetStandardDeviation(.5)
wavelet.SetXFreq(60)
wavelet.SetYFreq(30)
wavelet.SetZFreq(40)
wavelet.SetXMag(10)
wavelet.SetYMag(18)
wavelet.SetZMag(5)
wavelet.SetSubsampleRate(1)
# linear transform
transform = vtk.vtkTransform()
transform.RotateZ(35)
transformFilter = vtk.vtkTransformFilter()
transformFilter.SetInputConnection(wavelet.GetOutputPort())
transformFilter.SetTransform(transform)
mapper = vtk.vtkDataSetMapper()
mapper.SetInputConnection(transformFilter.GetOutputPort())
mapper.SetScalarRange(75,290)
actor = vtk.vtkActor()
actor.SetMapper(mapper)
renderer = vtk.vtkRenderer()
renderer.AddActor(actor)
renderer.ResetCamera()
renWin.AddRenderer(renderer)
# --- end of script --

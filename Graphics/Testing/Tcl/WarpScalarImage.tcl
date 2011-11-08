package require vtk

# create a rendering window
vtkRenderWindow renWin
renWin SetMultiSamples 0
renWin SetSize 200 200

vtkRTAnalyticSource wavelet
  wavelet SetWholeExtent -100 100 -100 100 0 0
  wavelet SetCenter 0 0 0
  wavelet SetMaximum 255
  wavelet SetStandardDeviation .5
  wavelet SetXFreq 60
  wavelet SetYFreq 30
  wavelet SetZFreq 40
  wavelet SetXMag 10
  wavelet SetYMag 18
  wavelet SetZMag 5
  wavelet SetSubsampleRate 1

vtkWarpScalar warp
warp SetInputConnection [wavelet GetOutputPort]

vtkDataSetMapper mapper
mapper SetInputConnection [warp GetOutputPort]
mapper SetScalarRange 75 290

vtkActor actor
actor SetMapper mapper

vtkRenderer renderer
renderer AddActor actor
renderer ResetCamera
[renderer GetActiveCamera] Elevation -10
renWin AddRenderer renderer

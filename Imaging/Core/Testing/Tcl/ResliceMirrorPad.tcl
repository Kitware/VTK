package require vtk

# this script tests vtkImageReslice with different interpolation modes,
# with the wrap-pad feature turned on

# Image pipeline
vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetDataSpacing 3.2 3.2 1.5
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImageReslice reslice1
reslice1 SetInputConnection [reader GetOutputPort]
reslice1 MirrorOn
reslice1 SetInterpolationModeToCubic
reslice1 SetResliceAxesDirectionCosines 0 1 0  -1 0 0  0 0 1
reslice1 SetResliceAxesOrigin 0 0 40
reslice1 SetOutputSpacing 2.0 2.0 1.5
reslice1 SetOutputOrigin -32 -32 0
reslice1 SetOutputExtent 0 127 0 127 0 0

vtkImageReslice reslice2
reslice2 SetInputConnection [reader GetOutputPort]
reslice2 MirrorOn
reslice2 SetResliceAxesDirectionCosines 0 1 0  -1 0 0  0 0 1
reslice2 SetResliceAxesOrigin 0 0 40
reslice2 SetInterpolationModeToLinear
reslice2 SetOutputSpacing 2.0 2.0 1.5
reslice2 SetOutputOrigin -32 -32 0
reslice2 SetOutputExtent 0 127 0 127 0 0

vtkImageReslice reslice3
reslice3 SetInputConnection [reader GetOutputPort]
reslice3 MirrorOn
reslice3 SetResliceAxesDirectionCosines 0 1 0  -1 0 0  0 0 1
reslice3 SetResliceAxesOrigin 0 0 40
reslice3 SetInterpolationModeToNearestNeighbor
reslice3 SetOutputSpacing 2.0 2.0 1.5
reslice3 SetOutputOrigin -32 -32 0
reslice3 SetOutputExtent 0 127 0 127 0 0

vtkImageReslice reslice4
reslice4 SetInputConnection [reader GetOutputPort]
reslice4 MirrorOn
reslice4 SetResliceAxesDirectionCosines 0 1 0  -1 0 0  0 0 1
reslice4 SetResliceAxesOrigin 0 0 40
reslice4 SetInterpolationModeToLinear
reslice4 SetOutputSpacing 3.2 3.2 1.5
reslice4 SetOutputOrigin -102.4 -102.4 0
reslice4 SetOutputExtent 0 127 0 127 0 0

vtkImageMapper mapper1
  mapper1 SetInputConnection [reslice1 GetOutputPort]
  mapper1 SetColorWindow 2000
  mapper1 SetColorLevel 1000
  mapper1 SetZSlice 0

vtkImageMapper mapper2
  mapper2 SetInputConnection [reslice2 GetOutputPort]
  mapper2 SetColorWindow 2000
  mapper2 SetColorLevel 1000
  mapper2 SetZSlice 0

vtkImageMapper mapper3
  mapper3 SetInputConnection [reslice3 GetOutputPort]
  mapper3 SetColorWindow 2000
  mapper3 SetColorLevel 1000
  mapper3 SetZSlice 0

vtkImageMapper mapper4
  mapper4 SetInputConnection [reslice4 GetOutputPort]
  mapper4 SetColorWindow 2000
  mapper4 SetColorLevel 1000
  mapper4 SetZSlice 0

vtkActor2D actor1
  actor1 SetMapper mapper1

vtkActor2D actor2
  actor2 SetMapper mapper2

vtkActor2D actor3
  actor3 SetMapper mapper3

vtkActor2D actor4
  actor4 SetMapper mapper4

vtkRenderer imager1
  imager1 AddActor2D actor1
  imager1 SetViewport 0.5 0.0 1.0 0.5

vtkRenderer imager2
  imager2 AddActor2D actor2
  imager2 SetViewport 0.0 0.0 0.5 0.5

vtkRenderer imager3
  imager3 AddActor2D actor3
  imager3 SetViewport 0.5 0.5 1.0 1.0

vtkRenderer imager4
  imager4 AddActor2D actor4
  imager4 SetViewport 0.0 0.5 0.5 1.0

vtkRenderWindow imgWin
  imgWin AddRenderer imager1
  imgWin AddRenderer imager2
  imgWin AddRenderer imager3
  imgWin AddRenderer imager4
  imgWin SetSize 256 256

imgWin Render




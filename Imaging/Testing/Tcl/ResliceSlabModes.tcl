package require vtk

# this script tests vtkImageReslice with different slab modes

# Image pipeline
vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataScalarTypeToUnsignedShort
reader SetDataExtent 0 63 0 63 1 93
reader SetDataSpacing 3.2 3.2 1.5
reader SetDataOrigin -100.8 -100.8 -70.5
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImageCast caster
caster SetInputConnection [reader GetOutputPort]
caster SetOutputScalarTypeToFloat

vtkTransform t1
t1 RotateY 75

vtkTransform t2
t2 RotateZ 90

vtkImageReslice reslice1
reslice1 SetInputConnection [reader GetOutputPort]
reslice1 SetSlabModeToMean
reslice1 SetSlabNumberOfSlices 45
reslice1 SetInterpolationModeToLinear
reslice1 SetOutputDimensionality 2
reslice1 SetOutputSpacing 3.2 3.2 1.5
reslice1 SetOutputExtent 0 63 0 63 0 0

vtkImageReslice reslice2
reslice2 SetInputConnection [caster GetOutputPort]
reslice2 SetSlabModeToSum
reslice2 SetSlabNumberOfSlices 93
reslice2 SetInterpolationModeToLinear
reslice2 SetResliceAxes [t1 GetMatrix]
reslice2 SetOutputDimensionality 2
reslice2 SetOutputSpacing 3.2 3.2 1.5
reslice2 SetOutputExtent 0 63 0 63 0 0

vtkImageReslice reslice3
reslice3 SetInputConnection [reader GetOutputPort]
reslice3 SetSlabModeToMax
reslice3 SetInterpolationModeToLinear
reslice3 SetSlabNumberOfSlices 50
reslice3 SetResliceAxes [t1 GetMatrix]
reslice3 SetResliceTransform t2
reslice3 SetOutputDimensionality 2
reslice3 SetOutputSpacing 3.2 3.2 1.5
reslice3 SetOutputExtent 0 63 0 63 0 0

vtkImageReslice reslice4
reslice4 SetInputConnection [reader GetOutputPort]
reslice4 SetSlabModeToMin
reslice4 SetSlabNumberOfSlices 11
reslice4 SetInterpolationModeToCubic
reslice4 SetResliceTransform t2
reslice4 SetOutputDimensionality 2
reslice4 SetOutputSpacing 3.2 3.2 1.5
reslice4 SetOutputExtent 0 63 0 63 0 0

vtkImageMapper mapper1
  mapper1 SetInputConnection [reslice1 GetOutputPort]
  mapper1 SetColorWindow 2000
  mapper1 SetColorLevel 1000
  mapper1 SetZSlice 0

vtkImageMapper mapper2
  mapper2 SetInputConnection [reslice2 GetOutputPort]
  mapper2 SetColorWindow 50000
  mapper2 SetColorLevel 100000
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
  imgWin SetSize 150 128

imgWin Render

wm withdraw .

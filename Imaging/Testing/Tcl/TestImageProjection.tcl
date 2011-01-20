package require vtk

# this script tests vtkImageProjection with various axes permutations,
# in order to cover a nasty set of "if" statements that check
# the intersections of the raster lines with the input bounding box.

# Image pipeline
vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 63 0 63 1 93
reader SetDataSpacing 3.2 3.2 1.5
reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
reader SetDataMask 0x7fff

vtkImageProjection projection1
projection1 SetInputConnection [reader GetOutputPort]
projection1 SetOperationToAverage
projection1 SetSliceDirectionToZ

vtkImageProjection projection2
projection2 SetInputConnection [reader GetOutputPort]
projection2 SetOperationToMinimum
projection2 SetOperationToMaximum
projection2 MultiSliceOutputOff
projection2 SetOutputScalarTypeToInputScalarType

vtkImageProjection projection3
projection3 SetInputConnection [reader GetOutputPort]
projection3 SetOperationToSum
projection3 SetSliceDirectionToX
projection3 MultiSliceOutputOn
projection3 SetOutputScalarTypeToDouble

vtkImageReslice reslice3
reslice3 SetInputConnection [projection3 GetOutputPort]
reslice3 SetResliceAxesDirectionCosines 0 1 0  0 0 -1  1 0 0
reslice3 SetOutputSpacing 3.2 3.2 3.2
reslice3 SetOutputExtent 0 74 0 74 0 0

vtkImageProjection projection4
projection4 SetInputConnection [reader GetOutputPort]
projection4 SetOperationToMaximum
projection4 SetSliceDirection 0
projection4 MultiSliceOutputOn
projection4 SetOutputScalarTypeToFloat

vtkImageReslice reslice4
reslice4 SetInputConnection [projection4 GetOutputPort]
reslice4 SetResliceAxesDirectionCosines 0 1 0  0 0 -1  1 0 0
reslice4 SetOutputSpacing 3.2 3.2 3.2
reslice4 SetOutputExtent 0 74 0 74 0 0

vtkImageProjection projection5
projection5 SetInputConnection [reader GetOutputPort]
projection5 SetOperationToAverage
projection5 SetSliceDirectionToY
projection5 MultiSliceOutputOn

vtkImageReslice reslice5
reslice5 SetInputConnection [projection5 GetOutputPort]
reslice5 SetResliceAxesDirectionCosines 1 0 0  0 0 -1  0 1 0
reslice5 SetOutputSpacing 3.2 3.2 3.2
reslice5 SetOutputExtent 0 74 0 74 0 0

vtkImageProjection projection6
projection6 SetInputConnection [reader GetOutputPort]
projection6 SetOperationToMaximum
projection6 SetSliceDirection 1
projection6 MultiSliceOutputOn

vtkImageReslice reslice6
reslice6 SetInputConnection [projection6 GetOutputPort]
reslice6 SetResliceAxesDirectionCosines 1 0 0  0 0 -1  0 1 0
reslice6 SetOutputSpacing 3.2 3.2 3.2
reslice6 SetOutputExtent 0 74 0 74 0 0

vtkImageMapper mapper1
  mapper1 SetInputConnection [projection1 GetOutputPort]
  mapper1 SetColorWindow 2000
  mapper1 SetColorLevel 1000
  mapper1 SetZSlice 0

vtkImageMapper mapper2
  mapper2 SetInputConnection [projection2 GetOutputPort]
  mapper2 SetColorWindow 2000
  mapper2 SetColorLevel 1000
  mapper2 SetZSlice 0

vtkImageMapper mapper3
  mapper3 SetInputConnection [reslice3 GetOutputPort]
  mapper3 SetColorWindow 128000
  mapper3 SetColorLevel 64000
  mapper3 SetZSlice 0

vtkImageMapper mapper4
  mapper4 SetInputConnection [reslice4 GetOutputPort]
  mapper4 SetColorWindow 2000
  mapper4 SetColorLevel 1000
  mapper4 SetZSlice 0

vtkImageMapper mapper5
  mapper5 SetInputConnection [reslice5 GetOutputPort]
  mapper5 SetColorWindow 2000
  mapper5 SetColorLevel 1000
  mapper5 SetZSlice 0

vtkImageMapper mapper6
  mapper6 SetInputConnection [reslice6 GetOutputPort]
  mapper6 SetColorWindow 2000
  mapper6 SetColorLevel 1000
  mapper6 SetZSlice 0

vtkActor2D actor1
  actor1 SetMapper mapper1

vtkActor2D actor2
  actor2 SetMapper mapper2

vtkActor2D actor3
  actor3 SetMapper mapper3

vtkActor2D actor4
  actor4 SetMapper mapper4

vtkActor2D actor5
  actor5 SetMapper mapper5

vtkActor2D actor6
  actor6 SetMapper mapper6

vtkRenderer imager1
  imager1 AddActor2D actor1
  imager1 SetViewport 0.0 0.0 0.3333 0.5

vtkRenderer imager2
  imager2 AddActor2D actor2
  imager2 SetViewport 0.0 0.5 0.3333 1.0

vtkRenderer imager3
  imager3 AddActor2D actor3
  imager3 SetViewport 0.3333 0.0 0.6667 0.5

vtkRenderer imager4
  imager4 AddActor2D actor4
  imager4 SetViewport 0.3333 0.5 0.6667 1.0

vtkRenderer imager5
  imager5 AddActor2D actor5
  imager5 SetViewport 0.6667 0.0 1.0 0.5

vtkRenderer imager6
  imager6 AddActor2D actor6
  imager6 SetViewport 0.6667 0.5 1.0 1.0

vtkRenderWindow imgWin
  imgWin AddRenderer imager1
  imgWin AddRenderer imager2
  imgWin AddRenderer imager3
  imgWin AddRenderer imager4
  imgWin AddRenderer imager5
  imgWin AddRenderer imager6
  imgWin SetSize 225 150

imgWin Render

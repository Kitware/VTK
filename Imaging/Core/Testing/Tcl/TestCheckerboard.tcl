package require vtk

# Image pipeline

vtkImageCanvasSource2D image1
  image1 SetNumberOfScalarComponents 3
  image1 SetScalarTypeToUnsignedChar
  image1 SetExtent 0 511 0 511 0 0
  image1 SetDrawColor 255 255 0
  image1 FillBox 0 511 0 511

vtkImageWrapPad pad1
  pad1 SetInputConnection [image1 GetOutputPort]
  pad1 SetOutputWholeExtent 0 511 0 511 0 99
  pad1 Update

vtkImageCanvasSource2D image2
  image2 SetNumberOfScalarComponents 3
  image2 SetScalarTypeToUnsignedChar
  image2 SetExtent 0 511 0 511 0 0
  image2 SetDrawColor 0 255 255
  image2 FillBox 0 511 0 511

vtkImageWrapPad pad2
  pad2 SetInputConnection [image2 GetOutputPort]
  pad2 SetOutputWholeExtent 0 511 0 511 0 99
  pad2 Update

vtkImageCheckerboard checkers
  checkers SetInput1Data [pad1 GetOutput]
  checkers SetInput2Data [pad2 GetOutput]
  checkers SetNumberOfDivisions 11 6 2

vtkImageViewer viewer
  viewer SetInputConnection [checkers GetOutputPort]
  viewer SetZSlice 49
  viewer SetColorWindow 255
  viewer SetColorLevel 127.5

viewer Render

wm withdraw .

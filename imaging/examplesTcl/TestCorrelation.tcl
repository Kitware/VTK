# Show the constant kernel.  Smooth an impulse function.

catch {load vtktcl}

source vtkImageInclude.tcl

vtkImageCanvasSource2D s1
s1 SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
s1 SetScalarTypeToFloat
s1 SetExtent 0 255 0 255
s1 SetDrawColor 0
s1 FillBox 0 255 0 255
s1 SetDrawColor 2.0
s1 FillTriangle 10 100  190 150  40 250

vtkImageCanvasSource2D s2
s2 SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
s2 SetScalarTypeToFloat
s2 SetExtent 0 31 0 31
s2 SetDrawColor 0
s2 FillBox 0 31 0 31
s2 SetDrawColor 2.0
s2 FillTriangle 10 1  25 10  1 5


vtkImageCorrelation convolve
convolve SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
convolve SetInput1 [s1 GetOutput]
convolve SetInput2 [s2 GetOutput]

vtkImageViewer viewer
viewer SetInput [convolve GetOutput]
viewer SetColorWindow 500
viewer SetColorLevel 250


# make interface
source WindowLevelInterface.tcl






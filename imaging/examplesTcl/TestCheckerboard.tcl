catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

source vtkImageInclude.tcl

# Image pipeline

vtkImageCanvasSource2D image1
image1 SetNumberOfScalarComponents 3
image1 SetScalarType $VTK_UNSIGNED_CHAR
image1 SetExtent 0 511 0 511 0 0
image1 SetDrawColor 255 255 0
image1 FillBox 0 511 0 511

vtkImageWrapPad pad1
  pad1 SetInput [image1 GetOutput]
  pad1 SetOutputWholeExtent 0 511 0 511 0 99

vtkImageCanvasSource2D image2
image2 SetNumberOfScalarComponents 3
image2 SetScalarType $VTK_UNSIGNED_CHAR
image2 SetExtent 0 511 0 511 0 0
image2 SetDrawColor 0 255 255
image2 FillBox 0 511 0 511

vtkImageWrapPad pad2
  pad2 SetInput [image2 GetOutput]
  pad2 SetOutputWholeExtent 0 511 0 511 0 99

vtkImageCheckerboard checkers
  checkers SetInput 0 [pad1 GetOutput]
  checkers SetInput 1 [pad2 GetOutput]
  checkers SetNumberOfDivisions 11 6 2
 
vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [checkers GetOutput]
viewer SetZSlice 49
viewer SetColorWindow 255
viewer SetColorLevel 127.5


#make interface
source WindowLevelInterface.tcl








catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


source vtkImageInclude.tcl


# Image pipeline

vtkPNMReader reader
reader SetFileName "$VTK_DATA/binary.pgm"

vtkImageCast cast
cast SetInput [reader GetOutput]
cast SetOutputScalarTypeToShort

vtkImageDilateErode3D dilate
dilate SetInput [cast GetOutput]
dilate SetDilateValue 255
dilate SetErodeValue 0
dilate SetKernelSize 31 31 1

vtkImageDilateErode3D erode
erode SetInput [dilate GetOutput]
erode SetDilateValue 0
erode SetErodeValue 255
erode SetKernelSize 31 31 1

vtkImageMathematics add
add SetInput1 [cast GetOutput]
add SetInput2 [erode GetOutput]
add SetOperationToAdd

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [add GetOutput]
viewer SetColorWindow 512
viewer SetColorLevel 256


# make interface
source WindowLevelInterface.tcl





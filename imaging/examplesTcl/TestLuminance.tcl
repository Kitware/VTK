catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# This script calculates the luminanace of an image


source vtkImageInclude.tcl


# Image pipeline

vtkBMPReader image1
  image1 SetFileName "$VTK_DATA/beach.bmp"

vtkImageLuminance luminance
luminance SetInput [image1 GetOutput]

vtkImageViewer viewer
viewer SetInput [luminance GetOutput]
viewer SetColorWindow 255
viewer SetColorLevel 127.5
#viewer DebugOn


#make interface
source WindowLevelInterface.tcl








catch {load vtktcl}
# This script calculates the luminanace of an image


source vtkImageInclude.tcl


# Image pipeline

vtkBMPReader image
  image SetFileName "../../../vtkdata/beach.bmp"

vtkImageLuminance luminance
luminance SetInput [image GetOutput]

vtkImageViewer viewer
viewer SetInput [luminance GetOutput]
viewer SetColorWindow 255
viewer SetColorLevel 127.5
#viewer DebugOn


#make interface
source WindowLevelInterface.tcl








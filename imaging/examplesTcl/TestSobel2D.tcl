catch {load vtktcl}
# This script is for testing the 3D Sobel filter.
# Displays the 3 components using color.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageSobel2D sobel
sobel SetInput [reader GetOutput]
sobel SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
sobel ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [sobel GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 400
viewer SetColorLevel 0


# make interface
source WindowLevelInterface.tcl









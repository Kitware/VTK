catch {load vtktcl}
# This script is for testing the 3D Sobel filter.
# Displays the 3 components using color.

source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageSobel3D sobel
sobel SetInput [reader GetOutput]
sobel SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
sobel ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [sobel GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 400
viewer SetColorLevel 0
viewer ColorFlagOn


# make interface
source WindowLevelInterface.tcl









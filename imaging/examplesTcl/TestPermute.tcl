catch {load vtktcl}
# Simple viewer for images.


source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader DebugOn

vtkImagePermute permute
permute SetInput [reader GetOutput]
permute SetFilteredAxes $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS $VTK_IMAGE_X_AXIS

vtkImageViewer viewer
viewer SetInput [permute GetOutput]
viewer SetZSlice 128
viewer SetColorWindow 2000
viewer SetColorLevel 1000
viewer SetOriginLocationToUpperLeft
#viewer DebugOn
#viewer Render

#make interface
source WindowLevelInterface.tcl








catch {load vtktcl}
# Simple viewer for images.


source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn
#reader Update

vtkImageClip clip
clip SetInput [reader GetOutput]
clip SetOutputAxisWholeExtent $VTK_IMAGE_Y_AXIS 50 150
clip ReleaseDataFlagOff

vtkImageViewer viewer
#viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Z_AXIS $VTK_IMAGE_Y_AXIS
viewer SetInput [clip GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000
#viewer DebugOn
viewer Render

#make interface
source WindowLevelInterface.tcl








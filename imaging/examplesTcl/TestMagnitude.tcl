catch {load vtktcl}
# This script shows the magnitude of an image in frequency domain.


source vtkImageInclude.tcl


# Image pipeline

vtkImageVolume16Reader reader
[reader GetCache] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageGradient gradient
gradient SetInput [reader GetOutput]
gradient SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
#gradient DebugOn

vtkImageMagnitude magnitude
magnitude SetInput [gradient GetOutput]
magnitude ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [magnitude GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1000
viewer SetColorLevel 200
#viewer DebugOn


#make interface
source WindowLevelInterface.tcl








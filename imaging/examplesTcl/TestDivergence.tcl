catch {load vtktcl}
# Divergence measures rate of change of gradient.

source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageGradient gradient
gradient SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
gradient SetInput [reader GetOutput]

vtkImageDivergence3D derivative
derivative SetInput [gradient GetOutput]
derivative ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [derivative GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1000
viewer SetColorLevel 0


# make interface
source WindowLevelInterface.tcl








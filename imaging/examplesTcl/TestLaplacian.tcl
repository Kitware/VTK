catch {load vtktcl}
# Divergence measures rate of change of gradient.

source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageLaplacian laplacian
laplacian SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
laplacian SetInput [reader GetOutput]
laplacian ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [laplacian GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 0
#viewer DebugOn


source WindowLevelInterface.tcl



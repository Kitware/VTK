catch {load vtktcl}
# Get Vectors from the gradent, and extract the z component.



source vtkImageInclude.tcl


# Image pipeline

vtkImageVolume16Reader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGradient gradient
gradient SetInput [reader GetOutput]
gradient SetFilteredAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS

vtkImageExtractComponent extract
extract SetInput [gradient GetOutput]
extract SetComponent 2
extract ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [extract GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 800
viewer SetColorLevel 0


#make interface
source WindowLevelInterface.tcl








# Tst the OpenClose3D filter.

catch {load vtktcl}
source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader SetOutputScalarType $VTK_SHORT
#reader DebugOn

vtkImageThreshold thresh
thresh SetInput [reader GetOutput]
thresh SetOutputScalarType $VTK_UNSIGNED_CHAR
thresh ThresholdByUpper 2000.0
thresh SetInValue 255
thresh SetOutValue 0
thresh ReleaseDataFlagOff

vtkImageOpenClose3D close
close SetInput [thresh GetOutput]
close SetOpenValue 0
close SetCloseValue 255
close SetKernelSize 5 5 3
close ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [close GetOutput]
viewer SetDisplayExtent 0 255 0 255
viewer SetZSlice 2
viewer SetColorWindow 255
viewer SetColorLevel 128


# make interface
source WindowLevelInterface.tcl








catch {load vtktcl}
# A script to test the island removal filter.
# first the image is thresholded, then small islands are removed.

source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
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

vtkImageIslandRemoval2D island
island SetInput [thresh GetOutput]
island SetIslandValue 255
island SetReplaceValue 128
island SetAreaThreshold 100

vtkImageViewer viewer
viewer SetInput [island GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 255
viewer SetColorLevel 128
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl








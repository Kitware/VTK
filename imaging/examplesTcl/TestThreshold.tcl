catch {load vtktcl}
# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.

source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff
reader SetOutputScalarType $VTK_SHORT
#reader DebugOn

vtkImageThreshold thresh
thresh SetInput [reader GetOutput]
thresh SetOutputScalarType $VTK_UNSIGNED_CHAR
thresh ThresholdByUpper 2000.0
thresh SetInValue 255
thresh SetOutValue 0
thresh ReleaseDataFlagOffo

vtkImageViewer viewer
viewer SetInput [thresh GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 255
viewer SetColorLevel 128
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl








catch {load vtktcl}
# Threshold a volume and write it to disk.
# It then reads the new data set from disk and displays it.
# Dont forget to delete the test files after the script is finished.


source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 33
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
reader DebugOn

vtkImageThreshold thresh
thresh SetInput [reader GetOutput]
thresh ThresholdByUpper 1000.0
thresh SetInValue 0.0
thresh SetOutValue 250.0
thresh ReplaceOutOn
thresh SetOutputScalarTypeToUnsignedChar

vtkImageWriter writer
writer SetInput [thresh GetOutput]
writer SetFilePrefix "test"
writer SetFileDimensionality 2
writer SetFilePattern "%s.%d"
writer DebugOn
writer Write





vtkImageReader reader2
#reader2 DebugOn
reader2 SetDataScalarTypeToUnsignedChar
reader2 ReleaseDataFlagOff
reader2 SetDataByteOrderToBigEndian
reader2 SetDataExtent 0 255 0 255 1 33
reader2 SetFilePrefix "test"

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [reader2 GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 300
viewer SetColorLevel 150


# make interface
source WindowLevelInterface.tcl







catch {load vtktcl}
# Threshold a volume and write it to disk.
# It then reads the new data set from disk and displays it.
# Dont forget to delete the test files after the script is finished.


source vtkImageInclude.tcl

# Image pipeline

vtkImageVolume16Reader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageThreshold thresh
thresh SetInput [reader GetOutput]
thresh ThresholdByUpper 1000.0
thresh SetInValue 0.0
thresh SetOutValue 3000.0
thresh ReplaceOutOn

vtkImageShortWriter writer
writer SetInput [thresh GetOutput]
writer SetFilePrefix "test"
writer DebugOn
writer Write





vtkImageVolume16Reader reader2
#reader2 DebugOn
reader2 ReleaseDataFlagOff
reader2 SetDataByteOrderToBigEndian
reader2 SetDataDimensions 256 256 93
reader2 SetFilePrefix "test"

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [reader2 GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 3000
viewer SetColorLevel 1500
viewer Render


# make interface
source WindowLevelInterface.tcl







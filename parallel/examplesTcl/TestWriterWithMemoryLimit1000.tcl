catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Threshold a volume and write it to disk.
# It then reads the new data set from disk and displays it.
# Dont forget to delete the test files after the script is finished.

# use the parallel factory
vtkParallelFactory pf
pf RegisterFactory pf

# Image pipeline

vtkImageReader reader
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 255 0 255 1 33
  reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader SetDataMask 0x7fff

vtkImageThreshold thresh
  thresh SetInput [reader GetOutput]
  thresh ThresholdByUpper 1000.0
  thresh SetInValue 0.0
  thresh SetOutValue 250.0
  thresh ReplaceOutOn
  thresh SetOutputScalarTypeToUnsignedChar

vtkImageWriter writer
  writer SetInput [thresh GetOutput]
  writer SetFileName "garf.xxx"
  writer SetFileName "test.xxx"
  writer SetFileDimensionality 3
  writer SetMemoryLimit 1000
  writer Write

vtkImageReader reader2
  reader2 SetDataScalarTypeToUnsignedChar
  reader2 ReleaseDataFlagOff
  reader2 SetDataExtent 0 255 0 255 1 33
  reader2 SetFileName "garf.xxx"
  reader2 SetFileName "test.xxx"
  reader2 SetFileDimensionality 3

vtkImageViewer viewer
  viewer SetInput [reader2 GetOutput]
  viewer SetZSlice 22
  viewer SetColorWindow 300
  viewer SetColorLevel 150

# make interface
source ../../imaging/examplesTcl/WindowLevelInterface.tcl

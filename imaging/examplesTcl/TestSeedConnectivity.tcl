catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# A script to test the island removal filter.
# first the image is thresholded, then small islands are removed.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageThreshold thresh
thresh SetInput [reader GetOutput]
thresh ThresholdByUpper 2000.0
thresh SetInValue 255
thresh SetOutValue 0
thresh ReleaseDataFlagOff

vtkImageCast cast
cast SetInput [thresh GetOutput]
cast SetOutputScalarType $VTK_UNSIGNED_CHAR

vtkImageSeedConnectivity connect
connect SetInput [cast GetOutput]
connect SetInputConnectValue 255
connect SetOutputConnectedValue 255
connect SetOutputUnconnectedValue 0
connect AddSeed 0 200 0
#connect DebugOn

vtkImageViewer viewer
viewer SetInput [connect GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 255
viewer SetColorLevel 127.5

# make interface
source WindowLevelInterface.tcl








catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# This script shows the magnitude of an image in frequency domain.


source vtkImageInclude.tcl


# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 0 92
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGradient gradient
gradient SetInput [reader GetOutput]
gradient SetDimensionality 3
#gradient DebugOn

vtkImageMagnitude magnitude
magnitude SetInput [gradient GetOutput]

vtkImageViewer viewer
viewer SetInput [magnitude GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1000
viewer SetColorLevel 200
#viewer DebugOn


#make interface
source WindowLevelInterface.tcl








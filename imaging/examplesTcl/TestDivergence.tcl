# Divergence measures rate of change of gradient.
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn

vtkImageGradient gradient
gradient SetDimensionality 3
gradient SetInput [reader GetOutput]

vtkImageDivergence derivative
derivative SetInput [gradient GetOutput]

vtkImageViewer viewer
viewer SetInput [derivative GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 1000
viewer SetColorLevel 0


# make interface
source WindowLevelInterface.tcl








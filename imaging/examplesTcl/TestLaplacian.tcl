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

vtkImageLaplacian laplacian
laplacian SetDimensionality 3
laplacian SetInput [reader GetOutput]

vtkImageViewer viewer
viewer SetInput [laplacian GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 0
#viewer DebugOn


source WindowLevelInterface.tcl



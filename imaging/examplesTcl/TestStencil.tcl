catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# A script to test the stencil filter.
# removes all but a sphere of headSq.

source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 94
reader SetDataSpacing 1 1 2
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff

vtkSphere sphere
sphere SetCenter 128 128 46
sphere SetRadius 80

vtkImageStencil stencil
stencil SetInput [reader GetOutput]
stencil SetStencilFunction sphere
stencil ReverseStencilOn
stencil SetDefaultValue 500

vtkImageViewer viewer
viewer SetInput [stencil GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 2000
viewer SetColorLevel 1000

# make interface
source WindowLevelInterface.tcl








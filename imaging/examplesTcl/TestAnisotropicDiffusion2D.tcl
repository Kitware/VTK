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

vtkImageAnisotropicDiffusion2D diffusion
diffusion SetInput [reader GetOutput]
diffusion SetDiffusionFactor 1.0
diffusion SetDiffusionThreshold 200.0
diffusion SetNumberOfIterations 5
#diffusion DebugOn

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [diffusion GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 3000
viewer SetColorLevel 1500


source WindowLevelInterface.tcl





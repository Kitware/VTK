catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# This scripts shows the real component of an image in frequencey space.


source vtkImageInclude.tcl

# Image pipeline

vtkImageReader reader
[reader GetOutput] ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageFFT fft
fft SetDimensionality 2
fft SetInput [reader GetOutput]
fft ReleaseDataFlagOff
#fft DebugOn

vtkImageFourierCenter center
center SetInput [fft GetOutput]
center SetDimensionality 2

vtkImageViewer viewer
viewer SetInput [center GetOutput]
viewer SetZSlice 22
viewer SetColorWindow 10000
viewer SetColorLevel 4000


source WindowLevelInterface.tcl









# A test for 3d FFT and RFFT.  Specifically, the viewer asks for only 
# a single slice of the resulting volume.

catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl

# Image pipeline

vtkImageSinusoidSource cos
cos SetWholeExtent 0 3 0 3 0 3
cos SetAmplitude 100.0
cos SetDirection 1 1 1
cos SetPeriod [expr 2.0/sqrt(3.0)]
cos ReleaseDataFlagOff

vtkImageFFT fft
fft SetDimensionality 3
fft SetInput [cos GetOutput]

vtkImageRFFT rfft
rfft SetDimensionality 3
rfft SetInput [fft GetOutput]
rfft ReleaseDataFlagOff

#rfft Update

# Does nothing now.  Was used for getting lines from a 2D image.
vtkImageClip clip
clip SetInput [rfft GetOutput]
clip SetOutputWholeExtent 0 3 0 3 0 3

vtkImageMagnify magnify
magnify SetInput [clip  GetOutput]
#magnify SetInput [fft  GetOutput]
#magnify SetInput [cos  GetOutput]
magnify SetMagnificationFactors 50 50 1

vtkImageViewer viewer
viewer SetInput [magnify GetOutput]
viewer SetZSlice 1
viewer SetColorWindow 200.0
viewer SetColorLevel  0.0
if { [[viewer GetImageWindow] GetClassName] != "vtkXImageWindow" } {
    [viewer GetImageWindow] DoubleBufferOn
}
#viewer DebugOn

# make interface
source WindowLevelInterface.tcl







catch {load vtktcl}

source vtkImageInclude.tcl

vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 94
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageCast cast
cast SetInput [reader GetOutput]
cast SetOutputScalarTypeToFloat

set shotNoiseAmplitude 2000.0
set shotNoiseFraction 0.1
set shotNoiseExtent "0 255 0 255 0 92"
source ShotNoiseInclude.tcl
 
vtkImageMathematics add
add SetInput1 [cast GetOutput]
add SetInput2 [shotNoise GetOutput]
add SetOperationToAdd

vtkImageMedian3D med
med SetInput [add GetOutput]
med SetKernelSize 5 5 1

vtkImageGaussianSmooth gauss
gauss SetDimensionality 2
gauss SetInput [add GetOutput]
gauss SetStandardDeviations 2.0 2.0
gauss SetRadiusFactors 2.0 2.0



vtkImageViewer viewer1
viewer1 SetInput [cast GetOutput]
viewer1 SetZSlice 22
viewer1 SetColorWindow 3000
viewer1 SetColorLevel 1000

vtkImageViewer viewer2
viewer2 SetInput [add GetOutput]
viewer2 SetZSlice 22
viewer2 SetColorWindow 3000
viewer2 SetColorLevel 1000

vtkImageViewer viewer3
viewer3 SetInput [gauss GetOutput]
viewer3 SetZSlice 22
viewer3 SetColorWindow 3000
viewer3 SetColorLevel 1000

vtkImageViewer viewer4
viewer4 SetInput [med GetOutput]
viewer4 SetZSlice 22
viewer4 SetColorWindow 3000
viewer4 SetColorLevel 1000

# Create the GUI
wm withdraw .
toplevel .top 

frame .top.f1 
frame .top.f2

vtkTkImageViewerWidget .top.f1.r1 -width 256 -height 256 -iv viewer1
vtkTkImageViewerWidget .top.f1.r2 -width 256 -height 256 -iv viewer2
vtkTkImageViewerWidget .top.f2.r3 -width 256 -height 256 -iv viewer3
vtkTkImageViewerWidget .top.f2.r4 -width 256 -height 256 -iv viewer4

button .top.btn  -text Quit -command exit

pack .top.f1.r1 .top.f1.r2 \
  -side left -padx 3 -pady 3 -expand t
pack .top.f2.r3 .top.f2.r4 \
  -side left -padx 3 -pady 3 -expand t
pack .top.f1 .top.f2  -fill both -expand t
pack .top.btn -fill x










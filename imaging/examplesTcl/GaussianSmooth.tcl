# Show the constant kernel.  Smooth an impulse function.

catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl


vtkImageReader reader
reader ReleaseDataFlagOff
reader SetDataExtent 0 255 0 255 1 1
reader SetFilePrefix "$VTK_DATA/heart"
reader SetDataByteOrderToBigEndian

vtkImageCast cast
cast SetInput [reader GetOutput]
cast SetOutputScalarTypeToFloat

vtkImageGaussianSmooth smooth
smooth SetDimensionality 2
smooth SetInput [cast GetOutput]
smooth SetStandardDeviations 4.0 4.0
smooth SetRadiusFactors 2.0 2.0




vtkImageCanvasSource2D imageCanvas
imageCanvas SetScalarType $VTK_FLOAT
imageCanvas SetExtent -10 10 -10 10 0 0
# back ground zero
imageCanvas SetDrawColor 0
imageCanvas FillBox -10 10 -10 10
# impulse
imageCanvas SetDrawColor 8000
imageCanvas DrawPoint 0 0
 
vtkImageGaussianSmooth smooth2
smooth2 SetDimensionality 2
smooth2 SetInput [imageCanvas GetOutput]
smooth2 SetStandardDeviations 4.0 4.0
smooth2 SetRadiusFactors 3.0 3.0
 
vtkImageMagnify magnify
magnify InterpolateOff
magnify SetMagnificationFactors 5 5 1
magnify SetInput [smooth2 GetOutput]
 
vtkImageViewer viewer2
viewer2 SetInput [magnify GetOutput]
viewer2 SetColorWindow 99
viewer2 SetColorLevel 32


vtkImageViewer viewer1
viewer1 SetInput [cast GetOutput]
viewer1 SetZSlice 0
viewer1 SetColorWindow 400
viewer1 SetColorLevel 200

vtkImageViewer viewer
viewer SetInput [smooth GetOutput]
viewer SetZSlice 0
viewer SetColorWindow 400
viewer SetColorLevel 200

# Create the GUI
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkImageViewerWidget .top.f1.r1 -width 256 -height 256 -iv viewer1
vtkTkImageViewerWidget .top.f1.r2 -width 105 -height 105 -iv viewer2
vtkTkImageViewerWidget .top.f1.r3 -width 256 -height 256 -iv viewer

button .top.btn  -text Quit -command exit

pack .top.f1.r1 .top.f1.r2 .top.f1.r3 \
  -side left -padx 3 -pady 3 -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x

BindTkImageViewer .top.f1.r1 
BindTkImageViewer .top.f1.r2
BindTkImageViewer .top.f1.r3 






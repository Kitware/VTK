catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl


vtkImageCanvasSource2D imageCanvas
imageCanvas SetScalarType $VTK_FLOAT
imageCanvas SetExtent 0 255 0 255 0 0
# back ground zero
imageCanvas SetDrawColor 0
imageCanvas FillBox 0 255 0 255

imageCanvas SetDrawColor 255
imageCanvas FillBox 30 225 30 225

imageCanvas SetDrawColor 0
imageCanvas FillBox 60 195 60 195

imageCanvas SetDrawColor 255
imageCanvas FillTube 100 100 154 154 40.0

imageCanvas SetDrawColor 0

imageCanvas DrawSegment 45 45 45 210
imageCanvas DrawSegment 45 210 210 210
imageCanvas DrawSegment 210 210 210 45
imageCanvas DrawSegment 210 45 45 45

imageCanvas DrawSegment 100 150 150 100
imageCanvas DrawSegment 110 160 160 110
imageCanvas DrawSegment 90 140 140 90
imageCanvas DrawSegment 120 170 170 120
imageCanvas DrawSegment 80 130 130 80




set shotNoiseAmplitude 255.0
set shotNoiseFraction 0.1
set shotNoiseExtent "0 255 0 255 0 0"
source ShotNoiseInclude.tcl

vtkImageMathematics add
add SetInput1 [shotNoise GetOutput]
add SetInput2 [imageCanvas GetOutput]
add SetOperationToAdd





vtkImageMedian3D median
median SetInput [add GetOutput]
median SetKernelSize 5 5 1

vtkImageHybridMedian2D hybrid1
hybrid1 SetInput [add GetOutput]

vtkImageHybridMedian2D hybrid2
hybrid2 SetInput [hybrid1 GetOutput]

vtkImageViewer viewer1
viewer1 SetInput [imageCanvas GetOutput]
viewer1 SetColorWindow 256
viewer1 SetColorLevel 127.5

vtkImageViewer viewer
viewer SetInput [add GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127.5

vtkImageViewer viewer3
viewer3 SetInput [hybrid2 GetOutput]
viewer3 SetColorWindow 256
viewer3 SetColorLevel 127.5

vtkImageViewer viewer4
viewer4 SetInput [median GetOutput]
viewer4 SetColorWindow 256
viewer4 SetColorLevel 127.5

# Create the GUI
wm withdraw .
toplevel .top 

frame .top.f1 
frame .top.f2

vtkTkImageViewerWidget .top.f1.r1 -width 256 -height 256 -iv viewer1
vtkTkImageViewerWidget .top.f1.r2 -width 256 -height 256 -iv viewer
vtkTkImageViewerWidget .top.f2.r3 -width 256 -height 256 -iv viewer3
vtkTkImageViewerWidget .top.f2.r4 -width 256 -height 256 -iv viewer4

button .top.btn  -text Quit -command exit

pack .top.f1.r1 .top.f1.r2 \
  -side left -padx 3 -pady 3 -expand t
pack .top.f2.r3 .top.f2.r4 \
  -side left -padx 3 -pady 3 -expand t
pack .top.f1 .top.f2  -fill both -expand t
pack .top.btn -fill x


BindTkImageViewer .top.f1.r1 
BindTkImageViewer .top.f1.r2
BindTkImageViewer .top.f2.r3
BindTkImageViewer .top.f2.r4



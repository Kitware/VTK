# the start of a mini application that will let the user select
# circular region of an image to process specifically.
catch {load vtktcl}

source vtkImageInclude.tcl
source ../../examplesTcl/WidgetObject.tcl

# This script uses a vtkTkRenderWidget to create a
# Tk widget that is associated with a vtkRenderWindow.
#source TkInteractor.tcl

vtkPNMReader reader
reader ReleaseDataFlagOff
reader SetFileName "../../../vtkdata/earth.ppm"

vtkImageCast cast
cast SetInput [reader GetOutput]
cast SetOutputScalarTypeToFloat

vtkImageGradientMagnitude filter
filter SetInput [cast GetOutput]

vtkImageShiftScale shiftScale
shiftScale SetInput [filter GetOutput]
shiftScale SetShift 64
shiftScale SetScale 2.0

vtkImageEllipsoidSource mask
mask SetRadius 40 40 30000
mask SetCenter 100 100 0
# set the correct size
cast UpdateImageInformation
mask SetWholeExtent 0 511 0 255 0 0

vtkImageMask clip1
clip1 SetImageInput [cast GetOutput]
clip1 SetMaskInput [mask GetOutput]
clip1 SetMaskedOutputValue 0.0;
clip1 NotMaskOn;

vtkImageMask clip2
clip2 SetImageInput [shiftScale GetOutput]
clip2 SetMaskInput [mask GetOutput]
clip2 SetMaskedOutputValue 0.0;
clip2 NotMaskOff;

vtkImageMathematics add
add SetOperationToAdd
add SetInput1 [clip1 GetOutput]
add SetInput2 [clip2 GetOutput]

vtkImageViewer viewer
viewer SetInput [add GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127.5

proc moveLens {x y} {
   #flip Y axis
   set y [expr 255 - $y]
   mask SetCenter $x $y 0
   viewer Render
}

# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top 

frame .top.f1 

vtkTkImageViewerWidget .top.f1.r1 -width 512 -height 256 -iv viewer

bind .top.f1.r1 <Button-1> {moveLens %x %y}
bind .top.f1.r1 <B1-Motion> {moveLens %x %y}
bind .top.f1.r1 <Expose> {Expose %W}

# a litle more complex than just "bind $widget <Expose> {%W Render}"
# we have to handle all pending expose events otherwise they que up.
proc Expose {widget} {
   if {[GetWidgetVariableValue $widget InExpose] == 1} {
      return
   }
   SetWidgetVariableValue $widget InExpose 1
   update
   $widget Render
   SetWidgetVariableValue $widget InExpose 0
}

button .top.btn  -text Quit -command exit

pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t
pack .top.f1  -fill both -expand t
pack .top.btn -fill x




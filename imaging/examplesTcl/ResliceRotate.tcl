
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# In this example, an image is centered at (0,0,0) before a
# rotation is applied to ensure that the rotation occurs about
# the center of the image.

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetDataSpacing 1.0 1.0 2.0
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff

# first center the image at (0,0,0)
vtkImageChangeInformation information
information SetInput [reader GetOutput]
information CenterImageOn

# apply a rotation about (0,0,0)
vtkTransform trans

vtkImageReslice reslice
reslice SetInput [information GetOutput]
reslice SetResliceTransform trans
reslice SetInterpolationModeToCubic

# reset the image back to the way it was (you don't have
# to do this, it is just put in as an example)
vtkImageChangeInformation information2 
information2 SetInput [reslice GetOutput]
information2 SetInformationInput [reader GetOutput]

vtkImageViewer viewer
viewer SetInput [information2 GetOutput]
#[viewer GetImageWindow] DoubleBufferOn
[viewer GetImageWindow] EraseOff
viewer SetZSlice 14
viewer SetColorWindow 2000
viewer SetColorLevel 1000

set angle 30
set oldangle 0

proc SetAngle angle {
    global oldangle
    if {$angle != $oldangle} {
       trans Identity
       trans RotateZ $angle
       viewer Render
       set oldangle $angle
    }
}

SetAngle $angle

source ../../imaging/examplesTcl/WindowLevelInterface.tcl

# only show ui if not testing
if {[info commands rtExMath] != "rtExMath"} {

frame .wl.f3
label .wl.f3.angleLabel -text "Angle"
scale .wl.f3.angle -from -180.0 -to 180.0 \
     -orient horizontal -command SetAngle -variable angle -resolution .1
pack .wl.f3 -side top
pack .wl.f3.angleLabel .wl.f3.angle -side left

}



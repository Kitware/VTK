catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


#source vtkImageInclude.tcl

# warp an image with a thin plate spline

# first, create an image to warp
vtkImageGridSource imageGrid
imageGrid SetGridSpacing 16 16 0
imageGrid SetGridOrigin 0 0 0
imageGrid SetDataExtent 0 255 0 255 0 0
imageGrid SetDataScalarTypeToUnsignedChar

vtkLookupTable table
table SetTableRange 0 1
table SetValueRange 1.0 0.0 
table SetSaturationRange 0.0 0.0 
table SetHueRange 0.0 0.0 
table SetAlphaRange 0.0 1.0
table Build

vtkImageMapToColors alpha
alpha SetInput [imageGrid GetOutput]
alpha SetLookupTable table

vtkPNMReader reader1
reader1 SetFileName "$VTK_DATA/masonry.ppm"

vtkImageBlend blend
blend SetInput 0 [reader1 GetOutput]
blend SetInput 1 [alpha GetOutput]

# next, create a ThinPlateSpline transform 

vtkPoints p1
p1 SetNumberOfPoints 8
p1 SetPoint 0 0 0 0
p1 SetPoint 1 0 255 0
p1 SetPoint 2 255 0 0
p1 SetPoint 3 255 255 0
p1 SetPoint 4 96 96 0
p1 SetPoint 5 96 159 0
p1 SetPoint 6 159 159 0
p1 SetPoint 7 159 96 0

vtkPoints p2
p2 SetNumberOfPoints 8
p2 SetPoint 0 0 0 0
p2 SetPoint 1 0 255 0
p2 SetPoint 2 255 0 0
p2 SetPoint 3 255 255 0
p2 SetPoint 4 96 159 0
p2 SetPoint 5 159 159 0
p2 SetPoint 6 159 96 0
p2 SetPoint 7 96 96 0

vtkThinPlateSplineTransform transform
  transform SetSourceLandmarks p2
  transform SetTargetLandmarks p1
  transform SetBasisToR2LogR
# you must invert the transform before passing it to vtkImageReslice
  transform Inverse

vtkImageReslice reslice
  reslice SetInput [blend GetOutput]
  reslice SetResliceTransform transform
  reslice SetInterpolationModeToLinear

# set the window/level to 255.0/127.5 to view full range
vtkImageViewer viewer
viewer SetInput [reslice GetOutput]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5
viewer SetZSlice 0
viewer Render

set angle -90

vtkMath mathInst
set degToRad [mathInst DoubleDegreesToRadians]

proc SetAngle angle {
    global degToRad
    set theta1 [expr $degToRad*($angle - 45)]
    set theta2 [expr $degToRad*($angle + 45)]
    set dx1 [expr 44.5477*cos($theta1)]
    set dx2 [expr 44.5477*cos($theta2)]
    set dy1 [expr 44.5477*sin($theta1)]
    set dy2 [expr 44.5477*sin($theta2)]
    p2 SetPoint 7 [expr 127.5 + $dx1] [expr 127.5 + $dy1] 0
    p2 SetPoint 4 [expr 127.5 - $dx2] [expr 127.5 - $dy2] 0
    p2 SetPoint 5 [expr 127.5 - $dx1] [expr 127.5 - $dy1] 0
    p2 SetPoint 6 [expr 127.5 + $dx2] [expr 127.5 + $dy2] 0
    p2 Modified
    viewer Render
}

SetAngle -90

#make interface
vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetImageWindow]

vtkTIFFWriter tiffWriter
  tiffWriter SetInput [windowToimage GetOutput]
  tiffWriter SetFileName "TestThinPlateWarp.tcl.tif"
#  pnmWriter Write

source ../../imaging/examplesTcl/WindowLevelInterface.tcl

# only show ui if not testing
if {[info commands rtExMath] != "rtExMath"} {

frame .wl.f3
label .wl.f3.angleLabel -text "Angle"
scale .wl.f3.angle -from -90 -to 90.0 \
     -orient horizontal -command SetAngle -variable angle -resolution .1
pack .wl.f3 -side top
pack .wl.f3.angleLabel .wl.f3.angle -side left

}



catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


source vtkImageInclude.tcl

# add grid lines to an image

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

# set the window/level to 255.0/127.5 to view full range
vtkImageViewer viewer
viewer SetInput [blend GetOutput]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5
viewer SetZSlice 0

viewer Render
set opacity 1
set xorigin 0
set yorigin 0

proc SetOpacity opacity {
    blend SetOpacity 1 $opacity
    viewer Render
}

SetOpacity 1

proc SetXOrigin xorigin {
    global yorigin
    imageGrid SetGridOrigin $xorigin $yorigin 0
    viewer Render
}

proc SetYOrigin yorigin {
    global xorigin
    imageGrid SetGridOrigin $xorigin $yorigin 0
    viewer Render
}

SetXOrigin 0
SetYOrigin 0

#make interface
vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetImageWindow]

vtkPNMWriter pnmWriter
  pnmWriter SetInput [windowToimage GetOutput]
  pnmWriter SetFileName "TestGridSource.tcl.ppm"
#  pnmWriter Write

source ../../imaging/examplesTcl/WindowLevelInterface.tcl

# only show ui if not testing
if {[info commands rtExMath] != "rtExMath"} {

frame .wl.f3
label .wl.f3.opacityLabel -text "Opacity"
scale .wl.f3.opacity -from 0.0 -to 1.0 \
     -orient horizontal -command SetOpacity -variable opacity -resolution .01
pack .wl.f3 -side top
pack .wl.f3.opacityLabel .wl.f3.opacity -side left

frame .wl.f4
label .wl.f4.originLabel -text "X Origin"
scale .wl.f4.origin -from 0 -to 15 \
     -orient horizontal -command SetXOrigin -variable xorigin -resolution 1
pack .wl.f4 -side top
pack .wl.f4.originLabel .wl.f4.origin -side left

frame .wl.f5
label .wl.f5.originLabel -text "Y Origin"
scale .wl.f5.origin -from 0 -to 15 \
     -orient horizontal -command SetYOrigin -variable yorigin -resolution 1
pack .wl.f5 -side top
pack .wl.f5.originLabel .wl.f5.origin -side left
}




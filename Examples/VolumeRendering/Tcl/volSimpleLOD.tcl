# This example demonstrates volume rendering and the use of vtkLODProp3D.
# vtkLODProp3D allows the user to set different graphical representations of
# the data to achieve different levels of interactivity.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

# Create an SLC reader and read in the data.
vtkSLCReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/neghip.slc"

# Create transfer functions for opacity and color to be used in volume
# properties.
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  20   0.0
    opacityTransferFunction AddPoint  255  0.2

vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddRGBPoint      0.0 0.0 0.0 0.0
    colorTransferFunction AddRGBPoint     64.0 1.0 0.0 0.0
    colorTransferFunction AddRGBPoint    128.0 0.0 0.0 1.0
    colorTransferFunction AddRGBPoint    192.0 0.0 1.0 0.0
    colorTransferFunction AddRGBPoint    255.0 0.0 0.2 0.0

# Create volume properties
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction
    volumeProperty SetInterpolationTypeToNearest

vtkVolumeProperty volumeProperty2
    volumeProperty2 SetColor colorTransferFunction
    volumeProperty2 SetScalarOpacity opacityTransferFunction
    volumeProperty2 SetInterpolationTypeToLinear
  
# Create a volume ray cast mapper.  The volume is used for levels 1 and 2 of
# LODProp3D.
vtkVolumeRayCastCompositeFunction  compositeFunction
vtkVolumeRayCastMapper volumeMapper
    volumeMapper SetInput [reader GetOutput]
    volumeMapper SetVolumeRayCastFunction compositeFunction

# Set up the color lookup table for the probe planes to be used in levels 3 and
# 4 of the LODProp3D
vtkLookupTable ColorLookupTable
ColorLookupTable SetNumberOfTableValues 256

for { set i 0 } { $i < 256 } { incr i } {
    set r [colorTransferFunction GetRedValue $i]
    set g [colorTransferFunction GetGreenValue $i]
    set b [colorTransferFunction GetBlueValue $i]
    set a [opacityTransferFunction GetValue $i]
    set a [expr $a * 10.0]
    if { $a > 1.0 } { set a 1.0 }
    ColorLookupTable SetTableValue $i $r $g $b $a
}

# Create planes or different resolutions to probe the data read in using the
# SLC reader.  The probe results are used as levels 3 and 4 of the LODProp3D.
set types [list hres lres]
foreach type $types {
    for { set i 0 } { $i < 3 } { incr i } {
	vtkPlaneSource plane${i}_${type}
	
	vtkTransform transform${i}_${type}
	transform${i}_${type} Identity
	
	vtkTransformPolyDataFilter transpd${i}_${type}
	transpd${i}_${type} SetInput [plane${i}_${type} GetOutput]
	transpd${i}_${type} SetTransform transform${i}_${type}
	
	vtkProbeFilter probe${i}_${type}
	probe${i}_${type} SetInput [transpd${i}_${type} GetOutput]
	probe${i}_${type} SetSource [reader GetOutput]
	
	vtkCastToConcrete cast${i}_${type}
	cast${i}_${type} SetInput [probe${i}_${type} GetOutput]
	
	vtkTriangleFilter tf${i}_${type}
	tf${i}_${type} SetInput [cast${i}_${type} GetPolyDataOutput]
	
	vtkStripper strip${i}_${type}
	strip${i}_${type} SetInput [tf${i}_${type} GetOutput]
    }

    transform0_${type} Translate 33.0 33.0 33.0
    transform0_${type} Scale  66.0 66.0 66.0
    transform1_${type} Translate 33.0 33.0 33.0
    transform1_${type} RotateX 90
    transform1_${type} Scale 66.0 66.0 66.0
    transform2_${type} Translate 33.0 33.0 33.0
    transform2_${type} RotateY 90
    transform2_${type} Scale  66.0 66.0 66.0

    vtkAppendPolyData apd_${type}
    apd_${type} AddInput [tf0_${type} GetOutput]
    apd_${type} AddInput [tf1_${type} GetOutput]
    apd_${type} AddInput [tf2_${type} GetOutput]
    
    vtkPolyDataMapper probeMapper_${type}
    probeMapper_${type} SetInput [apd_${type} GetOutput]
    probeMapper_${type} SetColorModeToMapScalars
    probeMapper_${type} SetLookupTable ColorLookupTable
    probeMapper_${type} SetScalarRange 0 255
}

# Set the resolution of each of the planes just created
plane0_hres SetResolution 60 60
plane1_hres SetResolution 60 60
plane2_hres SetResolution 60 60
plane0_lres SetResolution 25 25
plane1_lres SetResolution 25 25
plane2_lres SetResolution 25 25

# Set the opacity to be used in the probe mappers
vtkProperty probeProperty
    probeProperty SetOpacity 0.99

# Create outline to be used as level 5 in the LODProp3D
vtkOutlineFilter outline
    outline SetInput [reader GetOutput]

vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]

vtkProperty outlineProperty
outlineProperty SetColor 1 1 1

# Create the LODProp3D and add the mappers to it.
vtkLODProp3D lod
set level1 [lod AddLOD volumeMapper volumeProperty2 0.0]
set level2 [lod AddLOD volumeMapper volumeProperty 0.0]
set level3 [lod AddLOD probeMapper_hres probeProperty 0.0]
set level4 [lod AddLOD probeMapper_lres probeProperty 0.0]
set level5 [lod AddLOD outlineMapper outlineProperty 0.0]

# Create a Tcl procedure to display (in the user interface) which LOD is
# currently active.
proc ChangeLabels { } {
    global level1 level2 level3 level4 level5

    set value [lod GetLastRenderedLODID]
    if { $value == $level1 } {
	.top.controls.lod configure -text "LOD Selected: Level 1"
    } elseif { $value == $level2 } {
	.top.controls.lod configure -text "LOD Selected: Level 2"
    } elseif { $value == $level3 } {
	.top.controls.lod configure -text "LOD Selected: Level 3"
    } elseif { $value == $level4 } {
	.top.controls.lod configure -text "LOD Selected: Level 4"
    } elseif { $value == $level5 } {
	.top.controls.lod configure -text "LOD Selected: Level 5"
    }

    set value [lod GetLODEstimatedRenderTime $level1]
    if { $value == 0 } {
	.top.controls.l1 configure -text "Level 1 FPS: unknown"
    } else {
	.top.controls.l1 configure -text \
		"Level 1 FPS: [format "%.2f" [expr 1.0 / $value]]"
    }

    set value [lod GetLODEstimatedRenderTime $level2]
    if { $value == 0 } {
	.top.controls.l2 configure -text "Level 2 FPS: unknown"
    } else {
	.top.controls.l2 configure -text \
		"Level 2 FPS: [format "%.2f" [expr 1.0 / $value]]"
    }

    set value [lod GetLODEstimatedRenderTime $level3]
    if { $value == 0 } {
	.top.controls.l3 configure -text "Level 3 FPS: unknown"
    } else {
	.top.controls.l3 configure -text \
		"Level 3 FPS: [format "%.2f" [expr 1.0 / $value]]"
    }

    set value [lod GetLODEstimatedRenderTime $level4]
    if { $value == 0 } {
	.top.controls.l4 configure -text "Level 4 FPS: unknown"
    } else {
	.top.controls.l4 configure -text \
		"Level 4 FPS: [format "%.2f" [expr 1.0 / $value]]"
    }

    set value [lod GetLODEstimatedRenderTime $level5]
    if { $value == 0 } {
	.top.controls.l5 configure -text "Level 5 FPS: unknown"
    } else {
	.top.controls.l5 configure -text \
		"Level 5 FPS: [format "%.2f" [expr 1.0 / $value]]"
    }

    update
}

# Set the ChangeLabels proc to be executed when rendering is finished
set TkInteractor_EndRenderMethod ChangeLabels

# Withdraw the default tk window
wm withdraw .

# Create the user interface including the RenderWindow
toplevel .top -visual best

frame .top.ren
frame .top.controls

pack .top.controls .top.ren -side left -padx 10 -pady 10

vtkRenderWindow renWin
vtkTkRenderWidget .top.ren.rw -rw renWin -width 256 -height 256 
BindTkRenderWidget .top.ren.rw

pack .top.ren.rw 

label .top.controls.lod -text "LOD Selected:"
pack .top.controls.lod -side top -anchor w -padx 5 -pady 6

label .top.controls.l1 -text "Level 1 FPS:"
pack .top.controls.l1 -side top -anchor w -padx 5 -pady 2

label .top.controls.l2 -text "Level 2 FPS:"
pack .top.controls.l2 -side top -anchor w -padx 5 -pady 2

label .top.controls.l3 -text "Level 3 FPS:"
pack .top.controls.l3 -side top -anchor w -padx 5 -pady 2

label .top.controls.l4 -text "Level 4 FPS:"
pack .top.controls.l4 -side top -anchor w -padx 5 -pady 2

label .top.controls.l5 -text "Level 5 FPS:"
pack .top.controls.l5 -side top -anchor w -padx 5 -pady 2

set TkInteractor_StillUpdateRate 0.5
set TkInteractor_InteractiveUpdateRate 5.0

scale .top.controls.s1 -label "Still FPS:" -orient horizontal \
	-length 150 -from 0.05 -to 1.0 -resolution 0.05 \
	-variable TkInteractor_StillUpdateRate -highlightthickness 0
pack .top.controls.s1 -side top -anchor w -padx 5 -pady 6

scale .top.controls.s2 -label "Moving FPS:" -orient horizontal \
	-length 150 -from 1.0 -to 30.0 -resolution 1.0 \
	-variable TkInteractor_InteractiveUpdateRate -highlightthickness 0
pack .top.controls.s2 -side top -anchor w -padx 5 -pady 6

button .top.controls.quit -text "Quit" -command {exit}
pack .top.controls.quit -side top -padx 5 -pady 6 -expand 1 -fill both

# Create the renderer.
vtkRenderer ren1
renWin AddRenderer ren1

# Add the LODProp3D to the renderer and set the background
ren1 AddProp lod
ren1 SetBackground 0.1 0.2 0.4

# Create a Tcl procedure to check whether to abort rendering
proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver AbortCheckEvent {TkCheckAbort}

# Render into the RenderWindow
renWin Render

# Determine whether to allow automatic LOD selection.
if { [info command rtExMath] != "" } {
    lod AutomaticLODSelectionOff
    lod SetSelectedLODID $level3
}

# Update the renderer and render
UpdateRenderer .top.ren.rw 0 0
Render .top.ren.rw

# This example demonstrates volume rendering and the use of vtkLODProp3D.
# vtkLODProp3D allows the user to set different graphical representations of
# the data to achieve different levels of interactivity.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

#
# Create an SLC reader and read in the data.
#
vtkSLCReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/neghip.slc"

#
# Create transfer functions for opacity and color to be used in volume
# properties.
#
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  20   0.0
    opacityTransferFunction AddPoint  255  0.2

vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddRGBPoint      0.0 0.0 0.0 0.0
    colorTransferFunction AddRGBPoint     64.0 1.0 0.0 0.0
    colorTransferFunction AddRGBPoint    128.0 0.0 0.0 1.0
    colorTransferFunction AddRGBPoint    192.0 0.0 1.0 0.0
    colorTransferFunction AddRGBPoint    255.0 0.0 0.2 0.0

#
# Create volume properties
#
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction
    volumeProperty SetInterpolationTypeToNearest

vtkVolumeProperty volumeProperty2
    volumeProperty2 SetColor colorTransferFunction
    volumeProperty2 SetScalarOpacity opacityTransferFunction
    volumeProperty2 SetInterpolationTypeToLinear
  
#
# Create a volume ray cast mapper.  The volume is used for levels 1 and 2 of
# LODProp3D.
#
vtkVolumeRayCastCompositeFunction  compositeFunction
vtkVolumeRayCastMapper volumeMapper
    volumeMapper SetInputConnection [reader GetOutputPort]
    volumeMapper SetVolumeRayCastFunction compositeFunction

#
# Set up the color lookup table for the probe planes to be used in levels 3 and
# 4 of the LODProp3D
#
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

#
# Create planes or different resolutions to probe the data read in using the
# SLC reader.  The probe results are used as levels 3 and 4 of the LODProp3D.
#
set types [list hres lres]
foreach type $types {
    for { set i 0 } { $i < 3 } { incr i } {
	vtkPlaneSource plane${i}_${type}
	
	vtkTransform transform${i}_${type}
	transform${i}_${type} Identity
	
	vtkTransformPolyDataFilter transpd${i}_${type}
	transpd${i}_${type} SetInputConnection [plane${i}_${type} GetOutputPort]
	transpd${i}_${type} SetTransform transform${i}_${type}
	
	vtkProbeFilter probe${i}_${type}
	probe${i}_${type} SetInputConnection [transpd${i}_${type} GetOutputPort]
	probe${i}_${type} SetSource [reader GetOutput]
	
	vtkCastToConcrete cast${i}_${type}
	cast${i}_${type} SetInputConnection [probe${i}_${type} GetOutputPort]
	
	vtkTriangleFilter tf${i}_${type}
	tf${i}_${type} SetInput [cast${i}_${type} GetPolyDataOutput]
	
	vtkStripper strip${i}_${type}
	strip${i}_${type} SetInputConnection [tf${i}_${type} GetOutputPort]
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
    probeMapper_${type} SetInputConnection [apd_${type} GetOutputPort]
    probeMapper_${type} SetColorModeToMapScalars
    probeMapper_${type} SetLookupTable ColorLookupTable
    probeMapper_${type} SetScalarRange 0 255
}

#
# Set the resolution of each of the planes just created
#
plane0_hres SetResolution 60 60
plane1_hres SetResolution 60 60
plane2_hres SetResolution 60 60
plane0_lres SetResolution 25 25
plane1_lres SetResolution 25 25
plane2_lres SetResolution 25 25

#
# Set the opacity to be used in the probe mappers
#
vtkProperty probeProperty
    probeProperty SetOpacity 0.99

#
# Create outline to be used as level 5 in the LODProp3D
#
vtkOutlineFilter outline
    outline SetInputConnection [reader GetOutputPort]

vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]

vtkProperty outlineProperty
outlineProperty SetColor 1 1 1

#
# Create the LODProp3D and add the mappers to it.
#
vtkLODProp3D lod
set level1 [lod AddLOD volumeMapper volumeProperty2 0.0]
set level2 [lod AddLOD volumeMapper volumeProperty 0.0]
set level3 [lod AddLOD probeMapper_hres probeProperty 0.0]
set level4 [lod AddLOD probeMapper_lres probeProperty 0.0]
set level5 [lod AddLOD outlineMapper outlineProperty 0.0]

#
# Create the render window and the renderer.
#
vtkRenderWindow renWin
vtkRenderer ren1
renWin AddRenderer ren1

#
# Add the LODProp3D to the renderer and set the background
#
ren1 AddViewProp lod
ren1 SetBackground 0.1 0.2 0.4

#
# Withdraw the default tk window
#
wm withdraw .

#
# Create the user interface including the RenderWindow
#
toplevel .top \
        -visual best
wm protocol .top WM_DELETE_WINDOW ::vtk::cb_exit

frame .top.ren
frame .top.controls

pack .top.controls \
        -side left -anchor n \
        -padx 3 -pady 3

pack .top.ren \
        -side right -anchor n \
        -padx 3 -pady 3 \
        -expand t -fill both

#
# Create Tk renderwidget, bind events
#
vtkTkRenderWidget .top.ren.rw \
        -rw renWin \
        -width 300 \
        -height 300

::vtk::bind_tk_render_widget .top.ren.rw

pack .top.ren.rw \
        -expand t -fill both

#
# Create LOD display controls
#
label .top.controls.lod \
        -text "LOD Selected:"

pack .top.controls.lod \
        -side top -anchor w \
        -padx 5 -pady 6

label .top.controls.l1 -text "Level 1 FPS:"
label .top.controls.l2 -text "Level 2 FPS:"
label .top.controls.l3 -text "Level 3 FPS:"
label .top.controls.l4 -text "Level 4 FPS:"
label .top.controls.l5 -text "Level 5 FPS:"

pack .top.controls.l1 .top.controls.l2 .top.controls.l3 .top.controls.l4 .top.controls.l5 \
        -side top -anchor w \
        -padx 5 -pady 2

#
# Create frame rate controls
#
set still_update_rate 0.5
set interactive_update_rate 5.0

proc update_rates {{foo 0}} {
    global still_update_rate interactive_update_rate
    set iren [renWin GetInteractor]
    $iren SetStillUpdateRate $still_update_rate
    $iren SetDesiredUpdateRate $interactive_update_rate
}

update_rates

scale .top.controls.s1 \
        -label "Still FPS:" \
        -orient horizontal \
	-length 150 \
        -from 0.05 -to 1.0 -resolution 0.05 \
	-variable still_update_rate \
        -command update_rates \
        -highlightthickness 0

scale .top.controls.s2 \
        -label "Moving FPS:" \
        -orient horizontal \
	-length 150 \
        -from 1.0 -to 100.0 -resolution 1.0 \
	-variable interactive_update_rate \
        -command update_rates \
        -highlightthickness 0

pack .top.controls.s1 .top.controls.s2 \
        -side top \
        -anchor w -padx 5 -pady 6

button .top.controls.quit \
        -text "Quit" \
        -command ::vtk::cb_exit

pack .top.controls.quit \
        -side top \
        -padx 5 -pady 6 \
        -expand 1 -fill both

#
# Determine whether to allow automatic LOD selection.
#
if { [info command rtExMath] != "" } {
    lod AutomaticLODSelectionOff
    lod SetSelectedLODID $level3
}

#
# Create a Tcl procedure to display (in the user interface) which LOD is
# currently active.
# This proc is called each time an EndEvent is triggered by the renderer.
#
renWin AddObserver EndEvent ChangeLabels

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
}

#
# Render into the RenderWindow
#
renWin Render

tkwait window .


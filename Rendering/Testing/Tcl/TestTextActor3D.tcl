package require vtk
package require vtkinteraction

# Renderer, renwin

vtkRenderer ren1
ren1 SetBackground 0.1 0.2 0.4

vtkRenderWindow renWin
renWin AddRenderer ren1
renWin SetSize 600 600

# The Tk render widget

set vtkw [vtkTkRenderWidget .ren -width 450 -height 450 -rw renWin]
::vtk::bind_tk_render_widget $vtkw

[[renWin GetInteractor] GetInteractorStyle] SetCurrentStyleToTrackballCamera

pack $vtkw -side left -fill both -expand yes

# Base text property

vtkTextProperty base_text_prop
base_text_prop SetFontSize 48
base_text_prop ShadowOn
base_text_prop SetColor 1.0 0.0 0.0
base_text_prop SetFontFamilyToArial

set base_scale 0.0025

set base_text "This is a test"

# The text actors

set text_actors {}

# Add a sphere

proc add_sphere {} {
    vtkSphereSource obj_source

    vtkPolyDataMapper obj_mapper
    obj_mapper SetInputConnection [obj_source GetOutputPort]

    vtkActor obj_actor
    obj_actor SetMapper obj_mapper
    [obj_actor GetProperty] SetRepresentationToWireframe

    ren1 AddActor obj_actor
}

# Add one text actor, centered

proc add_one_text_actor {} {
    global text_actors

    vtkTextActor3D ia
    lappend text_actors ia

    set tprop [ia GetTextProperty]
    $tprop ShallowCopy base_text_prop
}

# Add many text actor

proc add_many_text_actors {} {
    global text_actors

    vtkColorTransferFunction lut
    lut SetColorSpaceToHSV
    lut AddRGBPoint 0.0 0.0 1.0 1.0
    lut AddRGBPoint 1.0 1.0 1.0 1.0

    for {set i 0} {$i < 10} {incr i} {
        set name "ia$i"
        vtkTextActor3D $name
        $name SetOrientation 0 [expr $i * 36] 0
  #      $name SetPosition [expr cos($i * 0.0314)] 0 0
        lappend text_actors $name

        set tprop [$name GetTextProperty]
        $tprop ShallowCopy base_text_prop
        set value [expr $i / 10.0]
        eval $tprop SetColor [lut GetColor $value]
    }

    lut Delete
}

set scale_length 200
frame .controls -relief groove -bd 2
pack .controls -padx 2 -pady 2 -anchor nw -side left -fill both -expand n

# Add control of text

set entry_text [entry .controls.text]

$entry_text insert 0 "$base_text"

pack $entry_text -padx 4 -pady 4 -side top -fill x -expand n

bind $entry_text <Return> {update_text_actors 0}
bind $entry_text <FocusOut> {update_text_actors 0}

# Add control of orientation

set scale_orientation [scale .controls.orientation \
        -from 0 -to 360 -res 1 \
        -length $scale_length \
        -orient horizontal \
        -label "Text orientation:" \
        -command update_text_actors]

$scale_orientation set [base_text_prop GetOrientation]
pack $scale_orientation -side top -fill x -expand n

# Add control of font size

set scale_font_size [scale .controls.font_size \
        -from 5 -to 150 -res 1 \
        -length $scale_length \
        -orient horizontal \
        -label "Font Size:" \
        -command update_text_actors]

$scale_font_size set [base_text_prop GetFontSize]
pack $scale_font_size -side top -fill x -expand n

# Add control of scale

set scale_scale [scale .controls.scale \
        -from 0 -to 100 -res 1 \
        -length $scale_length \
        -orient horizontal \
        -label "Actor scale:" \
        -command update_text_actors]

$scale_scale set [expr $base_scale * 10000.0]
pack $scale_scale -side top -fill x -expand n

# Add control of opacity

set scale_opacity [scale .controls.opacity \
        -from 0.0 -to 1.0 -res 0.01 \
        -length $scale_length \
        -orient horizontal \
        -label "Text opacity:" \
        -command update_text_actors]

$scale_opacity set [base_text_prop GetOpacity]
pack $scale_opacity -side top -fill x -expand n

# Update all text actors

proc update_text_actors {dummy} {
    global scale_orientation scale_font_size scale_scale entry_text scale_opacity
    set orientation [$scale_orientation get]
    set font_size [$scale_font_size get]
    set scale [expr [$scale_scale get] / 10000.0]
    set text [$entry_text get]
    set opacity [$scale_opacity get]

    global text_actors
    set i 0
    foreach actor $text_actors {
        $actor SetScale $scale
        $actor SetInput "$text"
        set tprop [$actor GetTextProperty]
        $tprop SetFontSize $font_size
        $tprop SetOrientation $orientation
        $tprop SetOpacity $opacity
        incr i
    }

    renWin Render
}

# Create and add all text actors

if {0} {
    add_sphere
    add_one_text_actor
    ren1 ResetCamera
} {
    add_many_text_actors
    ren1 ResetCamera

    set cam [ren1 GetActiveCamera]
    $cam Elevation 30
    $cam Dolly 0.4
}

update_text_actors 0

foreach actor $text_actors {
    ren1 AddActor $actor
}

# Interact

renWin Render

wm protocol . WM_DELETE_WINDOW ::vtk::cb_exit
tkwait window .
vtkCommand DeleteAllObject

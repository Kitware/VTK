package require vtk
package require vtkinteraction

# Renderer, renwin

vtkRenderer ren1
ren1 SetBackground 0.1 0.2 0.4

vtkRenderWindow renWin
renWin AddRenderer ren1
renWin SetSize 600 600

# The Tk render widget

set vtkw [vtkTkRenderWidget .ren -width 600 -height 600 -rw renWin]
::vtk::bind_tk_render_widget $vtkw

pack $vtkw -side top -fill both -expand yes

# Base text property

vtkTextProperty base_text_prop
base_text_prop SetFontSize 24
base_text_prop ShadowOn
base_text_prop SetColor 1.0 0.0 0.0
base_text_prop SetFontFamilyToArial

set base_scale 0.005

# The text actors

set text_actors {}

# Add a sphere

proc add_sphere {} {
    vtkSphereSource obj_source

    vtkPolyDataMapper obj_mapper
    obj_mapper SetInput [obj_source GetOutput]

    vtkActor obj_actor
    obj_actor SetMapper obj_mapper
    [obj_actor GetProperty] SetRepresentationToWireframe

    ren1 AddActor obj_actor
}

# Add one text actor, centered

proc add_one_text_actor {} {
    global base_scale text_actors

    vtkTextActor3D ia
    ia SetInput "This is a test"
    ia SetScale $base_scale
    lappend text_actors ia

    set tprop [ia GetTextProperty]
    $tprop ShallowCopy base_text_prop
}

# Add many text actor

proc add_many_text_actors {} {
    global base_scale text_actors

    vtkColorTransferFunction lut
    lut SetColorSpaceToHSV
    lut AddRGBPoint 0.0 0.0 1.0 1.0
    lut AddRGBPoint 1.0 1.0 1.0 1.0

    for {set i 0} {$i < 10} {incr i} {
        set name "ia$i"
        vtkTextActor3D $name
        $name SetInput "  This is a test ($i)"
        $name SetScale $base_scale
        $name SetOrientation 0 [expr $i * 36] 0
        lappend text_actors $name

        set tprop [$name GetTextProperty]
        $tprop ShallowCopy base_text_prop
        set value [expr $i / 10.0]
        eval $tprop SetColor [lut GetColor $value]
        eval $tprop SetColor [lut GetRedValue $value] [lut GetGreenValue $value] [lut GetBlueValue $value]
    }

    lut Delete
}

# Add control of orientation

set scale_orientation [scale .orientation \
        -from 0 -to 360 -res 1 \
        -orient horizontal \
        -label "Text orientation:" \
        -command update_text_actors]

$scale_orientation set [base_text_prop GetOrientation]
pack $scale_orientation -side top -fill x -expand n

# Add control of font size

set scale_font_size [scale .font_size \
        -from 5 -to 150 -res 1 \
        -orient horizontal \
        -label "Font Size:" \
        -command update_text_actors]

$scale_font_size set [base_text_prop GetFontSize]
pack $scale_font_size -side top -fill x -expand n

# Add control of scale

set scale_scale [scale .scale \
        -from 0 -to 100 -res 1 \
        -orient horizontal \
        -label "Actor scale:" \
        -command update_text_actors]

$scale_scale set [expr $base_scale * 10000.0]
pack $scale_scale -side top -fill x -expand n

# Update all text actors

proc update_text_actors {dummy} {
    global scale_orientation scale_font_size scale_scale
    set orientation [$scale_orientation get]
    set font_size [$scale_font_size get]
    set scale [expr [$scale_scale get] / 10000.0]

    global text_actors
    foreach actor $text_actors {
        $actor SetScale $scale
        set tprop [$actor GetTextProperty]
        $tprop SetFontSize $font_size
        $tprop SetOrientation $orientation
    }

    renWin Render
}

# Create and add all text actors

if {0} {
    add_sphere
    add_one_text_actor
} {
    add_many_text_actors
    set cam [ren1 GetActiveCamera]
    $cam Elevation 30
    $cam Dolly 0.3
}

foreach actor $text_actors {
    ren1 AddActor $actor
}

# Interact

ren1 ResetCamera
renWin Render

wm protocol . WM_DELETE_WINDOW ::vtk::cb_exit
tkwait window .
vtkCommand DeleteAllObject

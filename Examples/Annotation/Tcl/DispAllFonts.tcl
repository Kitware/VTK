#
# This example displays all possible combinations of font families and styles.
#

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

#
# We set the font size constraints, default text and colors
#
set current_font_size 16
set min_font_size 3
set max_font_size 50

set default_text "ABCDEFGHIJKLMnopqrstuvwxyz 0123456789 !@#$%()-=_+[]{};:,./<>?"
# set default_text "The quick red fox"

set text_color [list [expr 246 / 255.0] [expr 255 / 255.0] [expr 11 / 255.0]]
set bg_color [list [expr 56 / 255.0] [expr 56 / 255.0] [expr 154 / 255.0]]

#
# We create the render window which will show up on the screen
# We put our renderer into the render window using AddRenderer.
# Do not set the size of the window here.
#
vtkRenderWindow renWin

vtkRenderer ren1
    eval ren1 SetBackground $bg_color
    renWin AddRenderer ren1

#
# We create text actors for each font family and several combinations of bold,
# italic and shadowed style.
#
set text_actors {}
foreach family {
    Arial
    Courier
    Times
} {
    foreach {bold italic shadow} {
        0 0 0
        0 0 1
        1 0 0
        0 1 0
        1 1 0
    } {
        set mapper [vtkTextMapper mapper_${family}_${bold}_${italic}_${shadow}]
        set attribs {}
        if {$bold} {
            lappend attribs "b"
        }
        if {$italic} {
            lappend attribs "i"
        }
        if {$shadow} {
            lappend attribs "s"
        }
        set face_name "$family"
        if {[llength $attribs]} {
            set face_name "$face_name ([join $attribs ,])"
        }
        $mapper SetInput "$face_name: $default_text"
        set tprop [$mapper GetTextProperty]
        eval $tprop SetFontFamilyTo$family
        eval $tprop SetColor $text_color
        $tprop SetBold $bold
        $tprop SetItalic $italic
        $tprop SetShadow $shadow

        set actor [vtkActor2D actor_${family}_${bold}_${italic}_${shadow}]
        $actor SetMapper $mapper
        lappend text_actors $actor
        ren1 AddActor $actor
    }
}

#
# vtkTkRenderWidget is a Tk widget that we can render into. It has a
# GetRenderWindow method that returns a vtkRenderWindow. This can then
# be used to create a vtkRenderer and etc. We can also specify a
# vtkRenderWindow to be used when creating the widget by using the -rw
# option, which is what we do here by using renWin. It also takes
# -width and -height options that can be used to specify the widget
# size, hence the render window size.
#
set vtkw [vtkTkRenderWidget .ren \
        -width 800 \
        -rw renWin]

#
# Setup Tk bindings and VTK observers for that widget.
#
::vtk::bind_tk_render_widget $vtkw

#
# Once the VTK widget has been created it can be inserted into a whole Tk GUI
# as well as any other standard Tk widgets.
#
# We create a size slider controlling the font size.
# The orientation of this widget is horizontal (-orient option). We label
# it using the -label option. Finally, we bind the scale to Tcl code
# by assigning the -command option to the name of a Tcl
# procedure. Whenever the slider value changes this procedure will be
# called, enabling us to propagate this GUI setting to the
# corresponding VTK object.
#
set size_slider [scale .size \
        -from $min_font_size -to $max_font_size -res 1 \
        -orient horizontal \
        -label "Font size:" \
        -command set_font_size]

#
# The procedure is called by the slider-widget handler whenever the
# slider value changes (either through user interaction or
# programmatically). It receives the slider value as parameter. We
# update the corresponding VTK objects by calling the
# SetFontSize method using this parameter and we render the scene to
# update the pipeline.
#
proc set_font_size {size} {
    global text_actors
    set i 0
    foreach actor $text_actors {
        incr i
        [[$actor GetMapper] GetTextProperty] SetFontSize $size
        $actor SetDisplayPosition 10 [expr $i * ($size + 5)]
    }

    renWin SetSize 800 [expr 20 + $i * ($size + 5)]
    renWin Render
}

$size_slider set $current_font_size

#
# Finally we pack the VTK widget and the sliders on top of each other
# (-side top) inside the main root widget.
#
pack $size_slider -side top -fill both
pack $vtkw -side top -fill both -expand yes

#
# We set the window manager (wm command) so that it registers a
# command to handle the WM_DELETE_WINDOW protocal request. This
# request is triggered when the widget is closed using the standard
# window manager icons or buttons. In this case the exit callback
# will be called and it will free up any objects we created then exit
# the application.
#
wm protocol . WM_DELETE_WINDOW ::vtk::cb_exit

#
# You only need this line if you run this script from a Tcl shell
# (tclsh) instead of a Tk shell (wish)
#
tkwait window .


## Procedure should be called to set bindings and initialize variables
#
source vtkInt.tcl

proc BindTkRenderWidget {widget} {
    bind $widget <Any-ButtonPress> {StartMotion %W %x %y}
    bind $widget <Any-ButtonRelease> {EndMotion %W %x %y}
    bind $widget <B1-Motion> {Rotate %W %x %y}
    bind $widget <B2-Motion> {Pan %W %x %y}
    bind $widget <B3-Motion> {Zoom %W %x %y}
    bind $widget <Shift-B1-Motion> {Pan %W %x %y}
    bind $widget <KeyPress-r> {Reset %W %x %y}
    bind $widget <KeyPress-u> {wm deiconify .vtkInteract}
    bind $widget <KeyPress-w> Wireframe
    bind $widget <KeyPress-s> Surface
    bind $widget <Enter> {Enter %W}
    bind $widget <Leave> {focus $oldFocus}
}

# Create event bindings
#
proc Render {} {
   global CurrentCamera CurrentLight CurrentRenderWindow

   if {$CurrentLight != ""} {
      eval $CurrentLight SetPosition [$CurrentCamera GetPosition]
      eval $CurrentLight SetFocalPoint [$CurrentCamera GetFocalPoint]
   }

   $CurrentRenderWindow Render
}

proc UpdateRenderer {widget} {
   global CurrentCamera CurrentLight 
   global CurrentRenderWindow CurrentRenderer
   
   set CurrentRenderWindow [$widget GetRenderWindow]
   set renderers [$CurrentRenderWindow GetRenderers]
   $renderers InitTraversal; set CurrentRenderer [$renderers GetNextItem]
   set CurrentCamera [$CurrentRenderer GetActiveCamera]
   set lights [$CurrentRenderer GetLights]
   $lights InitTraversal; set CurrentLight [$lights GetNextItem]
}

proc Enter {widget} {
    global oldFocus

    set oldFocus [focus]
    focus $widget
    UpdateRenderer $widget
}

proc StartMotion {widget x y} {
    global CurrentCamera CurrentLight 
    global CurrentRenderWindow CurrentRenderer
    global LastX LastY
    global WindowX WindowY 

    UpdateRenderer $widget

      $CurrentRenderWindow SetDesiredUpdateRate 5.0
      set LastX $x
      set LastY $y
      set WindowX [lindex [$widget configure -width] 4]
      set WindowY [lindex [$widget configure -height] 4]
}

set CurrentRenderWindow ""
proc EndMotion {widget x y} {
    global CurrentRenderWindow
    if { $CurrentRenderWindow == "" } {return}

    $CurrentRenderWindow SetDesiredUpdateRate 0.01
    Render
}

proc Rotate {widget x y} {
    global CurrentCamera 
    global LastX LastY

    $CurrentCamera Azimuth [expr ($LastX - $x)]
    $CurrentCamera Elevation [expr ($y - $LastY)]
    $CurrentCamera OrthogonalizeViewUp

    set LastX $x
    set LastY $y

    Render
}

proc Pan {widget x y} {
    global CurrentRenderer CurrentCamera
    global WindowX WindowY LastX LastY

    set FPoint [$CurrentCamera GetFocalPoint]
        set FPoint0 [lindex $FPoint 0]
        set FPoint1 [lindex $FPoint 1]
        set FPoint2 [lindex $FPoint 2]

    set PPoint [$CurrentCamera GetPosition]
        set PPoint0 [lindex $PPoint 0]
        set PPoint1 [lindex $PPoint 1]
        set PPoint2 [lindex $PPoint 2]

    $CurrentRenderer SetWorldPoint $FPoint0 $FPoint1 $FPoint2 1.0
    $CurrentRenderer WorldToDisplay
    set DPoint [$CurrentRenderer GetDisplayPoint]
    set focalDepth [lindex $DPoint 2]

    set APoint0 [expr $WindowX/2.0 + ($x - $LastX)]
    set APoint1 [expr $WindowY/2.0 - ($y - $LastY)]

    $CurrentRenderer SetDisplayPoint $APoint0 $APoint1 $focalDepth
    $CurrentRenderer DisplayToWorld
    set RPoint [$CurrentRenderer GetWorldPoint]
        set RPoint0 [lindex $RPoint 0]
        set RPoint1 [lindex $RPoint 1]
        set RPoint2 [lindex $RPoint 2]
        set RPoint3 [lindex $RPoint 3]
    if { $RPoint3 != 0.0 } {
        set RPoint0 [expr $RPoint0 / $RPoint3]
        set RPoint1 [expr $RPoint1 / $RPoint3]
        set RPoint2 [expr $RPoint2 / $RPoint3]
    }

    $CurrentCamera SetFocalPoint \
      [expr ($FPoint0 - $RPoint0)/2.0 + $FPoint0] \
      [expr ($FPoint1 - $RPoint1)/2.0 + $FPoint1] \
      [expr ($FPoint2 - $RPoint2)/2.0 + $FPoint2]

    $CurrentCamera SetPosition \
      [expr ($FPoint0 - $RPoint0)/2.0 + $PPoint0] \
      [expr ($FPoint1 - $RPoint1)/2.0 + $PPoint1] \
      [expr ($FPoint2 - $RPoint2)/2.0 + $PPoint2]

    set LastX $x
    set LastY $y

    Render
}

proc Zoom {widget x y} {
    global CurrentCamera
    global LastX LastY

    set zoomFactor [expr pow(1.02,($y - $LastY))]
    set clippingRange [$CurrentCamera GetClippingRange]
    set minRange [lindex $clippingRange 0]
    set maxRange [lindex $clippingRange 1]
    $CurrentCamera SetClippingRange [expr $minRange / $zoomFactor] \
                                    [expr $maxRange / $zoomFactor]
    $CurrentCamera Dolly $zoomFactor

    set LastX $x
    set LastY $y

    Render
}

proc Reset {widget x y} {
   global CurrentRenderWindow
   set CurrentRenderWindow [$widget GetRenderWindow]
   
   if {[$CurrentRenderWindow GetInAbortCheck] == 0} {
      set renderers [$CurrentRenderWindow GetRenderers]
      $renderers InitTraversal; set CurrentRenderer [$renderers GetNextItem]
      $CurrentRenderer ResetCamera
      
      Render
   }
}

proc Wireframe {} {
    global CurrentRenderer

    set actors [$CurrentRenderer GetActors]

    $actors InitTraversal
    set actor [$actors GetNextItem]
    while { $actor != "" } {
        [$actor GetProperty] SetWireframe
        set actor [$actors GetNextItem]
    }

    Render
}

proc Surface {} {
    global CurrentRenderer

    set actors [$CurrentRenderer GetActors]

    $actors InitTraversal
    set actor [$actors GetNextItem]
    while { $actor != "" } {
        [$actor GetProperty] SetSurface
        set actor [$actors GetNextItem]
    }

    Render
}


## Procedure should be called to set bindings and initialize variables
#
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/WidgetObject.tcl

proc BindTkRenderWidget {widget} {
    bind $widget <Any-ButtonPress> {StartMotion %W %x %y}
    bind $widget <Any-ButtonRelease> {EndMotion %W %x %y}
    bind $widget <B1-Motion> {Rotate %W %x %y}
    bind $widget <B2-Motion> {Pan %W %x %y}
    bind $widget <B3-Motion> {Zoom %W %x %y}
    bind $widget <Shift-B1-Motion> {Pan %W %x %y}
    bind $widget <KeyPress-r> {Reset %W %x %y}
    bind $widget <KeyPress-u> {wm deiconify .vtkInteract}
    bind $widget <KeyPress-w> {Wireframe %W}
    bind $widget <KeyPress-s> {Surface %W}
    bind $widget <KeyPress-p> {PickActor %W %x %y}
    bind $widget <Enter> {Enter %W %x %y}
    bind $widget <Leave> {focus $oldFocus}
    bind $widget <Expose> {Expose %W}
}

# a litle more complex than just "bind $widget <Expose> {%W Render}"
# we have to handle all pending expose events otherwise they que up.
proc Expose {widget} {
   if {[GetWidgetVariableValue $widget InExpose] == 1} {
      return
   }
   SetWidgetVariableValue $widget InExpose 1
   update
   [$widget GetRenderWindow] Render
   SetWidgetVariableValue $widget InExpose 0
}

# Global variable keeps track of whether active renderer was found
set RendererFound 0

# Create event bindings
#
proc Render {widget} {
    global CurrentCamera CurrentLight

    eval $CurrentLight SetPosition [$CurrentCamera GetPosition]
    eval $CurrentLight SetFocalPoint [$CurrentCamera GetFocalPoint]

    $widget Render
}

proc UpdateRenderer {widget x y} {
    global CurrentCamera CurrentLight 
    global CurrentRenderWindow CurrentRenderer
    global RendererFound LastX LastY
    global WindowCenterX WindowCenterY

    # Get the renderer window dimensions
    set WindowX [lindex [$widget configure -width] 4]
    set WindowY [lindex [$widget configure -height] 4]

    # Find which renderer event has occurred in
    set CurrentRenderWindow [$widget GetRenderWindow]
    set renderers [$CurrentRenderWindow GetRenderers]
    set numRenderers [$renderers GetNumberOfItems]

    $renderers InitTraversal; set RendererFound 0
    for {set i 0} {$i < $numRenderers} {incr i} {
        set CurrentRenderer [$renderers GetNextItem]
        set vx [expr double($x) / $WindowX]
        set vy [expr ($WindowY - double($y)) / $WindowY]
        set viewport [$CurrentRenderer GetViewport]
        set vpxmin [lindex $viewport 0]
        set vpymin [lindex $viewport 1]
        set vpxmax [lindex $viewport 2]
        set vpymax [lindex $viewport 3]
        if { $vx >= $vpxmin && $vx <= $vpxmax && \
        $vy >= $vpymin && $vy <= $vpymax} {
            set RendererFound 1
            set WindowCenterX [expr double($WindowX)*(($vpxmax - $vpxmin)/2.0\
                                + $vpxmin)]
            set WindowCenterY [expr double($WindowY)*(($vpymax - $vpymin)/2.0\
                                + $vpymin)]
            break
        }
    }
    
    set CurrentCamera [$CurrentRenderer GetActiveCamera]
    set lights [$CurrentRenderer GetLights]
    $lights InitTraversal; set CurrentLight [$lights GetNextItem]
   
    set LastX $x
    set LastY $y
}

proc Enter {widget x y} {
    global oldFocus

    set oldFocus [focus]
    focus $widget
    UpdateRenderer $widget $x $y
}

proc StartMotion {widget x y} {
    global CurrentCamera CurrentLight 
    global CurrentRenderWindow CurrentRenderer
    global LastX LastY
    global RendererFound

    UpdateRenderer $widget $x $y
    if { ! $RendererFound } { return }

    $CurrentRenderWindow SetDesiredUpdateRate 5.0
}

proc EndMotion {widget x y} {
    global CurrentRenderWindow
    global RendererFound

    if { ! $RendererFound } {return}
    $CurrentRenderWindow SetDesiredUpdateRate 0.01
    Render $widget
}

proc Rotate {widget x y} {
    global CurrentCamera 
    global LastX LastY
    global RendererFound

    if { ! $RendererFound } { return }

    $CurrentCamera Azimuth [expr ($LastX - $x)]
    $CurrentCamera Elevation [expr ($y - $LastY)]
    $CurrentCamera OrthogonalizeViewUp

    set LastX $x
    set LastY $y

    Render $widget
}

proc Pan {widget x y} {
    global CurrentRenderer CurrentCamera
    global WindowCenterX WindowCenterY LastX LastY
    global RendererFound

    if { ! $RendererFound } { return }

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

    set APoint0 [expr $WindowCenterX + ($x - $LastX)]
    set APoint1 [expr $WindowCenterY - ($y - $LastY)]

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

    Render $widget
}

proc Zoom {widget x y} {
    global CurrentCamera
    global LastX LastY
    global RendererFound

    if { ! $RendererFound } { return }

    set zoomFactor [expr pow(1.02,($y - $LastY))]

    if {[$CurrentCamera GetParallelProjection]} {
      set parallelScale [expr [$CurrentCamera GetParallelScale] * $zoomFactor];
      $CurrentCamera SetParallelScale $parallelScale;
    } else {
      set clippingRange [$CurrentCamera GetClippingRange]
      set minRange [lindex $clippingRange 0]
      set maxRange [lindex $clippingRange 1]
      $CurrentCamera SetClippingRange [expr $minRange / $zoomFactor] \
                                      [expr $maxRange / $zoomFactor]
      $CurrentCamera Dolly $zoomFactor
    }

    set LastX $x
    set LastY $y

    Render $widget
}

proc Reset {widget x y} {
    global CurrentRenderWindow
    global RendererFound
    global CurrentRenderer

    # Get the renderer window dimensions
    set WindowX [lindex [$widget configure -width] 4]
    set WindowY [lindex [$widget configure -height] 4]

    # Find which renderer event has occurred in
    set CurrentRenderWindow [$widget GetRenderWindow]
    set renderers [$CurrentRenderWindow GetRenderers]
    set numRenderers [$renderers GetNumberOfItems]

    $renderers InitTraversal; set RendererFound 0
    for {set i 0} {$i < $numRenderers} {incr i} {
        set CurrentRenderer [$renderers GetNextItem]
        set vx [expr double($x) / $WindowX]
        set vy [expr ($WindowY - double($y)) / $WindowY]

        set viewport [$CurrentRenderer GetViewport]
        set vpxmin [lindex $viewport 0]
        set vpymin [lindex $viewport 1]
        set vpxmax [lindex $viewport 2]
        set vpymax [lindex $viewport 3]
        if { $vx >= $vpxmin && $vx <= $vpxmax && \
        $vy >= $vpymin && $vy <= $vpymax} {
            set RendererFound 1
            break
        }
    }

    if { $RendererFound } {$CurrentRenderer ResetCamera}

    Render $widget
}

proc Wireframe {widget} {
    global CurrentRenderer

    set actors [$CurrentRenderer GetActors]

    $actors InitTraversal
    set actor [$actors GetNextItem]
    while { $actor != "" } {
        [$actor GetProperty] SetRepresentationToWireframe
        set actor [$actors GetNextItem]
    }

    Render $widget
}

proc Surface {widget} {
    global CurrentRenderer

    set actors [$CurrentRenderer GetActors]

    $actors InitTraversal
    set actor [$actors GetNextItem]
    while { $actor != "" } {
        [$actor GetProperty] SetRepresentationToSurface
        set actor [$actors GetNextItem]
    }

    Render $widget
}

# Used to support picking operations
#
set PickedAssembly ""
vtkCellPicker ActorPicker
vtkProperty PickedProperty
    PickedProperty SetColor 1 0 0
set PrePickedProperty ""

proc PickActor {widget x y} {
    global CurrentRenderer RendererFound
    global PickedAssembly PrePickedProperty WindowY

    set WindowY [lindex [$widget configure -height] 4]

    if { ! $RendererFound } { return }
    ActorPicker Pick $x [expr $WindowY - $y - 1] 0.0 $CurrentRenderer
    set assembly [ActorPicker GetAssembly]

    if { $PickedAssembly != "" && $PrePickedProperty != "" } {
        $PickedAssembly SetProperty $PrePickedProperty
    }

    if { $assembly != "" } {
        set PickedAssembly $assembly
        set PrePickedProperty [$PickedAssembly GetProperty]
        $PickedAssembly SetProperty PickedProperty
    }

    Render $widget
}

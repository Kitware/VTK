## Procedure should be called to set bindings and initialize variables
#
source [file join [file dirname [info script]] vtkInt.tcl]
source [file join [file dirname [info script]] WidgetObject.tcl]

set TkInteractor_StartRenderMethod ""
set TkInteractor_EndRenderMethod ""
set TkInteractor_InteractiveUpdateRate 15.0
set TkInteractor_StillUpdateRate 0.1

proc BindTkRenderWidget {widget} {
    bind $widget <Any-ButtonPress> {StartMotion %W %x %y}
    bind $widget <Any-ButtonRelease> {EndMotion %W %x %y}
    bind $widget <B1-Motion> {Rotate %W %x %y}
    bind $widget <B2-Motion> {Pan %W %x %y}
    bind $widget <B3-Motion> {Zoom %W %x %y}
    bind $widget <Shift-B1-Motion> {Pan %W %x %y}
    bind $widget <Shift-B3-Motion> {RubberZoom %W %x %y}
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
    global TkInteractor_StillUpdateRate
    if {[GetWidgetVariableValue $widget InExpose] == 1} {
	return
    }
    SetWidgetVariableValue $widget InExpose 1    
    [$widget GetRenderWindow] SetDesiredUpdateRate $TkInteractor_StillUpdateRate
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
    global TkInteractor_StartRenderMethod
    global TkInteractor_EndRenderMethod

    if { $TkInteractor_StartRenderMethod != "" } {
	$TkInteractor_StartRenderMethod
    }

    eval $CurrentLight SetPosition [$CurrentCamera GetPosition]
    eval $CurrentLight SetFocalPoint [$CurrentCamera GetFocalPoint]

    $widget Render

    if { $TkInteractor_EndRenderMethod != "" } {
	$TkInteractor_EndRenderMethod
    }
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
    global TkInteractor_InteractiveUpdateRate
    global RubberZoomPerformed

    UpdateRenderer $widget $x $y
    if { ! $RendererFound } { return }

    set RubberZoomPerformed 0

    $CurrentRenderWindow SetDesiredUpdateRate $TkInteractor_InteractiveUpdateRate
}

proc EndMotion {widget x y} {
    global CurrentRenderWindow
    global RendererFound
    global TkInteractor_StillUpdateRate
    global RubberZoomPerformed
    global CurrentRenderer

    if { ! $RendererFound } {return}
    $CurrentRenderWindow SetDesiredUpdateRate $TkInteractor_StillUpdateRate


    if { $RubberZoomPerformed } {
	$CurrentRenderer RemoveProp RubberBandActor
	DoRubberZoom $widget
    }

    Render $widget
}

# Objects used to display rubberband
vtkPoints            RubberBandPoints
vtkCellArray         RubberBandLines
vtkScalars           RubberBandScalars
vtkPolyData          RubberBandPolyData
vtkPolyDataMapper2D  RubberBandMapper
vtkActor2D           RubberBandActor
vtkLookupTable       RubberBandColors

RubberBandPolyData SetPoints      RubberBandPoints
RubberBandPolyData SetLines       RubberBandLines
RubberBandMapper   SetInput       RubberBandPolyData
RubberBandMapper   SetLookupTable RubberBandColors
RubberBandActor    SetMapper      RubberBandMapper

RubberBandColors SetNumberOfTableValues 2
RubberBandColors SetNumberOfColors 2
RubberBandColors SetTableValue 0 1.0 0.0 0.0 1.0
RubberBandColors SetTableValue 1 1.0 1.0 1.0 1.0

[RubberBandPolyData GetPointData] SetScalars RubberBandScalars

RubberBandMapper SetScalarRange 0 1

RubberBandPoints InsertPoint 0  0  0  0
RubberBandPoints InsertPoint 1  0 10  0
RubberBandPoints InsertPoint 2 10 10  0
RubberBandPoints InsertPoint 3 10  0  0

RubberBandLines  InsertNextCell 5
RubberBandLines  InsertCellPoint 0
RubberBandLines  InsertCellPoint 1
RubberBandLines  InsertCellPoint 2
RubberBandLines  InsertCellPoint 3
RubberBandLines  InsertCellPoint 0

RubberBandScalars InsertNextScalar 0
RubberBandScalars InsertNextScalar 1
RubberBandScalars InsertNextScalar 0
RubberBandScalars InsertNextScalar 1

RubberBandMapper ScalarVisibilityOn

# Called when the mouse button is release - do the zoom
proc DoRubberZoom { widget } {
    global CurrentCamera CurrentRenderer
    global RendererFound
    global StartRubberZoomX StartRubberZoomY
    global EndRubberZoomX EndRubberZoomY

    # Return if there is no renderer, or the rubber band is less
    # that 5 pixels in either direction
    if { ! $RendererFound } { return }
    if { [expr $StartRubberZoomX - $EndRubberZoomX] < 5 && \
	    [expr $StartRubberZoomX - $EndRubberZoomX] > -5 } { return }
    if { [expr $StartRubberZoomY - $EndRubberZoomY] < 5 && \
	    [expr $StartRubberZoomY - $EndRubberZoomY] > -5 } { return }
    
    # We'll need the window height later
    set WindowY [lindex [$widget configure -height] 4]

    # What is the center of the rubber band box in pixels?
    set centerX [expr ($StartRubberZoomX + $EndRubberZoomX)/2.0]
    set centerY [expr ($StartRubberZoomY + $EndRubberZoomY)/2.0]

    # Convert the focal point to a display coordinate in order to get the
    # depth of the focal point in display units
    set FPoint [$CurrentCamera GetFocalPoint]
        set FPoint0 [lindex $FPoint 0]
        set FPoint1 [lindex $FPoint 1]
        set FPoint2 [lindex $FPoint 2]
    $CurrentRenderer SetWorldPoint $FPoint0 $FPoint1 $FPoint2 1.0
    $CurrentRenderer WorldToDisplay
    set DPoint [$CurrentRenderer GetDisplayPoint]
    set focalDepth [lindex $DPoint 2]

    # Convert the position of the camera to a display coordinate in order
    # to get the depth of the camera in display coordinates. Note this is
    # a negative number (behind the near clipping plane of 0) but it works
    # ok anyway
    set PPoint [$CurrentCamera GetPosition]
        set PPoint0 [lindex $PPoint 0]
        set PPoint1 [lindex $PPoint 1]
        set PPoint2 [lindex $PPoint 2]
    $CurrentRenderer SetWorldPoint $PPoint0 $PPoint1 $PPoint2 1.0
    $CurrentRenderer WorldToDisplay
    set DPoint [$CurrentRenderer GetDisplayPoint]
    set positionDepth [lindex $DPoint 2]

    # Find out the world position of where our new focal point should
    # be - it will be at the center of the box, back at the same focal depth
    # Don't actually set it now - we need to do all our computations before
    # we modify the camera
    $CurrentRenderer SetDisplayPoint $centerX $centerY $focalDepth
    $CurrentRenderer DisplayToWorld
    set newFocalPoint [$CurrentRenderer GetWorldPoint]
    set newFocalPoint0 [lindex $newFocalPoint 0]
    set newFocalPoint1 [lindex $newFocalPoint 1]
    set newFocalPoint2 [lindex $newFocalPoint 2]
    set newFocalPoint3 [lindex $newFocalPoint 3]
    if { $newFocalPoint3 != 0.0 } {
        set newFocalPoint0 [expr $newFocalPoint0 / $newFocalPoint3]
        set newFocalPoint1 [expr $newFocalPoint1 / $newFocalPoint3]
        set newFocalPoint2 [expr $newFocalPoint2 / $newFocalPoint3]
    }

    # Find out where the new camera position will be - at the center of
    # the rubber band box at the position depth. Don't set it yet...
    $CurrentRenderer SetDisplayPoint $centerX $centerY $positionDepth
    $CurrentRenderer DisplayToWorld
    set newPosition [$CurrentRenderer GetWorldPoint]
    set newPosition0 [lindex $newPosition 0]
    set newPosition1 [lindex $newPosition 1]
    set newPosition2 [lindex $newPosition 2]
    set newPosition3 [lindex $newPosition 3]
    if { $newPosition3 != 0.0 } {
        set newPosition0 [expr $newPosition0 / $newPosition3]
        set newPosition1 [expr $newPosition1 / $newPosition3]
        set newPosition2 [expr $newPosition2 / $newPosition3]
    }

    # We figured out how to position the camera to be centered, now we
    # need to "zoom". In parallel, this is simple since we only need to
    # change our parallel scale to encompass the entire y range of the
    # rubber band box. In perspective, we assume the box is drawn on the
    # near plane - this means that it is not possible that someone can
    # draw a rubber band box around a nearby object and dolly past it. It 
    # also means that you won't get very close to distance objects - but that
    # seems better than getting lost.
    if {[$CurrentCamera GetParallelProjection]} {
	# the new scale is just based on the y size of the rubber band box
	# compared to the y size of the window
	set ydiff [expr ($StartRubberZoomX - $EndRubberZoomX)]
	if { $ydiff < 0.0 } { set ydiff [expr $ydiff * -1.0] }
	set newScale [$CurrentCamera GetParallelScale]
	set newScale [expr $newScale * $ydiff / $WindowY]

	# now we can actually modify the camera
	$CurrentCamera SetFocalPoint $newFocalPoint0 $newFocalPoint1 $newFocalPoint2
	$CurrentCamera SetPosition $newPosition0 $newPosition1 $newPosition2
	$CurrentCamera SetParallelScale $newScale

    } else {
	# find out the center of the rubber band box on the near plane
	$CurrentRenderer SetDisplayPoint $centerX $centerY 0.0
	$CurrentRenderer DisplayToWorld
	set nearFocalPoint [$CurrentRenderer GetWorldPoint]
	set nearFocalPoint0 [lindex $nearFocalPoint 0]
	set nearFocalPoint1 [lindex $nearFocalPoint 1]
	set nearFocalPoint2 [lindex $nearFocalPoint 2]
	set nearFocalPoint3 [lindex $nearFocalPoint 3]
	if { $nearFocalPoint3 != 0.0 } {
	    set nearFocalPoint0 [expr $nearFocalPoint0 / $nearFocalPoint3]
	    set nearFocalPoint1 [expr $nearFocalPoint1 / $nearFocalPoint3]
	    set nearFocalPoint2 [expr $nearFocalPoint2 / $nearFocalPoint3]
	}

	# find the world coordinates of the point centered on the rubber band box
	# in x, on the border in y, and at the near plane depth.
	$CurrentRenderer SetDisplayPoint $centerX $StartRubberZoomY  0.0
	$CurrentRenderer DisplayToWorld
	set focalEdge [$CurrentRenderer GetWorldPoint]
        set focalEdge0 [lindex $focalEdge 0]
        set focalEdge1 [lindex $focalEdge 1]
        set focalEdge2 [lindex $focalEdge 2]
        set focalEdge3 [lindex $focalEdge 3]
	if { $focalEdge3 != 0.0 } {
	    set focalEdge0 [expr $focalEdge0 / $focalEdge3]
	    set focalEdge1 [expr $focalEdge1 / $focalEdge3]
	    set focalEdge2 [expr $focalEdge2 / $focalEdge3]
	}

	# how far is this "rubberband edge point" from the focal point?
	set ydist [expr \
		sqrt( \
		($nearFocalPoint0 - $focalEdge0)*($nearFocalPoint0 - $focalEdge0) + \
		($nearFocalPoint1 - $focalEdge1)*($nearFocalPoint1 - $focalEdge1) + \
		($nearFocalPoint2 - $focalEdge2)*($nearFocalPoint2 - $focalEdge2) )]

	# We need to know how far back we must be so that when we view the scene
	# with the current view angle, we see all of the y range of the rubber
	# band box. Use a simple tangent equation - opposite / adjacent = tan theta
	# where opposite is half the y height of the rubber band box on the near
	# plane, adjacent is the distance we are solving for, and theta is half
	# the viewing angle. This distance that we solve for is the new distance
	# to the near plane - to find the new distance to the focal plane we
	# must take the old distance to the focal plane, subtract the near plane
	# distance, and add in the distance we solved for.
	set angle [expr 0.5 * (3.141592 / 180.0) * [$CurrentCamera GetViewAngle]]
	set d [expr $ydist/tan($angle)]
	set range [$CurrentCamera GetClippingRange]
	set nearplane [lindex $range 0]
	set factor [expr [$CurrentCamera GetDistance] / \
		([$CurrentCamera GetDistance] - $nearplane + $d)]

	# now we can actually modify the camera
	$CurrentCamera SetFocalPoint $newFocalPoint0 $newFocalPoint1 $newFocalPoint2
	$CurrentCamera SetPosition $newPosition0 $newPosition1 $newPosition2
	$CurrentCamera Dolly $factor
	$CurrentRenderer ResetCameraClippingRange
    }    
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

proc RubberZoom {widget x y} {
    global RendererFound
    global CurrentRenderer
    global RubberZoomPerformed
    global LastX LastY
    global StartRubberZoomX StartRubberZoomY
    global EndRubberZoomX EndRubberZoomY

    if { ! $RendererFound } { return }

    set WindowY [lindex [$widget configure -height] 4]

    if { ! $RubberZoomPerformed } {
	$CurrentRenderer AddProp RubberBandActor

	set StartRubberZoomX $x
	set StartRubberZoomY [expr $WindowY - $y - 1]
	
	set RubberZoomPerformed 1
    }

    set EndRubberZoomX $x
    set EndRubberZoomY [expr $WindowY - $y - 1]

    RubberBandPoints SetPoint 0 $StartRubberZoomX $StartRubberZoomY  0
    RubberBandPoints SetPoint 1 $StartRubberZoomX $EndRubberZoomY    0
    RubberBandPoints SetPoint 2 $EndRubberZoomX   $EndRubberZoomY    0
    RubberBandPoints SetPoint 3 $EndRubberZoomX   $StartRubberZoomY  0

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
    global CurrentCamera CurrentRenderer
    global LastX LastY
    global RendererFound

    if { ! $RendererFound } { return }

    set zoomFactor [expr pow(1.02,(0.5*($y - $LastY)))]

    if {[$CurrentCamera GetParallelProjection]} {
	set parallelScale [expr [$CurrentCamera GetParallelScale] * $zoomFactor];
	$CurrentCamera SetParallelScale $parallelScale;
    } else {
	$CurrentCamera Dolly $zoomFactor
	$CurrentRenderer ResetCameraClippingRange
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
        # release hold on the property
        $PrePickedProperty UnRegister $PrePickedProperty
        set PrePickedProperty ""
    }

    if { $assembly != "" } {
        set PickedAssembly $assembly
        set PrePickedProperty [$PickedAssembly GetProperty]
        # hold onto the property
        $PrePickedProperty Register $PrePickedProperty
        $PickedAssembly SetProperty PickedProperty
    }

    Render $widget
}

source ../../examplesTcl/WidgetObject.tcl
source ../../examplesTcl/vtkInt.tcl


proc BindTkImageViewer {widget} {
   # to avoid queing up multple expose events.
   SetWidgetVariableValue $widget Rendering 0

   set imager [[$widget GetImageViewer] GetImager]

   # stuff for window level text.
   set mapper [NewWidgetObject $widget vtkTextMapper Mapper1]
     $mapper SetInput "none"
     $mapper SetFontFamilyToTimes
     $mapper SetFontSize 18
     $mapper BoldOn
     $mapper ShadowOn
   set coordinate [NewWidgetObject $widget vtkCoordinate Coordinate1]
     $coordinate SetCoordinateSystemToViewport
     $coordinate SetValue 4 22
   set actor [NewWidgetObject $widget vtkActor2D Actor1]
     $actor SetMapper $mapper
     $actor SetLayerNumber 1
     $actor SetPositionCoordinate $coordinate
     $actor SetVisibility 0
   $imager AddActor2D $actor
   
   # stuff for window level text.
   set mapper [NewWidgetObject $widget vtkTextMapper Mapper2]
     $mapper SetInput "none"
     $mapper SetFontFamilyToTimes
     $mapper SetFontSize 18
     $mapper BoldOn
     $mapper ShadowOn
   set coordinate [NewWidgetObject $widget vtkCoordinate Coordinate2]
     $coordinate SetCoordinateSystemToViewport
     $coordinate SetValue 4 4
   set actor [NewWidgetObject $widget vtkActor2D Actor2]
     $actor SetMapper $mapper
     $actor SetLayerNumber 1
     $actor SetPositionCoordinate $coordinate
     $actor SetVisibility 0
   $imager AddActor2D $actor
   
   # bindings
   # window level
   bind $widget <ButtonPress-1> {StartWindowLevelInteraction %W %x %y}
   bind $widget <B1-Motion> {UpdateWindowLevelInteraction %W %x %y}
   bind $widget <ButtonRelease-1> {EndWindowLevelInteraction %W}

   # Get the value
   bind $widget <ButtonPress-2> {StartQueryInteraction %W %x %y}
   bind $widget <B2-Motion> {UpdateQueryInteraction %W %x %y}
   bind $widget <ButtonRelease-2> {EndQueryInteraction %W}

   bind $widget <Expose> {ExposeTkImageViewer %W %x %y %w %h}
   bind $widget <Enter> {EnterTkViewer %W}
   bind $widget <Leave> {LeaveTkViewer %W}
   bind $widget <KeyPress-u> {wm deiconify .vtkInteract}
}


proc EnterTkViewer {widget} {
   SetWidgetVariableValue $widget OldFocus [focus]
   focus $widget
}

proc LeaveTkViewer {widget} {
   set old [GetWidgetVariableValue $widget OldFocus]
   if {$old != ""} {
      focus $old
   }
}

proc ExposeTkImageViewer {widget x y w h} {
   # Do not render if we are already rendering
   if {[GetWidgetVariableValue $widget Rendering] == 1} {
      #puts "Abort Expose: x = $x,  y = $y"
      return
   }

   # empty the que of any other expose events
   SetWidgetVariableValue $widget Rendering 1
   update
   SetWidgetVariableValue $widget Rendering 0

   # ignore the region to redraw for now.
   #puts "Expose: x = $x,  y = $y"
   $widget Render
}

proc StartWindowLevelInteraction {widget x y} {
   SetWidgetVariableValue $widget X $x
   SetWidgetVariableValue $widget Y $y
   set actor [GetWidgetObject $widget Actor1]
   $actor SetVisibility 1
   set actor [GetWidgetObject $widget Actor2]
   $actor SetVisibility 1

   UpdateWindowLevelInteraction $widget $x $y
}


proc EndWindowLevelInteraction {widget} {
   set actor [GetWidgetObject $widget Actor1]
   $actor SetVisibility 0
   set actor [GetWidgetObject $widget Actor2]
   $actor SetVisibility 0
   $widget Render
}


# try to scale window level appropriately.
# do not let window go below 0.5, do not let abs(level) go below 1.0.
proc UpdateWindowLevelInteraction {widget x y} {
   set viewer [$widget GetImageViewer]

   # get old x, y values to compute delta
   set old_x [GetWidgetVariableValue $widget X]
   set old_y [GetWidgetVariableValue $widget Y]
   # record new x, y values
   SetWidgetVariableValue $widget X $x
   SetWidgetVariableValue $widget Y $y

   # get the widgets dimensions
   set width [lindex [$widget configure -width] 4]
   set height [lindex [$widget configure -height] 4]

   # get the old window level values
   set window [$viewer GetColorWindow]
   set level [$viewer GetColorLevel]

   # conditions might not be necessary since I fixed the x = y bug.
   set window [expr $window + (0.0 + $x - $old_x) * $window / $width]
   if {$level > 0.0} {
      set level [expr $level + (0.0 + $y - $old_y) * $level / $height]
   } else {
      set level [expr $level + (0.0 + $old_y - $y) * $level / $height]
   }

   # impose some minimum (if window == 0 ...)
   if {$window < 0.5} {
      set $window 0.5
   }
   if {$level < 1.0 && $level > -1.0} {
      if {$y > $old_y} {
	 set level 1.0
      } else {
	 set level -1.0
      }
   }

   $viewer SetColorWindow $window
   $viewer SetColorLevel $level

   set mapper [GetWidgetObject $widget Mapper1]
   $mapper SetInput "Window: $window"

   set mapper [GetWidgetObject $widget Mapper2]
   $mapper SetInput "Level: $level"

   $widget Render
}



# ----------- Query PixleValue stuff ---------------

proc StartQueryInteraction {widget x y} {
   set actor [GetWidgetObject $widget Actor2]
   $actor SetVisibility 1

   UpdateQueryInteraction $widget $x $y
}


proc EndQueryInteraction {widget} {
   set actor [GetWidgetObject $widget Actor2]
   $actor SetVisibility 0
   $widget Render
}


proc UpdateQueryInteraction {widget x y} {
   set viewer [$widget GetImageViewer]
   set input [$viewer GetInput]
   set z [$viewer GetZSlice]

   # y is flipped upside down
   set height [lindex [$widget configure -height] 4]
   set y [expr $height - $y]

   $input SetUpdateExtent $x $x $y $y $z $z
   set data [$input UpdateAndReturnData]
   set numComps [$data GetNumberOfScalarComponents]
   set str ""
   for {set idx 0} {$idx < $numComps} {incr idx} {
      set val [$data GetScalarComponentAsFloat $x $y $z $idx]
      set str [format "%s  %.1f" $str $val]
   }

   set mapper [GetWidgetObject $widget Mapper2]
   $mapper SetInput "($x, $y): $str"

   $widget Render
}


package provide vtkinteraction 4.0

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
   set actor [NewWidgetObject $widget vtkActor2D Actor1]
     $actor SetMapper $mapper
     $actor SetLayerNumber 1
     [$actor GetPositionCoordinate] SetValue 4 22
     [$actor GetProperty] SetColor 1 1 0.5
     $actor SetVisibility 0
   $imager AddActor2D $actor
   
   # stuff for window level text.
   set mapper [NewWidgetObject $widget vtkTextMapper Mapper2]
     $mapper SetInput "none"
     $mapper SetFontFamilyToTimes
     $mapper SetFontSize 18
     $mapper BoldOn
     $mapper ShadowOn
   set actor [NewWidgetObject $widget vtkActor2D Actor2]
     $actor SetMapper $mapper
     $actor SetLayerNumber 1
     [$actor GetPositionCoordinate] SetValue 4 4
     [$actor GetProperty] SetColor 1 1 0.5
     $actor SetVisibility 0
   $imager AddActor2D $actor
   
   # bindings
   # window level
   bind $widget <ButtonPress-1> {StartWindowLevelInteraction %W %x %y}
   bind $widget <B1-Motion> {UpdateWindowLevelInteraction %W %x %y}
   bind $widget <ButtonRelease-1> {EndWindowLevelInteraction %W}

   # Get the value
   bind $widget <ButtonPress-3> {StartQueryInteraction %W %x %y}
   bind $widget <B3-Motion> {UpdateQueryInteraction %W %x %y}
   bind $widget <ButtonRelease-3> {EndQueryInteraction %W}

   bind $widget <Expose> {ExposeTkImageViewer %W %x %y %w %h}
   bind $widget <Enter> {EnterTkViewer %W}
   bind $widget <Leave> {LeaveTkViewer %W}
   bind $widget <KeyPress-e> {exit}
   bind $widget <KeyPress-u> {wm deiconify .vtkInteract}
   bind $widget <KeyPress-r> {ResetTkImageViewer %W}
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
   set viewer [$widget GetImageViewer]

   # save the starting mouse position and the corresponding window/level
   SetWidgetVariableValue $widget X $x
   SetWidgetVariableValue $widget Y $y
   SetWidgetVariableValue $widget Window [$viewer GetColorWindow]
   SetWidgetVariableValue $widget Level [$viewer GetColorLevel]

   #puts "------------------------------------"
   #puts "start: ($x, $y), w = [$viewer GetColorWindow], l =[$viewer GetColorLevel] "

   # make the window level text visible
   set actor [GetWidgetVariableValue $widget Actor1]
   $actor SetVisibility 1
   set actor [GetWidgetVariableValue $widget Actor2]
   $actor SetVisibility 1

   UpdateWindowLevelInteraction $widget $x $y
}


proc EndWindowLevelInteraction {widget} {
   set actor [GetWidgetVariableValue $widget Actor1]
   $actor SetVisibility 0
   set actor [GetWidgetVariableValue $widget Actor2]
   $actor SetVisibility 0
   $widget Render
}


# clicking on the window sets up sliders with current value at mouse,
# and scaled so that the whole window represents x4 change.
proc UpdateWindowLevelInteraction {widget x y} {
   set viewer [$widget GetImageViewer]

   # get the widgets dimensions
   set width [lindex [$widget configure -width] 4]
   set height [lindex [$widget configure -height] 4]

   # get the old window level values
   set window [GetWidgetVariableValue $widget Window]
   set level [GetWidgetVariableValue $widget Level]

   # get starting x, y and window/level values to compute delta
   set start_x [GetWidgetVariableValue $widget X]
   set start_y [GetWidgetVariableValue $widget Y]

   # compute normalized delta
   set dx [expr 4.0 * ($x - $start_x) / $width]
   set dy [expr 4.0 * ($start_y - $y) / $height]

   # scale by current values 
   set dx [expr $dx * $window]
   set dy [expr $dy * $window]

   #puts "   update: ($x, $y), dx = $dx, dy = $dy"

   # abs so that direction does not flip
   if {$window < 0.0} {
      set dx [expr -$dx]
      set dy [expr -$dy]
   }

   # compute new window level
   set new_window [expr $dx + $window]
   if {$new_window < 0.0} {
      set new_level [expr $dy + $level]
   } else {
      set new_level [expr $level - $dy]
   }

   # zero window or level can trap the value.
   # put a limit of 1 / 100 value


   # if window is negative, then delta level should flip (down is dark).
   if {$new_window < 0.0} {set dy [expr -$dy]}


   $viewer SetColorWindow $new_window
   $viewer SetColorLevel $new_level

   set mapper [GetWidgetVariableValue $widget Mapper1]
   $mapper SetInput "Window: $new_window"

   set mapper [GetWidgetVariableValue $widget Mapper2]
   $mapper SetInput "Level: $new_level"

   $widget Render
}

# ----------- Reset: Set window level to show all values ---------------

proc ResetTkImageViewer {widget} {
   set viewer [$widget GetImageViewer]
   set input [$viewer GetInput]
   if {$input == ""} {
      return
   }
   # Get the extent in viewer
   set z [$viewer GetZSlice]
   # x, y????
   $input UpdateInformation
   set whole [$input GetWholeExtent]
   $input SetUpdateExtent [lindex $whole 0] [lindex $whole 1] \
	   [lindex $whole 2] [lindex $whole 3] $z $z
   $input Update

   set range [$input GetScalarRange]
   set low [lindex $range 0]
   set high [lindex $range 1]
   
   $viewer SetColorWindow [expr $high - $low]
   $viewer SetColorLevel [expr ($high + $low) * 0.5]

   $widget Render
}
   



# ----------- Query PixleValue stuff ---------------

proc StartQueryInteraction {widget x y} {
   set actor [GetWidgetVariableValue $widget Actor2]
   $actor SetVisibility 1

   UpdateQueryInteraction $widget $x $y
}


proc EndQueryInteraction {widget} {
   set actor [GetWidgetVariableValue $widget Actor2]
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

   # make sure point is in the whole extent of the image.
   scan [$input GetWholeExtent] "%d %d %d %d %d %d" \
     xMin xMax yMin yMax zMin zMax
   if {$x < $xMin || $x > $xMax || $y < $yMin || $y > $yMax || \
       $z < $zMin || $z > $zMax} {
      return
   }

   $input SetUpdateExtent $x $x $y $y $z $z
   $input Update
   set data $input
   set numComps [$data GetNumberOfScalarComponents]
   set str ""
   for {set idx 0} {$idx < $numComps} {incr idx} {
      set val [$data GetScalarComponentAsFloat $x $y $z $idx]
      set str [format "%s  %.1f" $str $val]
   }

   set mapper [GetWidgetVariableValue $widget Mapper2]
   $mapper SetInput "($x, $y): $str"

   $widget Render
}


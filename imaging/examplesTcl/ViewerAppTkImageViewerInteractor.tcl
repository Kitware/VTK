catch {source ../../examplesTcl/WidgetObject.tcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "$VTK_DATA" }

catch {source $VTK_TCL/vtkInt.tcl}


proc BindTkImageViewer {widget} {
   set viewer [$widget GetImageViewer]

   # to avoid queing up multple expose events.
   SetWidgetVariableValue $widget Rendering 0

   SetWidgetVariableValue $widget WindowLevelString [format "W/L: %1.0f/%1.0f" [$viewer GetColorWindow] [$viewer GetColorLevel]]
   SetWidgetVariableValue $widget PixelPositionString "Pos:"
   SetWidgetVariableValue $widget SliceString [format "Slice: %1.0f" [$viewer GetZSlice]]

   # bindings
   # window level, note the B1-motion event calls the "probe" or "query"
   # method as well as the standard window/level interaction
   bind $widget <ButtonPress-1> {StartWindowLevelInteraction %W %x %y}
   bind $widget <B1-Motion> {UpdateWindowLevelInteraction %W %x %y; UpdateQueryInteraction %W %x %y}
   bind $widget <ButtonRelease-1> {EndWindowLevelInteraction %W}

   # Change the slice, note the B3-motion 
   bind $widget <ButtonPress-3> {StartSliceInteraction %W %x %y}
   bind $widget <B3-Motion> {UpdateSliceInteraction %W %x %y}
   bind $widget <ButtonRelease-3> {EndSliceInteraction %W}

   # Handle enter, leave and motion event
   # Motion is bound to a "probe" or "query" operation
   bind $widget <Expose> {ExposeTkImageViewer %W %x %y %w %h}
   bind $widget <Enter> {EnterTkViewer %W; StartQueryInteraction %W %x %y}
   bind $widget <Leave> {EndQueryInteraction %W; LeaveTkViewer %W}
   bind $widget <Motion> {UpdateQueryInteraction %W %x %y}

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

   UpdateWindowLevelInteraction $widget $x $y
}


proc EndWindowLevelInteraction {widget} {

}


proc UpdateWindowLevelInteraction {widget x y} {
   set viewer [$widget GetImageViewer]

   # get the widgets dimensions
   set width [lindex [$widget configure -width] 4]
   set height [lindex [$widget configure -height] 4]

   # get the old window level values
   set window [GetWidgetVariableValue $widget Window]
   set level [GetWidgetVariableValue $widget Level]

   set input [$viewer GetInput]
   if {$input == ""} {
      return
   }
   # Get the extent in viewer
   set range [$input GetScalarRange]
   set low [lindex $range 0]
   set high [lindex $range 1]

   # get starting x, y and window/level values to compute delta
   set start_x [GetWidgetVariableValue $widget X]
   set start_y [GetWidgetVariableValue $widget Y]

   # compute normalized delta
   set dx [expr 1.0 * ($x - $start_x) / $width]
   set dy [expr 1.0 * ($y - $start_y) / $height]

   # scale by dynamic range
   set dx [expr $dx * ($high - $low)]
   set dy [expr $dy * ($high - $low)]

   # compute new window level
   set new_window [expr $dx + $window]
   set new_level [expr $dy + $level]

   if { $new_window < 0.0 } {
       set new_window 1
   }

   $viewer SetColorWindow $new_window
   $viewer SetColorLevel $new_level

   SetWidgetVariableValue $widget WindowLevelString [format "W/L: %1.0f/%1.0f" $new_window $new_level]

}


proc StartSliceInteraction {widget x y} {
   set viewer [$widget GetImageViewer]

   # save the starting mouse position and the corresponding slice
   SetWidgetVariableValue $widget X $x
   SetWidgetVariableValue $widget Y $y
   SetWidgetVariableValue $widget Slice [$viewer GetZSlice]

   UpdateSliceInteraction $widget $x $y
}


proc EndSliceInteraction {widget} {

}


proc UpdateSliceInteraction {widget x y} {
   set viewer [$widget GetImageViewer]

   # get the widgets dimensions
   set height [lindex [$widget configure -height] 4]

   # get the old slice value
   set slice [GetWidgetVariableValue $widget Slice]

   # get the minimum/maximum slice
   set dims [[$viewer GetInput] GetWholeExtent]
   set minSlice [lindex $dims 4]
   set maxSlice [lindex $dims 5]

   # get starting y and slice value to compute delta
   set start_y [GetWidgetVariableValue $widget Y]

   # compute normalized delta
   set dy [expr 1.0 * ($start_y - $y) / $height]

   # scale by current values 
   set dy [expr $dy * ($maxSlice - $minSlice)]

   # compute new slice
   set new_slice [expr $dy + $slice]
   if {$new_slice < $minSlice} {
      set new_slice $minSlice
   } elseif {$new_slice > $maxSlice} {
      set new_slice $maxSlice
   }
   set new_slice [expr int($new_slice)]

   $viewer SetZSlice $new_slice

   SetWidgetVariableValue $widget SliceString [format "Slice: %1.0f" $new_slice]

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
   SetWidgetVariableValue $widget WindowLevelString [format "W/L: %1.0f/%1.0f" [$viewer GetColorWindow] [$viewer GetColorLevel]]

   $widget Render
}
   



# ----------- Query PixleValue stuff ---------------

proc StartQueryInteraction {widget x y} {
   UpdateQueryInteraction $widget $x $y
}


proc EndQueryInteraction {widget} {

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
   if { $numComps > 1} {
       for {set idx 0} {$idx < $numComps} {incr idx} {
	   set val [$data GetScalarComponentAsFloat $x $y $z $idx]
	   set str [format "%s  %1.0f" $str $val]
       }
   } else {
       set val [$data GetScalarComponentAsFloat $x $y $z 0]
       set str [format "%1.0f" $val]
   }       

   SetWidgetVariableValue $widget PixelPositionString "\[$x, $y\] = $str"

   $widget Render
}


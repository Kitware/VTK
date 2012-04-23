package require vtkinteraction

# a simple user interface that manipulates window level.
# places in the tcl top window.  Looks for object named viewer

#only use this interface when not doing regression tests
if {[info commands rtExMath] != "rtExMath"} {

# Take window level parameters from viewer
proc InitializeWindowLevelInterface {} {
   global viewer sliceNumber

   # Get parameters from viewer
   set w [viewer GetColorWindow]
   set l [viewer GetColorLevel]
   set sliceNumber [viewer GetZSlice]
   set zMin [viewer GetWholeZMin]
   set zMax [viewer GetWholeZMax]
#   set zMin 0
#   set zMax 128

   frame .slice
   label .slice.label -text "Slice"
   scale .slice.scale -from $zMin -to $zMax -orient horizontal \
     -command SetSlice -variable sliceNumber
#   button .slice.up -text "Up" -command SliceUp
#   button .slice.down -text "Down" -command SliceDown

   frame .wl
   frame .wl.f1
   label .wl.f1.windowLabel -text "Window"
   scale .wl.f1.window -from 1 -to [expr $w * 2]  -orient horizontal \
     -command SetWindow -variable window
   frame .wl.f2
   label .wl.f2.levelLabel -text "Level"
   scale .wl.f2.level -from [expr $l - $w] -to [expr $l + $w] \
     -orient horizontal -command SetLevel
   checkbutton .wl.video -text "Inverse Video" -command SetInverseVideo

   # resolutions less than 1.0
   if {$w < 10} {
      set res [expr 0.05 * $w]
      .wl.f1.window configure -resolution $res -from $res -to [expr 2.0 * $w]
      .wl.f2.level configure -resolution $res \
	-from [expr 0.0 + $l - $w] -to [expr 0.0 + $l + $w]
   }

   .wl.f1.window set $w
   .wl.f2.level set $l

   frame .ex
   button .ex.exit -text "Exit" -command "exit"

   pack .slice .wl .ex -side top
   pack .slice.label .slice.scale -side left
   pack .wl.f1 .wl.f2 .wl.video -side top
   pack .wl.f1.windowLabel .wl.f1.window -side left
   pack .wl.f2.levelLabel .wl.f2.level -side left
   pack .ex.exit -side left
}

proc SetSlice { slice } {
   global sliceNumber viewer

   viewer SetZSlice $slice
   viewer Render
}

proc SetWindow window {
   global viewer video
   if {$video} {
      viewer SetColorWindow [expr -$window]
   } else {
      viewer SetColorWindow $window
   }
   viewer Render
}

proc SetLevel level {
   global viewer
   viewer SetColorLevel $level
   viewer Render
}

proc SetInverseVideo {} {
   global viewer video window
   if {$video} {
      viewer SetColorWindow [expr -$window]
   } else {
      viewer SetColorWindow $window
   }
   viewer Render
}


InitializeWindowLevelInterface

} else {
  viewer Render
}


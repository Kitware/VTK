# a simple user interface that manipulates window level.
# places in the tcl top window.  Looks for object named viewer

# Take window level parameters from viewer
proc InitializeWindowLevelInterface {} {
   global viewer sliceNumber

   # Get parameters from viewer
   set w [viewer GetColorWindow]
   set l [viewer GetColorLevel]
   set sliceNumber [viewer GetZSlice]

   frame .slice
   button .slice.up -text "Slice Up" -command SliceUp
   button .slice.down -text "Slice Down" -command SliceDown
   
   frame .wl
   frame .wl.f1
   label .wl.f1.windowLabel -text Window
   scale .wl.f1.window -from 1 -to [expr $w * 2]  -orient horizontal \
     -command SetWindow -variable window
   frame .wl.f2
   label .wl.f2.levelLabel -text Level
   scale .wl.f2.level -from [expr $l - $w] -to [expr $l + $w] \
     -orient horizontal -command SetLevel
   checkbutton .wl.video -text "Inverse Video" -command SetInverseVideo
   
   .wl.f1.window set $w
   .wl.f2.level set $l
   
   pack .slice .wl -side left
   pack .slice.up .slice.down -side top
   pack .wl.f1 .wl.f2 .wl.video -side top
   pack .wl.f1.windowLabel .wl.f1.window -side left
   pack .wl.f2.levelLabel .wl.f2.level -side left
}
   
   
proc SliceUp {} {
   global sliceNumber viewer
   set sliceNumber [expr $sliceNumber + 1]
   viewer SetZSlice $sliceNumber
   puts $sliceNumber
   viewer Render
}

proc SliceDown {} {
   global sliceNumber viewer
   set sliceNumber [expr $sliceNumber - 1]
   viewer SetZSlice $sliceNumber
   puts $sliceNumber
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


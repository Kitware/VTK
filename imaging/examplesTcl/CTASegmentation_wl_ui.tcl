catch {load vtktcl}
catch {load vtktcl}
wm withdraw .
toplevel .wl  
wm title .wl {Window & Level}
wm resizable .wl 0 0
wm withdraw .wl

frame .wl.dd -relief ridge -borderwidth 2
frame .wl.dd.slice
button .wl.dd.slice.up -text "Slice Up" -command SliceUp
button .wl.dd.slice.down -text "Slice Down" -command SliceDown
entry .wl.dd.slice.snum  -width 4 
frame .wl.dd.wl
frame .wl.dd.wl.f1
label .wl.dd.wl.f1.windowLabel -text "Window:"
scale .wl.dd.wl.f1.window -from 1 -to 2000 -length 5c -orient horizontal -command SetWindow
frame .wl.dd.wl.f2
label .wl.dd.wl.f2.levelLabel -text "Level...:"
scale .wl.dd.wl.f2.level -from 1 -to 2000 -length 5c -orient horizontal -command SetLevel

button .wl.b0 -text "Cancel" -command Cancelwl

.wl.dd.wl.f1.window set 500
.wl.dd.wl.f2.level set 1100

pack .wl.dd -side top

# packing Display Data frame
pack .wl.b0 .wl.dd.slice .wl.dd.wl -side bottom 
pack .wl.dd.slice.up .wl.dd.slice.snum .wl.dd.slice.down  -side right 
pack .wl.dd.wl.f1 .wl.dd.wl.f2  -side top 
pack .wl.dd.wl.f1.windowLabel .wl.dd.wl.f1.window -side left
pack .wl.dd.wl.f2.levelLabel .wl.dd.wl.f2.level -side left
pack .wl.dd -side top -expand 1 -in .wl -fill both -padx 2 -pady 2
pack  .wl.b0 -side left -expand 1 -in .wl -fill both -padx 2 -pady 2

global slicenumber
.wl.dd.slice.snum insert 0 $slicenumber
bind .wl.dd.slice.snum    <Return> { SetSlice }

proc SliceUp {} {
   global slicenumber viewer viewer1 numslices 
   puts [expr $numslices-1]
   if {$slicenumber < [expr $numslices-1]} {set slicenumber [expr $slicenumber + 1]}
   puts $slicenumber
   .wl.dd.slice.snum delete 0 10
   .wl.dd.slice.snum insert 0 $slicenumber
   viewer1 SetCoordinate2 $slicenumber
   viewer1 Render
   viewer SetCoordinate2 $slicenumber
   viewer Render

}

proc SliceDown {} {
   global slicenumber viewer viewer1 numslices 
   puts [expr $numslices-1]
   if {$slicenumber > 0} {set slicenumber [expr $slicenumber - 1]}
   puts  $slicenumber
   .wl.dd.slice.snum delete 0 10
   .wl.dd.slice.snum insert 0 $slicenumber
   viewer1 SetCoordinate2 $slicenumber
   viewer1 Render
   viewer SetCoordinate2 $slicenumber
   viewer Render
}

proc SetWindow window {
   global viewer viewer1 
   viewer SetColorWindow $window
   viewer Render
   viewer1 SetColorWindow $window
   viewer1 Render

}

proc SetLevel level {
   global viewer viewer1 
   viewer SetColorLevel $level
   viewer Render
   viewer1 SetColorLevel $level
   viewer1 Render

}
proc SetSlice {} {
   global slicenumber viewer viewer1 numslices 
   set slicenumber [.wl.dd.slice.snum  get]
   if {$slicenumber > [expr $numslices-1]} {set slicenumber [expr $numslices-1]}
   puts  $slicenumber
   viewer1 SetCoordinate2 $slicenumber
   viewer1 Render
   viewer SetCoordinate2 $slicenumber
   viewer Render
   

}
puts "Done"

proc  Cancelwl {} {
    wm withdraw .wl
}

package require vtk
package require vtkinteraction

# This script uses a vtkTkRenderWidget to create a
# Tk widget that is associated with a vtkRenderWindow.
#source TkInteractor.tcl


set RANGE            150
set MAX_ITERATIONS_1 $RANGE
set MAX_ITERATIONS_2 $RANGE
set XRAD             200
set YRAD             200

vtkImageMandelbrotSource mandelbrot1
  mandelbrot1 SetMaximumNumberOfIterations [expr int($MAX_ITERATIONS_1)]
  mandelbrot1 SetWholeExtent [expr -$XRAD] [expr $XRAD-1] \
                            [expr -$YRAD] [expr $YRAD-1] 0 0
  set sample [expr 1.3 / $XRAD]
  mandelbrot1 SetSampleCX $sample $sample $sample $sample 
  mandelbrot1 SetOriginCX -0.72 0.22  0.0 0.0
  mandelbrot1 SetProjectionAxes 0 1 2

vtkLookupTable table1
  table1 SetTableRange 0 $RANGE
  table1 SetNumberOfColors $RANGE
  table1 Build
  table1 SetTableValue [expr $RANGE - 1]  0.0 0.0 0.0 0.0

vtkImageMapToRGBA map1
  map1 SetInput [mandelbrot1 GetOutput]
  map1 SetLookupTable table1

vtkImageViewer viewer
  viewer SetInput [map1 GetOutput]
  viewer SetColorWindow 255.0
  viewer SetColorLevel 127.5
  [viewer GetActor2D] SetPosition $XRAD $YRAD




vtkImageMandelbrotSource mandelbrot2
  mandelbrot2 SetMaximumNumberOfIterations [expr int($MAX_ITERATIONS_2)]
  mandelbrot2 SetWholeExtent [expr -$XRAD] [expr $XRAD-1] \
                            [expr -$YRAD] [expr $YRAD-1] 0 0
  set sample [expr 1.3 / $XRAD]
  mandelbrot2 SetSampleCX $sample $sample $sample $sample 
  mandelbrot2 SetOriginCX -0.72 0.22  0.0 0.0
  mandelbrot2 SetProjectionAxes 2 3 1

vtkLookupTable table2
  table2 SetTableRange 0 $RANGE
  table2 SetNumberOfColors $RANGE
  table2 Build
  table2 SetTableValue [expr $RANGE - 1]  0.0 0.0 0.0 0.0

vtkImageMapToRGBA map2
  map2 SetInput [mandelbrot2 GetOutput]
  map2 SetLookupTable table2

vtkImageViewer viewer2
  viewer2 SetInput [map2 GetOutput]
  viewer2 SetColorWindow 256.0
  viewer2 SetColorLevel 127.5
  [viewer2 GetActor2D] SetPosition $XRAD $YRAD

# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
set top [toplevel .top] 

set f1 [frame $top.f1] 
set quit [button $top.quit  -text Quit -command exit]

pack $quit -side bottom -fill x -expand f


set reset [button $top.reset  -text Reset -command Reset]
pack $reset -side bottom -fill x -expand f


pack $f1 -side bottom  -fill both -expand t



set manFrame [frame $f1.man]
set julFrame [frame $f1.jul]

pack $manFrame $julFrame -side left -padx 3 -pady 3 -fill both -expand t


set manView [vtkTkImageViewerWidget $manFrame.view -iv viewer \
                  -width [expr $XRAD*2] -height [expr $YRAD*2]]
set manRange [label $manFrame.range -text "Mandelbrot Range: 0 - $RANGE"]
pack $manRange -side bottom -fill none -expand f
pack $manView -side bottom -fill none -expand t

set julView [vtkTkImageViewerWidget $julFrame.view -iv viewer2 \
                  -width [expr $XRAD*2] -height [expr $YRAD*2]]
set julRange [label $julFrame.range -text "Julia Range: 0 - $RANGE"]
pack $julRange -side bottom -fill none -expand f
pack $julView -side bottom -fill both -expand t


set equation [label $top.equation -text "X = X^2 + C"]

pack $equation -side bottom -fill x


focus $manView
bind $manView <ButtonPress-1> {StartZoom %x %y}
bind $manView <ButtonRelease-1> {EndZoom %x %y 1 2}
bind $manView <ButtonRelease-2> {Pan %x %y 1 2}
bind $manView <ButtonRelease-3> {ZoomOut %x %y 1 2}
bind $manView <KeyPress-u> {wm deiconify .vtkInteract}
bind $manView <KeyPress-e> {exit}
bind $manView <Expose> {Expose %W}

bind $julView <ButtonPress-1> {StartZoom %x %y}
bind $julView <ButtonRelease-1> {EndZoom %x %y 2 1}
bind $julView <ButtonRelease-2> {Pan %x %y 2 1}
bind $julView <ButtonRelease-3> {ZoomOut %x %y 2 1}
bind $julView <KeyPress-u> {wm deiconify .vtkInteract}
bind $julView <KeyPress-e> {exit}
bind $julView <Expose> {Expose %W}








set IN_EXPOSE 0
proc Expose {widget} {
  global IN_EXPOSE
  if {$IN_EXPOSE == $widget} {return}
  set IN_EXPOSE $widget
  update
  set IN_EXPOSE 0
  $widget Render
}

proc Reset {} {
  global MAX_ITERATIONS_1 MAX_ITERATIONS_2 RANGE XRAD

  set MAX_ITERATIONS_2 $RANGE
  set MAX_ITERATIONS_1 $RANGE

  set sample [expr 1.3 / $XRAD]
  mandelbrot1 SetSampleCX $sample $sample $sample $sample 
  mandelbrot1 SetOriginCX -0.72 0.22  0.0 0.0

  set sample [expr 1.3 / $XRAD]
  mandelbrot2 SetSampleCX $sample $sample $sample $sample 
  mandelbrot2 SetOriginCX -0.72 0.22  0.0 0.0

  MandelbrotUpdate
}

proc MandelbrotUpdate {} {
  global MAX_ITERATIONS_1 MAX_ITERATIONS_2 RANGE
  global manView julView manRange julRange

  mandelbrot1 SetMaximumNumberOfIterations [expr int($MAX_ITERATIONS_1)]
  mandelbrot2 SetMaximumNumberOfIterations [expr int($MAX_ITERATIONS_1)]
  
  set tmp [mandelbrot1 GetOriginCX]
  set cr [lindex $tmp 0]
  set ci [lindex $tmp 1]
  set xr [lindex $tmp 2]
  set xi [lindex $tmp 3]

  mandelbrot1 Update
  set tmp [[mandelbrot1 GetOutput] GetScalarRange]
  set min [lindex $tmp 0]
  set max [lindex $tmp 1]
  $manRange configure -text "C = $cr + i $ci,    Mandelbrot Range: $min - $max"
  eval table1 SetTableRange [expr $min - 1] $max
  set MAX_ITERATIONS_1 [expr $min + $RANGE]

  mandelbrot2 Update
  set tmp [[mandelbrot2 GetOutput] GetScalarRange]
  set min [lindex $tmp 0]
  set max [lindex $tmp 1]
  $julRange configure -text "X = $xr + i $xi,    Julia Range: $min - $max"
  eval table2 SetTableRange [expr $min - 1] $max
  set MAX_ITERATIONS_2 [expr $min + $RANGE]

  $manView Render
  $julView Render
}



proc StartZoom {x y} {
  global X Y

  set X $x
  set Y $y
}

# prescision good enough?
proc EndZoom {x y master slave} {
  global X Y XRAD YRAD
  
  # Tk origin in uppder left. Flip y axis. Put origin in middle. 
  set y [expr $YRAD - $y]
  set Y [expr $YRAD - $Y]
  set x [expr $x - $XRAD]
  set X [expr $X - $XRAD]

  # sort
  if {$X < $x} {
    set tmp $X
    set X $x
    set x $tmp
  }
  if {$Y < $y} {
    set tmp $Y
    set Y $y
    set y $tmp
  }

  # middle/radius
  set xMid [expr 0.5 * ($x + $X)]
  set yMid [expr 0.5 * ($y + $Y)]
  set xDim [expr ($X - $x)]
  set yDim [expr ($Y - $y)]

  # determine scale
  if { $xDim <= 4 && $yDim <= 4} {
    # Box too small.  Zoom into point.
    set scale 0.5
  } else {
    # relative to window dimensions
    set xDim [expr 1.0 * $xDim / (2*$XRAD)]
    set yDim [expr 1.0 * $yDim / (2*$YRAD)]
    # take the largest
    if {$xDim > $yDim} {
      set scale $xDim
    } else {
      set scale $yDim
    }
  }

  mandelbrot$master Pan $xMid $yMid 0.0
  mandelbrot$master Zoom $scale
  mandelbrot$slave CopyOriginAndSample mandelbrot$master

  MandelbrotUpdate
}

proc Pan {x y master slave} {
  global XRAD YRAD
    
  # Tk origin in uppder left. Flip y axis. Put origin in middle. 
  set x [expr $x - $XRAD]
  set y [expr $YRAD - $y]

  set scale 2.0

  # Compute new origin.

  mandelbrot$master Pan $x $y 0.0
  mandelbrot$slave CopyOriginAndSample mandelbrot$master

  MandelbrotUpdate
}

proc ZoomOut {x y master slave} {
  global XRAD YRAD
  
  # Tk origin in uppder left. Flip y axis. Put origin in middle. 
  set x [expr $x - $XRAD]
  set y [expr $YRAD - $y]

  set scale 2.0

  # Compute new origin.

  mandelbrot$master Pan $x $y 0.0
  mandelbrot$master Zoom $scale
  mandelbrot$slave CopyOriginAndSample mandelbrot$master

  MandelbrotUpdate
}


MandelbrotUpdate

 

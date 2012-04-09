#
# This script uses a vtkTkRenderWidget to create a
# Tk widget that is associated with a vtkRenderWindow.
#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

set RANGE            150
set MAX_ITERATIONS_1 $RANGE
set MAX_ITERATIONS_2 $RANGE
set XRAD             200
set YRAD             200

#
# Create firt Mandelbrot viewer
#
vtkImageMandelbrotSource mandelbrot1
  mandelbrot1 SetMaximumNumberOfIterations [expr int($MAX_ITERATIONS_1)]
  mandelbrot1 SetWholeExtent [expr -$XRAD] [expr $XRAD-1] \
                             [expr -$YRAD] [expr $YRAD-1] \
                             0 0
  set sample [expr 1.3 / $XRAD]
  mandelbrot1 SetSampleCX $sample $sample $sample $sample
  mandelbrot1 SetOriginCX -0.72 0.22  0.0 0.0
  mandelbrot1 SetProjectionAxes 0 1 2

vtkLookupTable table1
  table1 SetTableRange 0 $RANGE
  table1 SetNumberOfColors $RANGE
  table1 Build
  table1 SetTableValue [expr $RANGE - 1]  0.0 0.0 0.0 0.0

vtkImageMapToColors map1
  map1 SetInputConnection [mandelbrot1 GetOutputPort]
  map1 SetOutputFormatToRGB
  map1 SetLookupTable table1

vtkImageViewer viewer
  viewer SetInputConnection [map1 GetOutputPort]
  viewer SetColorWindow 255.0
  viewer SetColorLevel 127.5
  [viewer GetActor2D] SetPosition $XRAD $YRAD

#
# Create second Mandelbrot viewer
#
vtkImageMandelbrotSource mandelbrot2
  mandelbrot2 SetMaximumNumberOfIterations [expr int($MAX_ITERATIONS_2)]
  mandelbrot2 SetWholeExtent [expr -$XRAD] [expr $XRAD-1] \
                             [expr -$YRAD] [expr $YRAD-1] \
                             0 0
  set sample [expr 1.3 / $XRAD]
  mandelbrot2 SetSampleCX $sample $sample $sample $sample
  mandelbrot2 SetOriginCX -0.72 0.22  0.0 0.0
  mandelbrot2 SetProjectionAxes 2 3 1

vtkLookupTable table2
  table2 SetTableRange 0 $RANGE
  table2 SetNumberOfColors $RANGE
  table2 Build
  table2 SetTableValue [expr $RANGE - 1]  0.0 0.0 0.0 0.0

vtkImageMapToColors map2
  map2 SetInputConnection [mandelbrot2 GetOutputPort]
  map2 SetOutputFormatToRGB
  map2 SetLookupTable table2

vtkImageViewer viewer2
  viewer2 SetInputConnection [map2 GetOutputPort]
  viewer2 SetColorWindow 256.0
  viewer2 SetColorLevel 127.5
  [viewer2 GetActor2D] SetPosition $XRAD $YRAD

#
# Create the GUI: two vtkTkImageViewer widgets and a quit/reset buttons
#
wm withdraw .
set top [toplevel .top]
wm protocol .top WM_DELETE_WINDOW ::vtk::cb_exit
wm title .top "Mandelbrot Viewer"

set f1 [frame $top.f1]

pack $f1 \
        -side bottom \
        -fill both -expand f

#
# Create the image viewer widget for set 1
#
set manFrame [frame $f1.man]
set julFrame [frame $f1.jul]

pack $manFrame $julFrame \
        -side left \
        -padx 3 -pady 3 \
        -fill both -expand f

set manView [vtkTkImageViewerWidget $manFrame.view \
        -iv viewer \
        -width [expr $XRAD*2] \
        -height [expr $YRAD*2]]

set manRange [label $manFrame.range \
        -text "Mandelbrot Range: 0 - $RANGE"]

set quit [button $manFrame.quit  \
        -text "Quit" \
        -command ::vtk::cb_exit]

#
# Create the image viewer widget for set 2
#
pack $manView $manRange $quit \
        -side top \
        -padx 2 -pady 2 \
        -fill both -expand f

set julView [vtkTkImageViewerWidget $julFrame.view \
        -iv viewer2 \
        -width [expr $XRAD*2] \
        -height [expr $YRAD*2]]

set julRange [label $julFrame.range \
        -text "Julia Range: 0 - $RANGE"]

set reset [button $julFrame.reset  \
        -text "Reset" \
        -command Reset]

pack $julView $julRange $reset \
        -side top \
        -padx 2 -pady 2 \
        -fill both -expand f
#
# The equation label
#
set equation [label $top.equation \
        -text "X = X^2 + C"]

pack $equation \
        -side bottom \
        -fill x -expand f

#
# Create the default bindings
#
::vtk::bind_tk_imageviewer_widget $manView
::vtk::bind_tk_imageviewer_widget $julView

#
# Override some of the bindings
#
foreach {widget master slave} [list $manView 1 2 $julView 2 1] {

    set iren [[[$widget GetImageViewer] GetRenderWindow] GetInteractor]
    set istyle [$iren GetInteractorStyle]

    # Zoom in

    $istyle RemoveObservers LeftButtonPressEvent
    $istyle AddObserver LeftButtonPressEvent \
            [list StartZoom $iren]

    $istyle RemoveObservers LeftButtonReleaseEvent
    $istyle AddObserver LeftButtonReleaseEvent \
            [list EndZoom $iren $master $slave]

    # Zoom out

    $istyle RemoveObservers RightButtonPressEvent
    $istyle AddObserver RightButtonPressEvent \
            [list ZoomOut $iren $master $slave]

    $istyle RemoveObservers RightButtonReleaseEvent

    # Pan

    $istyle RemoveObservers MiddleButtonPressEvent
    $istyle AddObserver MiddleButtonPressEvent \
            [list Pan $iren $master $slave]

    $istyle RemoveObservers MiddleButtonReleaseEvent
}

#
# Update the display
#
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

#
# Reset both sets
#
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

#
# Zoom in
#
proc StartZoom {iren} {
  global X Y

  set pos [$iren GetEventPosition]
  set X [lindex $pos 0]
  set Y [lindex $pos 1]
}

proc EndZoom {iren master slave} {
  global X Y XRAD YRAD

  # Put origin in middle.

  set pos [$iren GetEventPosition]
  set x [expr [lindex $pos 0] - $XRAD]
  set y [expr [lindex $pos 1] - $YRAD]
  set X [expr $X - $XRAD]
  set Y [expr $Y - $YRAD]

  # Sort

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

  # Middle/radius

  set xMid [expr 0.5 * ($x + $X)]
  set yMid [expr 0.5 * ($y + $Y)]
  set xDim [expr ($X - $x)]
  set yDim [expr ($Y - $y)]

  # Determine scale

  if { $xDim <= 4 && $yDim <= 4} {
    # Box too small.  Zoom into point.
    set scale 0.5
  } else {
    # Relative to window dimensions
    set xDim [expr 1.0 * $xDim / (2*$XRAD)]
    set yDim [expr 1.0 * $yDim / (2*$YRAD)]
    # Take the largest
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

#
# Zoom out
#
proc ZoomOut {iren master slave} {
  global XRAD YRAD

  # Put origin in middle.

  set pos [$iren GetEventPosition]
  set x [expr [lindex $pos 0] - $XRAD]
  set y [expr [lindex $pos 1] - $YRAD]

  set scale 2.0

  # Compute new origin.

  mandelbrot$master Pan $x $y 0.0
  mandelbrot$master Zoom $scale
  mandelbrot$slave CopyOriginAndSample mandelbrot$master

  MandelbrotUpdate
}

#
# Pan
#
proc Pan {iren master slave} {
  global XRAD YRAD

  # Put origin in middle.

  set pos [$iren GetEventPosition]
  set x [expr [lindex $pos 0] - $XRAD]
  set y [expr [lindex $pos 1] - $YRAD]

  set scale 2.0

  # Compute new origin.

  mandelbrot$master Pan $x $y 0.0
  mandelbrot$slave CopyOriginAndSample mandelbrot$master

  MandelbrotUpdate
}

MandelbrotUpdate



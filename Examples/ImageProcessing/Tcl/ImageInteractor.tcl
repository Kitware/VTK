# This example shows how to use the InteractorStyleImage and add your own
# event handling.  The InteractorStyleImage is a special interactor designed
# to be used with vtkImageActor in a rendering window context. It forces the
# camera to stay perpendicular to the x-y plane. You may also wish to refer
# to the MandelbrotViewer.tcl example for comparison.

package require vtk
package require vtkinteraction

# Create the image
#
set RANGE            150
set MAX_ITERATIONS_1 $RANGE
set MAX_ITERATIONS_2 $RANGE
set XRAD             200
set YRAD             200
set sample [expr 1.3 / $XRAD]

# Create a Mandelbrot set of appropriate resolution
vtkImageMandelbrotSource mandelbrot1
  mandelbrot1 SetMaximumNumberOfIterations [expr int($MAX_ITERATIONS_1)]
  mandelbrot1 SetWholeExtent [expr -$XRAD] [expr $XRAD-1] \
                            [expr -$YRAD] [expr $YRAD-1] 0 0
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

vtkImageActor ia
ia SetInput [map1 GetOutput]

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Create an image interactor
vtkInteractorStyleImage interactor
iren SetInteractorStyle interactor
interactor AddObserver LeftButtonPressEvent {StartZoom}
interactor AddObserver LeftButtonReleaseEvent {EndZoom}

# Add the actors to the renderer, set the background and size
ren1 AddActor ia
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 400 400

# render the image
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

set cam1 [ren1 GetActiveCamera]

ren1 ResetCameraClippingRange
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

# methods to support Mandelbrot viewing
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
  global manRange

  mandelbrot1 SetMaximumNumberOfIterations [expr int($MAX_ITERATIONS_1)]
  
  set tmp [mandelbrot1 GetOriginCX]
  set cr [lindex $tmp 0]
  set ci [lindex $tmp 1]
  set xr [lindex $tmp 2]
  set xi [lindex $tmp 3]

  mandelbrot1 Update
  set tmp [[mandelbrot1 GetOutput] GetScalarRange]
  set min [lindex $tmp 0]
  set max [lindex $tmp 1]
  eval table1 SetTableRange [expr $min - 1] $max
  set MAX_ITERATIONS_1 [expr $min + $RANGE]

  renWin Render
}

proc StartZoom {} {
  global X Y

  set xy [iren GetEventPosition]
  set X [lindex $xy 0]
  set Y [lindex $xy 1]
}

# precision good enough?
proc EndZoom {} {
  global X Y XRAD YRAD
  
  set xy [iren GetEventPosition]
  set x [lindex $xy 0]
  set y [lindex $xy 1]

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

  mandelbrot1 Pan $xMid $yMid 0.0
  mandelbrot1 Zoom $scale

  MandelbrotUpdate
}

MandelbrotUpdate





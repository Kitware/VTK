catch {load vtktcl}

source vtkImageInclude.tcl
source TkImageViewerInteractor.tcl
source ../../examplesTcl/vtkInt.tcl


# This script uses a vtkTkRenderWidget to create a
# Tk widget that is associated with a vtkRenderWindow.
#source TkInteractor.tcl



set MAX_ITERATIONS 150
set XDIM 512
set YDIM 512

vtkImageMandelbrotSource mandelbrot
  mandelbrot SetMaximumNumberOfIterations $MAX_ITERATIONS
  mandelbrot SetWholeExtent 0 [expr $XDIM-1] 0 [expr $YDIM-1] 0 0
  mandelbrot SetSpacing [expr 3.0 / $XDIM]
  mandelbrot SetOrigin -2.2 -1.5 0

vtkLookupTable table
  table SetTableRange 0 $MAX_ITERATIONS
  table SetNumberOfColors $MAX_ITERATIONS
  table Build
  table SetTableValue [expr $MAX_ITERATIONS - 1]  0.0 0.0 0.0 0.0

vtkImageMapToRGBA map
  map SetInput [mandelbrot GetOutput]
  map SetLookupTable table

vtkImageViewer viewer
  viewer SetInput [map GetOutput]
  viewer SetColorWindow 78.0
  viewer SetColorLevel 17.0

# Create the GUI: two renderer widgets and a quit button
#
wm withdraw .
toplevel .top 

frame .top.f1 
pack .top.f1  -fill both -expand t

vtkTkImageViewerWidget .top.f1.r1 -width $XDIM -height $YDIM -iv viewer
#    BindTkRenderWidget .top.f1.r1

scale .top.slider -from -1.0 -to 1.0 -resolution 0.01 -variable ZPAN \
        -orient horizontal
button .top.quit  -text Quit -command exit


radiobutton .top.juliaRadio -text "Julia" -command {JuliaMandelbrotSelect} \
  -value julia -variable JULIA_MANDELBROT_RADIO -justify left \
  -command {SelectJuliaMandelbrot}
radiobutton .top.mandelbrotRadio -text "Mandelbrot" \
  -value mandelbrot -variable JULIA_MANDELBROT_RADIO -justify left \
  -command {SelectJuliaMandelbrot}
set JULIA_MANDELBROT_RADIO mandelbrot


pack .top.slider -side bottom -fill x 
pack .top.quit -side bottom -fill x
pack .top.juliaRadio -side bottom -fill x
pack .top.mandelbrotRadio -side bottom -fill x
pack .top.f1.r1 -side left -padx 3 -pady 3 -fill both -expand t


#BindTkImageViewer .top.f1.r1 

focus .top.f1.r1
bind .top.f1.r1 <ButtonPress-1> {StartZoom %x %y}
bind .top.f1.r1 <ButtonRelease-1> {EndZoom %x %y}
bind .top.f1.r1 <ButtonRelease-3> {ZoomOut %x %y}
bind .top.f1.r1 <KeyPress-u> {wm deiconify .vtkInteract}
bind .top.f1.r1 <KeyPress-e> {exit}
bind .top.f1.r1 <Expose> {Expose %W}

bind .top.slider <ButtonRelease-1> {PanZ}

set IN_EXPOSE 0
proc Expose {widget} {
  global IN_EXPOSE
  if {$IN_EXPOSE} {return}
  set IN_EXPOSE 1
  update
  set IN_EXPOSE 0
  $widget Render
}

proc MandelbrotUpdate {} {
  global MAX_ITERATIONS

  mandelbrot SetMaximumNumberOfIterations $MAX_ITERATIONS
  
  # A bug !
  #[mandelbrot GetOutput] ReleaseData
  #[map GetOutput] ReleaseData

  mandelbrot Update
  set tmp [[mandelbrot GetOutput] GetScalarRange]
  set min [lindex $tmp 0]
  set max [lindex $tmp 1]
  eval table SetTableRange [expr $min - 1] $max
  set MAX_ITERATIONS [expr $min + 150]

  .top.f1.r1 Render
}

proc SelectJuliaMandelbrot {} {
  global JULIA_MANDELBROT_RADIO XDIM YDIM
  
  if {[string compare $JULIA_MANDELBROT_RADIO "mandelbrot"] == 0} {
    mandelbrot JuliaSetOff
    mandelbrot SetWholeExtent 0 [expr $XDIM-1] 0 [expr $YDIM-1] 0 0
    mandelbrot SetSpacing [expr 3.0 / $XDIM]
    mandelbrot SetOrigin -2.2 -1.5 0
  } else {
    mandelbrot SetOrigin -1.5 -1.5 0.12
    mandelbrot SetSpacing [expr 3.0 / $XDIM]
    mandelbrot JuliaSetOn
  }

  MandelbrotUpdate
}


proc PanZ {} {
  global ZPAN XDIM

  mandelbrot Pan 0.0 0.0 [expr $ZPAN * $XDIM]

  # set the slider back to the middle
  set ZPAN 0.0

  MandelbrotUpdate
}


proc StartZoom {x y} {
  global X Y

  set X $x
  set Y $y
}

# prescision good enough?
proc EndZoom {x y} {
  global X Y XDIM YDIM
  
  # Tk origin in uppder left. Flip y axis. Put origin in lower left. 
  set y [expr $YDIM - 1 - $y]
  set Y [expr $YDIM - 1 - $Y]

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
    set xDim [expr 1.0 * $xDim / $XDIM]
    set yDim [expr 1.0 * $yDim / $YDIM]
    # take the largest
    if {$xDim > $yDim} {
      set scale $xDim
    } else {
      set scale $yDim
    }
  }

  # Compute new origin.
  set x [expr $xMid - (0.5 * $XDIM * $scale)] 
  set y [expr $yMid - (0.5 * $YDIM * $scale)] 

  mandelbrot Pan $x $y 0.0
  mandelbrot Zoom $scale

  MandelbrotUpdate
}

proc ZoomOut {x y} {
  global XDIM YDIM MAX_ITERATIONS
  
  # Tk origin in uppder left. Flip y axis. Put origin in lower left. 
  set y [expr $YDIM - 1 - $y]

  #tk_messageBox "out: $x $y"

  set scale 2.0

  # Compute new origin.
  set x [expr $x - (0.5 * $XDIM * $scale)] 
  set y [expr $y - (0.5 * $YDIM * $scale)] 

  mandelbrot Pan $x $y 0.0
  mandelbrot Zoom $scale

  MandelbrotUpdate
}


update


 
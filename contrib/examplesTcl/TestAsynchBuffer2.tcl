# This example provides interactive isosurface value selection
# using a low res data set.  The high res version is computed
# when the mouse is released.  The computation of the high res
# surface can be interrupted with another mouse selection.  
# It uses an asynch buffer so the interaction continues 
# while the high res surface is being computed.





catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
vtkLight lgt

# create pipeline
#

vtkImageReader reader
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 255 0 255 1 93
  reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
  reader SetDataMask 0x7fff
  reader SetDataSpacing 1.6 1.6 3.0
  reader Update

vtkImageShrink3D shrink
  shrink SetInput [reader GetOutput]
  shrink SetShrinkFactors 4 4 4
  shrink AveragingOff
 shrink Update


set IsoValue 1150
vtkImageMarchingCubes iso
    iso SetInput [shrink GetOutput]
    iso SetValue 0 $IsoValue
    iso ComputeGradientsOn
    iso ComputeScalarsOff

vtkAsynchronousBuffer buf
    buf SetInput [iso GetOutput]

vtkPolyDataMapper isoMapper
    isoMapper SetInput [buf GetOutput]
    isoMapper ScalarVisibilityOff
    isoMapper ImmediateModeRenderingOn

vtkActor isoActor
    isoActor SetMapper isoMapper
    set isoProp [isoActor GetProperty]
    eval $isoProp SetColor $antique_white



vtkOutlineFilter outline
    outline SetInput [reader GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
#eval $outlineProp SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
ren1 SetBackground 1 1 1
ren1 AddLight lgt
renWin SetSize 500 500
ren1 SetBackground 0.1 0.2 0.4

set cam1 [ren1 GetActiveCamera]
$cam1 Elevation 90
$cam1 SetViewUp 0 0 -1
$cam1 Zoom 1.3
eval lgt SetPosition [$cam1 GetPosition]
eval lgt SetFocalPoint [$cam1 GetFocalPoint]

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# low res quick render
renWin Render
# start an asynchronous high res render
buf BlockingOff
iso SetInput [reader GetOutput]
renWin Render

iren Initialize
#renWin SetFileName "headBone.tcl.ppm"
#renWin SaveImageAsPPM


# ----- Set up the slider, and interactive choice stuff. -----


# call back of the slider
proc SetSurfaceValue {val} {
  # This check may not be needed, but it makes things cleaner.
  if {[buf GetFinished] == 0} {
    return
  }

  iso SetValue 0 $val
}

# Called when mouse is first pressed over the slider.
proc StartInteraction {} {
  #LogMessage "start interaction"
  if {[buf GetFinished] == 0} {
    # abort the iso surface execution
    iso AbortExecuteOn
    # wait until the other thread finishes
    while {[buf GetFinished] == 0} {
      #LogMessage "waiting for iso to abort"
      # sleep 1
    }
  }

  iso SetInput [shrink GetOutput]
  buf BlockingOn
}


# Called when mouse is released.  
# Starts the generatikon of the full res model.
proc StopInteraction {} {
  #LogMessage "start interaction"
  iso SetInput [reader GetOutput]
  buf BlockingOff
}

set QUIT_FLAG 0
proc Quit {} {
  global QUIT_FLAG
  # signal loop to exit
  set QUIT_FLAG 1
}

# create a slider to set the iso surface value.
toplevel .ui
scale .ui.scale -from 0 -to 3000 -orient horizontal \
     -command SetSurfaceValue -variable IsoValue
button .ui.quit -text "Quit" -command Quit
pack .ui.scale -side top -expand t -fill both
pack .ui.quit -side top -expand t -fill both

# extra calls to change modes (interactive, full res)
bind .ui.scale <ButtonPress-1> {StartInteraction}
bind .ui.scale <ButtonRelease-1> {StopInteraction}

wm withdraw .



# we need our own little event loop to detect when our input has been changed
# by another thread


set RENDER_TIME 0

while {1} {
  global QUIT_FLAG

  # tcl handle events
  #LogMessage "update"
  update

  if {$QUIT_FLAG} {
    exit
  }

  # check if anything has changed.
  buf UpdateInformation
  set DATA_TIME [[buf GetOutput] GetPipelineMTime]
  if {$DATA_TIME > $RENDER_TIME} {
    renWin Render
    set RENDER_TIME $DATA_TIME
  } else {
    after 10
  }
}




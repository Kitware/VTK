# this example lets the user interact with a sphere,
# while an iso surface is being comp[uted in another thread.


catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

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
  reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
  reader SetDataMask 0x7fff
  reader SetDataSpacing 1.6 1.6 3.0

vtkImageShrink3D shrink
  shrink SetInput [reader GetOutput]
  shrink SetShrinkFactors 4 4 4
  shrink AveragingOn
  shrink Update

set IsoValue 1150
vtkImageMarchingCubes iso
    iso SetInput [reader GetOutput]
    iso SetValue 0 $IsoValue
    iso ComputeGradientsOn
    iso ComputeScalarsOff

vtkImageMarchingCubes isoSmall
    isoSmall SetInput [shrink GetOutput]
    isoSmall SetValue 0 $IsoValue
    isoSmall ComputeGradientsOn
    isoSmall ComputeScalarsOff


vtkPolyDataMapper isoMapper
    isoMapper SetInput [iso GetOutput]
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

renWin Render
iren Initialize
#renWin SetFileName "headBone.tcl.ppm"
#renWin SaveImageAsPPM


proc SetSurfaceValue {val} {
    iso SetValue 0 $val
    isoSmall SetValue 0 $val
	renWin Render
}

proc StartInteraction {} {
	global isoProp
    eval $isoProp SetColor 1 0 0
    isoMapper SetInput [isoSmall GetOutput]
}

proc StopInteraction {} {
	global isoProp
    eval $isoProp SetColor 1 1 1
    isoMapper SetInput [iso GetOutput]
	renWin Render
}


# create a slider to set the iso surface value.
scale .scale -from 0 -to 3000 -orient horizontal \
     -command SetSurfaceValue -variable IsoValue

pack .scale -side top -expand t -fill both
bind .scale <ButtonPress-1> {StartInteraction}
bind .scale <ButtonRelease-1> {StopInteraction}






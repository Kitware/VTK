catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Simple volume rendering example.
vtkSLCReader reader
    reader SetFileName "$VTK_DATA/spring.slc"

# Create transfer functions for opacity and color
vtkPiecewiseFunction opacityTransferFunction1
    opacityTransferFunction1 AddPoint  60   0.0
    opacityTransferFunction1 AddPoint  80   1.0

vtkPiecewiseFunction opacityTransferFunction2
    opacityTransferFunction2 AddPoint  40   0.0
    opacityTransferFunction2 AddPoint  100  0.5

vtkPiecewiseFunction opacityTransferFunction3
    opacityTransferFunction3 AddPoint  20   0.0
    opacityTransferFunction3 AddPoint 120   0.25

vtkPiecewiseFunction opacityTransferFunction4
    opacityTransferFunction4 AddPoint  10   0.0
    opacityTransferFunction4 AddPoint 150   0.1

vtkColorTransferFunction colorTransferFunction1
    colorTransferFunction1 AddRGBPoint 0   1.0 0.0 0.0
    colorTransferFunction1 AddRGBPoint 255 1.0 0.0 0.0

vtkColorTransferFunction colorTransferFunction2
    colorTransferFunction2 AddRGBPoint 0   1.0 0.5 0.0
    colorTransferFunction2 AddRGBPoint 255 1.0 0.5 0.0

vtkColorTransferFunction colorTransferFunction3
    colorTransferFunction3 AddRGBPoint 0   1.0 1.0 0.0
    colorTransferFunction3 AddRGBPoint 255 1.0 1.0 0.0

vtkColorTransferFunction colorTransferFunction4
    colorTransferFunction4 AddRGBPoint 0   0.0 1.0 0.0
    colorTransferFunction4 AddRGBPoint 255 0.0 1.0 0.0

vtkColorTransferFunction colorTransferFunction5
    colorTransferFunction5 AddRGBPoint 0   0.0 0.0 1.0
    colorTransferFunction5 AddRGBPoint 255 0.0 0.0 1.0

vtkColorTransferFunction colorTransferFunction6
    colorTransferFunction6 AddRGBPoint 0   0.7 0.0 1.0
    colorTransferFunction6 AddRGBPoint 255 0.7 0.0 1.0

# Create properties, mappers, volume actors, and ray cast function
for { set i 1 } { $i < 5 } { incr i } {
    for { set j 1 } { $j < 7 } { incr j } {
	vtkVolumeProperty volumeProperty${i}${j}
	volumeProperty${i}${j} ShadeOn
	volumeProperty${i}${j} SetInterpolationTypeToLinear
	volumeProperty${i}${j} SetColor colorTransferFunction${j}
	volumeProperty${i}${j} SetScalarOpacity opacityTransferFunction${i}
    }
}

vtkVolumeRayCastCompositeFunction  compositeFunction

vtkVolumeRayCastMapper volumeMapper
    volumeMapper SetInput [reader GetOutput]
    volumeMapper SetSampleDistance 0.25
    volumeMapper SetVolumeRayCastFunction compositeFunction

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 256 256
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

set rot 0
set tran 0
set scale 1
for { set i 1 } { $i < 5 } { incr i } {
    for { set j 1 } { $j < 7 } { incr j } {
	vtkVolume volume${i}${j}
	volume${i}${j} SetMapper volumeMapper
	volume${i}${j} SetProperty volumeProperty${i}${j}
	volume${i}${j} SetOrigin 23.5 0 23.5
	volume${i}${j} RotateX $rot
	volume${i}${j} AddPosition $tran 0 0
	volume${i}${j} SetScale $scale
	ren1 AddVolume volume${i}${j}
	incr rot 15
	incr tran 47
	set scale [expr $scale * 1.05]
    }
}


ren1 SetBackground .1 .2 .4
[ren1 GetActiveCamera] SetPosition -8000 0 23.5
[ren1 GetActiveCamera] SetFocalPoint 0 0 23.5
[ren1 GetActiveCamera] SetClippingRange 100 2000
[ren1 GetActiveCamera] SetViewUp 0 1 0

renWin Render

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .

update
[ren1 GetActiveCamera] SetPosition -800 0 23.5
renWin Render

#renWin SetFileName "valid/volRainbow.tcl.ppm"
#renWin SaveImageAsPPM



## This is a grid of MIP volumes - with 3 values permuted - the
## type of maximization (scalar value or opacity) the type of
## color (grey or RGB) and the interpolation type (nearest or linear)

catch {load vtktcl}
source ../../examplesTcl/vtkInt.tcl

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

vtkSLCReader reader

reader SetFileName "../../../vtkdata/poship.slc"

vtkPiecewiseFunction opacityTransferFunction

opacityTransferFunction AddPoint    0   0.0
opacityTransferFunction AddPoint   20   0.0
opacityTransferFunction AddPoint  120   0.25

vtkPiecewiseFunction gradopTransferFunction

gradopTransferFunction AddPoint    0   0.0
gradopTransferFunction AddPoint    5   0.0
gradopTransferFunction AddPoint   10   1.0

vtkColorTransferFunction colorTransferFunction

colorTransferFunction AddRGBPoint   0 1.0 0.0 0.0
colorTransferFunction AddRGBPoint  31 1.0 0.5 0.0
colorTransferFunction AddRGBPoint  63 1.0 1.0 0.3
colorTransferFunction AddRGBPoint  95 0.0 1.0 0.0
colorTransferFunction AddRGBPoint 127 0.3 0.7 0.5
colorTransferFunction AddRGBPoint 159 0.0 0.0 1.0
colorTransferFunction AddRGBPoint 191 1.0 0.0 1.0
colorTransferFunction AddRGBPoint 223 1.0 0.5 1.0
colorTransferFunction AddRGBPoint 255 1.0 1.0 1.0

vtkPiecewiseFunction greyTransferFunction

greyTransferFunction AddPoint    0   1.0
greyTransferFunction AddPoint  255   1.0

vtkVolumeProperty volumeProperty1
vtkVolumeProperty volumeProperty2
vtkVolumeProperty volumeProperty3
vtkVolumeProperty volumeProperty4
vtkVolumeProperty volumeProperty5
vtkVolumeProperty volumeProperty6
vtkVolumeProperty volumeProperty7
vtkVolumeProperty volumeProperty8
vtkVolumeProperty volumeProperty9
vtkVolumeProperty volumeProperty10
vtkVolumeProperty volumeProperty11
vtkVolumeProperty volumeProperty12
vtkVolumeProperty volumeProperty13
vtkVolumeProperty volumeProperty14
vtkVolumeProperty volumeProperty15
vtkVolumeProperty volumeProperty16

for { set i 0 } { $i < 16 } { incr i } {
    set p [expr $i + 1]
    set w [expr (($i %  2) / 1)]
    set x [expr (($i %  4) / 2)]
    set y [expr (($i %  8) / 4)]
    set z [expr (($i % 16) / 8)]
     volumeProperty${p} SetScalarOpacity opacityTransferFunction
    if { $w } {
	volumeProperty${p} SetColor colorTransferFunction
    } else {
	volumeProperty${p} SetColor greyTransferFunction
    }
    if { $x } {
	volumeProperty${p} SetInterpolationTypeToLinear
    } else {
	volumeProperty${p} SetInterpolationTypeToNearest
    }
    if { $y } {	
	volumeProperty${p} ShadeOn
    } else {
	volumeProperty${p} ShadeOff
    }

    if { $z } {
	volumeProperty${p} SetGradientOpacity gradopTransferFunction
    }
}

vtkVolumeRayCastCompositeFunction  CompositeFunction1
vtkVolumeRayCastCompositeFunction  CompositeFunction2

CompositeFunction1 SetCompositeMethodToInterpolateFirst
CompositeFunction2 SetCompositeMethodToClassifyFirst

vtkFiniteDifferenceGradientEstimator GradientEstimator

vtkVolumeRayCastMapper volumeMapper1
volumeMapper1 SetScalarInput [reader GetOutput]
volumeMapper1 SetVolumeRayCastFunction CompositeFunction1
volumeMapper1 SetGradientEstimator GradientEstimator
volumeMapper1 SetSampleDistance 0.3

vtkVolumeRayCastMapper volumeMapper2
volumeMapper2 SetScalarInput [reader GetOutput]
volumeMapper2 SetVolumeRayCastFunction CompositeFunction2
volumeMapper2 SetGradientEstimator GradientEstimator

for { set j 1 } { $j <= 2 } { incr j } {
    for { set i 1 } { $i <= 16 } { incr i } {
	vtkVolume volume${i}_${j}
	volume${i}_${j} SetVolumeMapper volumeMapper${j}
	volume${i}_${j} SetVolumeProperty volumeProperty${i}
	set k [expr int( ($i - 1)/8 ) + 2*($j - 1)]
	set yoff [expr 70 * $k]
	set k [expr (( $i - 1 ) % 8)]
	set xoff [expr 70 * $k]
	ren1 AddVolume volume${i}_${j}
	volume${i}_${j} AddPosition $xoff $yoff 0
    }
}


renWin SetSize 800 400
[ren1 GetActiveCamera] ParallelProjectionOn
[ren1 GetActiveCamera] SetParallelScale 140

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName "valid/volCompositeGrid.tcl.ppm"
#renWin SaveImageAsPPM

wm withdraw .


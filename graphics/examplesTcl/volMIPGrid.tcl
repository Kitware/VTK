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
opacityTransferFunction AddPoint  128   1.0
opacityTransferFunction AddPoint  255   0.0

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

volumeProperty1 SetScalarOpacity opacityTransferFunction
volumeProperty2 SetScalarOpacity opacityTransferFunction
volumeProperty3 SetScalarOpacity opacityTransferFunction
volumeProperty4 SetScalarOpacity opacityTransferFunction

volumeProperty1 SetColor colorTransferFunction
volumeProperty2 SetColor colorTransferFunction
volumeProperty3 SetColor greyTransferFunction
volumeProperty4 SetColor greyTransferFunction

volumeProperty1 SetInterpolationTypeToNearest
volumeProperty2 SetInterpolationTypeToLinear
volumeProperty3 SetInterpolationTypeToNearest
volumeProperty4 SetInterpolationTypeToLinear


vtkVolumeRayCastMIPFunction  MIPFunction1
vtkVolumeRayCastMIPFunction  MIPFunction2

MIPFunction1 SetMaximizeMethodToScalarValue
MIPFunction2 SetMaximizeMethodToOpacity

vtkVolumeRayCastMapper volumeMapper1
vtkVolumeRayCastMapper volumeMapper2

volumeMapper1 SetScalarInput [reader GetOutput]
volumeMapper1 SetVolumeRayCastFunction MIPFunction1

volumeMapper2 SetScalarInput [reader GetOutput]
volumeMapper2 SetVolumeRayCastFunction MIPFunction2

for { set i 1 } { $i <= 8 } { incr i } {
    vtkVolume volume${i}
    set j [expr int( ($i - 1)/4 )]
    set yoff [expr 70 * $j]
    incr j
    volume${i} SetVolumeMapper volumeMapper${j}
    set j [expr (( $i - 1 ) % 4)]
    set xoff [expr 70 * $j]
    incr j
    volume${i} SetVolumeProperty volumeProperty${j}
    ren1 AddVolume volume${i}
    volume${i} AddPosition $xoff $yoff 0
}

renWin SetSize 600 300
[ren1 GetActiveCamera] ParallelProjectionOn
[ren1 GetActiveCamera] SetParallelScale 70

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName "valid/volMIPGrid.tcl.ppm"
#renWin SaveImageAsPPM

wm withdraw .


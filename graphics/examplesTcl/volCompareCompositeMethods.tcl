## This is a grid of MIP volumes - with 3 values permuted - the
## type of maximization (scalar value or opacity) the type of
## color (grey or RGB) and the interpolation type (nearest or linear)
wm withdraw .
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

reader SetFileName "../../../vtkdata/sphere.slc"

vtkPiecewiseFunction opacityTransferFunction
opacityTransferFunction AddPoint    0   0.0
opacityTransferFunction AddPoint   57   0.0
opacityTransferFunction AddPoint   64   0.3
opacityTransferFunction AddPoint   71   0.0
opacityTransferFunction AddPoint  117   0.0
opacityTransferFunction AddPoint  124   0.4
opacityTransferFunction AddPoint  131   0.0
opacityTransferFunction AddPoint  180   0.0
opacityTransferFunction AddPoint  192   0.6
opacityTransferFunction AddPoint  210   0.0

vtkColorTransferFunction colorTransferFunction

colorTransferFunction AddRGBPoint   0 0.0 0.0 1.0
colorTransferFunction AddRGBPoint  90 0.0 0.0 1.0
colorTransferFunction AddRGBPoint  91 0.0 1.0 0.0
colorTransferFunction AddRGBPoint 160 0.0 1.0 0.0
colorTransferFunction AddRGBPoint 161 1.0 0.0 0.0
colorTransferFunction AddRGBPoint 255 1.0 0.0 0.0


vtkVolumeProperty volumeProperty
volumeProperty SetScalarOpacity opacityTransferFunction
volumeProperty SetColor colorTransferFunction
volumeProperty SetInterpolationTypeToLinear
volumeProperty ShadeOn
volumeProperty SetDiffuse 0.8
volumeProperty SetSpecular 0.4
volumeProperty SetSpecularPower 80


vtkVolumeRayCastCompositeFunction  CompositeFunction1
vtkVolumeRayCastCompositeFunction  CompositeFunction2

CompositeFunction1 SetCompositeMethodToInterpolateFirst
CompositeFunction2 SetCompositeMethodToClassifyFirst

vtkFiniteDifferenceGradientEstimator GradientEstimator

vtkVolumeRayCastMapper volumeMapper1
volumeMapper1 SetScalarInput [reader GetOutput]
volumeMapper1 SetVolumeRayCastFunction CompositeFunction1
volumeMapper1 SetGradientEstimator GradientEstimator
volumeMapper1 SetSampleDistance 0.2
volumeMapper1 SetClippingPlanes 0 49 20 49 0 49
volumeMapper1 ClippingOn

vtkVolumeRayCastMapper volumeMapper2
volumeMapper2 SetScalarInput [reader GetOutput]
volumeMapper2 SetVolumeRayCastFunction CompositeFunction2
volumeMapper2 SetGradientEstimator GradientEstimator
volumeMapper2 SetSampleDistance 0.2
volumeMapper2 SetClippingPlanes 0 49 20 49 0 49
volumeMapper2 ClippingOn

vtkVolume volume1
volume1 SetVolumeMapper volumeMapper1
volume1 SetVolumeProperty volumeProperty
ren1 AddVolume volume1

vtkVolume volume2
volume2 SetVolumeMapper volumeMapper2
volume2 SetVolumeProperty volumeProperty
ren1 AddVolume volume2
volume2 AddPosition 48 0 0

vtkSphereSource sphere
sphere SetRadius 10
sphere SetCenter 24.5 24.5 24.5
sphere SetThetaResolution 20 
sphere SetPhiResolution 20

vtkPolyDataMapper mapper
mapper SetInput [sphere GetOutput]

vtkActor actor1
actor1 SetMapper mapper
[actor1 GetProperty] SetColor .8 0 1
[actor1 GetProperty] SetDiffuse 0.7
[actor1 GetProperty] SetSpecular 0.3
[actor1 GetProperty] SetSpecularPower 80

vtkActor actor2
actor2 SetMapper mapper
[actor2 GetProperty] SetColor .8 0 1
[actor2 GetProperty] SetDiffuse 0.7
[actor2 GetProperty] SetSpecular 0.3
[actor2 GetProperty] SetSpecularPower 80
actor2 AddPosition 48 0 0

ren1 AddActor actor1
ren1 AddActor actor2

renWin SetSize 400 200
[ren1 GetActiveCamera] ParallelProjectionOn
[ren1 GetActiveCamera] SetParallelScale 500
[ren1 GetActiveCamera] Elevation -30

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

vtkLight light
light SetPosition -1 1 1
light SetFocalPoint 0 0 0
light SwitchOn
light SetIntensity 0.7
ren1 AddLight light

vtkLight light2
light2 SetPosition 1 1 1
light2 SetFocalPoint 0 0 0
light2 SwitchOn
light2 SetIntensity 0.7
ren1 AddLight light2

[ren1 GetActiveCamera] SetParallelScale 24

renWin Render
renWin SetFileName "valid/volCompareCompositeMethods.tcl.ppm"
renWin SaveImageAsPPM



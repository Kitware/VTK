# Volume rendering example with multiple lights

catch {load vtktcl}
source ../../examplesTcl/vtkInt.tcl

vtkSLCReader reader
    reader SetFileName "../../../vtkdata/sphere.slc"

vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint   80  0.0
    opacityTransferFunction AddPoint  100  1.0

vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddRGBPoint     0 1.0 1.0 1.0
    colorTransferFunction AddRGBPoint   255 1.0 1.0 1.0

vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction
    volumeProperty ShadeOn
    volumeProperty SetInterpolationTypeToLinear
    volumeProperty SetDiffuse 0.7
    volumeProperty SetAmbient 0.01
    volumeProperty SetSpecular 0.5
    volumeProperty SetSpecularPower 70.0

vtkVolumeRayCastCompositeFunction  compositeFunction

vtkVolumeRayCastMapper volumeMapper
    volumeMapper SetInput [reader GetOutput]
    volumeMapper SetVolumeRayCastFunction compositeFunction

vtkVolume volume
    volume SetMapper volumeMapper
    volume SetProperty volumeProperty

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 256 256
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkSphereSource sphere
sphere SetRadius 20
sphere SetCenter 70 25 25
sphere SetThetaResolution 50
sphere SetPhiResolution 50

vtkPolyDataMapper mapper
mapper SetInput [sphere GetOutput]

vtkActor actor
actor SetMapper mapper
[actor GetProperty] SetColor 1 1 1
[actor GetProperty] SetAmbient 0.01
[actor GetProperty] SetDiffuse 0.7
[actor GetProperty] SetSpecular 0.5
[actor GetProperty] SetSpecularPower 70.0

ren1 AddVolume volume
ren1 AddActor actor
ren1 SetBackground 0.1 0.2 0.4
[ren1 GetActiveCamera] Zoom 1.6
renWin Render

set lights [ren1 GetLights]
$lights InitTraversal
set light [$lights GetNextItem]
$light SetIntensity 0.7

vtkLight redlight 
redlight SetColor 1 0 0
redlight SetPosition 1000 25 25
redlight SetFocalPoint 25 25 25
redlight SetIntensity 0.5

vtkLight greenlight 
greenlight SetColor 0 1 0
greenlight SetPosition 25 1000 25
greenlight SetFocalPoint 25 25 25
greenlight SetIntensity 0.5

vtkLight bluelight 
bluelight SetColor 0 0 1
bluelight SetPosition 25 25 1000
bluelight SetFocalPoint 25 25 25
bluelight SetIntensity 0.5

ren1 AddLight redlight
ren1 AddLight greenlight
ren1 AddLight bluelight

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName "valid/volMultiLight.tcl.ppm"
#renWin SaveImageAsPPM

wm withdraw .


catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Volume rendering example with multiple lights
vtkSLCReader reader
    reader SetFileName "../../../vtkdata/sphere.slc"

# Create transfer functions for opacity and color
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint   80  0.0
    opacityTransferFunction AddPoint  100  1.0

vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddRGBPoint     0 1.0 1.0 1.0
    colorTransferFunction AddRGBPoint   255 1.0 1.0 1.0

# Create properties, mappers, volume actors, and ray cast function
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
    volumeMapper SetScalarInput [reader GetOutput]
    volumeMapper SetVolumeRayCastFunction compositeFunction

vtkVolume volume
    volume SetVolumeMapper volumeMapper
    volume SetVolumeProperty volumeProperty


# Okay now the graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 256 256
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddVolume volume
ren1 SetBackground 0.1 0.2 0.4
[ren1 GetActiveCamera] Azimuth 30.0
[ren1 GetActiveCamera] Elevation 20.0
[ren1 GetActiveCamera] Roll 30.0
[ren1 GetActiveCamera] Zoom 1.8
ren1 Render

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


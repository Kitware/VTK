catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Simple volume rendering example.
vtkSLCReader reader
    reader SetFileName "$VTK_DATA/poship.slc"

# Create transfer functions for opacity and color
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  20   0.0
    opacityTransferFunction AddPoint  255  0.2

vtkPiecewiseFunction colorTransferFunction
    colorTransferFunction AddPoint      0.0 0.0
    colorTransferFunction AddPoint     64.0 1.0
    colorTransferFunction AddPoint    128.0 0.0
    colorTransferFunction AddPoint    255.0 0.0

# Create properties, mappers, volume actors, and ray cast function
vtkVolumeProperty volumeProperty1
    volumeProperty1 SetColor colorTransferFunction
    volumeProperty1 SetScalarOpacity opacityTransferFunction
    volumeProperty1 SetInterpolationTypeToLinear
    volumeProperty1 ShadeOn

vtkVolumeProperty volumeProperty2
    volumeProperty2 SetColor colorTransferFunction
    volumeProperty2 SetScalarOpacity opacityTransferFunction
    volumeProperty2 SetInterpolationTypeToLinear
    volumeProperty2 ShadeOff

vtkVolumeProperty volumeProperty3
    volumeProperty3 SetColor colorTransferFunction
    volumeProperty3 SetScalarOpacity opacityTransferFunction
    volumeProperty3 SetInterpolationTypeToNearest
    volumeProperty3 ShadeOn

vtkVolumeProperty volumeProperty4
    volumeProperty4 SetColor colorTransferFunction
    volumeProperty4 SetScalarOpacity opacityTransferFunction
    volumeProperty4 SetInterpolationTypeToNearest
    volumeProperty4 ShadeOff

vtkVolumeRayCastCompositeFunction  compositeFunction

vtkVolumeRayCastMapper volumeMapper
    volumeMapper SetInput [reader GetOutput]
    volumeMapper SetVolumeRayCastFunction compositeFunction

vtkVolume volume1
    volume1 SetMapper volumeMapper
    volume1 SetProperty volumeProperty1

vtkVolume volume2
    volume2 SetMapper volumeMapper
    volume2 SetProperty volumeProperty2

vtkVolume volume3
    volume3 SetMapper volumeMapper
    volume3 SetProperty volumeProperty3

vtkVolume volume4
    volume4 SetMapper volumeMapper
    volume4 SetProperty volumeProperty4

# Create outline
vtkOutlineFilter outline
    outline SetInput [reader GetOutput]

vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]

vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor 1 1 1

# Okay now the graphics stuff
vtkRenderer ren1
    ren1 SetViewport 0 0 .5 .5
vtkRenderer ren2
    ren2 SetViewport .5 0 1.0 .5
vtkRenderer ren3
    ren3 SetViewport 0 .5 .5 1
vtkRenderer ren4
    ren4 SetViewport .5 .5 1 1

vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin AddRenderer ren2
    renWin AddRenderer ren3
    renWin AddRenderer ren4
    renWin SetSize 256 256
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor outlineActor
ren2 AddActor outlineActor
ren3 AddActor outlineActor
ren4 AddActor outlineActor

ren1 AddVolume volume1
ren2 AddVolume volume2
ren3 AddVolume volume3
ren4 AddVolume volume4

# Render the unshaded volume
renWin Render

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName "valid/volExercise.tcl.ppm"
#renWin SaveImageAsPPM

wm withdraw .


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

vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddRGBPoint      0.0 0.0 0.0 0.0
    colorTransferFunction AddRGBPoint     64.0 1.0 0.0 0.0
    colorTransferFunction AddRGBPoint    128.0 0.0 0.0 1.0
    colorTransferFunction AddRGBPoint    192.0 0.0 1.0 0.0
    colorTransferFunction AddRGBPoint    255.0 0.0 0.2 0.0

# Create property, mapper, volume 
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction

vtkVolumeTextureMapper2D volumeMapper
    volumeMapper SetInput [reader GetOutput]

vtkVolume volume
    volume SetMapper volumeMapper
    volume SetProperty volumeProperty

# Okay now the graphics stuff
vtkRenderer ren1
vtkRenderer ren2
vtkRenderer ren3
vtkRenderer ren4
vtkRenderer ren5
vtkRenderer ren6
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin AddRenderer ren2
    renWin AddRenderer ren3
    renWin AddRenderer ren4
    renWin AddRenderer ren5
    renWin AddRenderer ren6
    renWin SetSize 768 512
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddVolume volume
ren2 AddVolume volume
ren3 AddVolume volume
ren4 AddVolume volume
ren5 AddVolume volume
ren6 AddVolume volume

ren1 SetBackground 0.1 0.2 0.4
ren2 SetBackground 0.1 0.2 0.4
ren3 SetBackground 0.1 0.2 0.4
ren4 SetBackground 0.1 0.2 0.4
ren5 SetBackground 0.1 0.2 0.4
ren6 SetBackground 0.1 0.2 0.4

ren1 SetViewport 0 0 .3 .5
ren2 SetViewport .3 0 .6 .5
ren3 SetViewport .6 0 1 .5
ren4 SetViewport 0 .5 .3 1
ren5 SetViewport .3 .5 .6 1
ren6 SetViewport .6 .5 1 1

[ren1 GetActiveCamera] SetPosition 1 0 0
[ren1 GetActiveCamera] SetFocalPoint 0 0 0
[ren1 GetActiveCamera] SetViewUp 0 1 0
ren1 ResetCamera

[ren4 GetActiveCamera] SetPosition -1 0 0
[ren4 GetActiveCamera] SetFocalPoint 0 0 0
[ren4 GetActiveCamera] SetViewUp 0 1 0
ren4 ResetCamera

[ren2 GetActiveCamera] SetPosition 0 0 1
[ren2 GetActiveCamera] SetFocalPoint 0 0 0
[ren2 GetActiveCamera] SetViewUp 0 1 0
ren2 ResetCamera

[ren5 GetActiveCamera] SetPosition 0 0 -1
[ren5 GetActiveCamera] SetFocalPoint 0 0 0
[ren5 GetActiveCamera] SetViewUp 0 1 0
ren5 ResetCamera

[ren3 GetActiveCamera] SetPosition 0 1 0
[ren3 GetActiveCamera] SetFocalPoint 0 0 0
[ren3 GetActiveCamera] SetViewUp 0 0 1
ren3 ResetCamera

[ren6 GetActiveCamera] SetPosition 0 -1 0
[ren6 GetActiveCamera] SetFocalPoint 0 0 0
[ren6 GetActiveCamera] SetViewUp 0 0 1
ren6 ResetCamera

renWin Render

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName "volTexSimple.tcl.ppm"
#renWin SaveImageAsPPM

wm withdraw .

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

# Get a value of the function when no points are defined
    opacityTransferFunction GetValue 50

# Exceed the initial size of the piecewise function (64)
for {set i 0} {$i< 64} {incr i} {
    opacityTransferFunction AddPoint  $i [expr $i/128.0]
}
for {} {$i< 128} {incr i} {
    opacityTransferFunction AddPoint  $i [expr .5 - ($i/128.0)]
}

# Add a segment
    opacityTransferFunction AddSegment 128 0 256 0
    opacityTransferFunction AddSegment 256 0 128 0

# Delete first point
    opacityTransferFunction RemovePoint 0 

# Duplicate an entry
    opacityTransferFunction AddPoint 64 .55

# Add a point at the start
    opacityTransferFunction AddPoint -1 100

# Get the value of the function at a point outside the set values
    opacityTransferFunction GetValue 640

# Turn clamping off
    opacityTransferFunction ClampingOff

# Get the value of the function at a point outside the set values
    opacityTransferFunction GetValue 640

# Turn clamping back on
    opacityTransferFunction ClampingOn

# Remove all points created so far
    opacityTransferFunction RemoveAllPoints

# Create a final transfer function for volume rendering
    opacityTransferFunction AddPoint  20   0.0
    opacityTransferFunction AddPoint  255  0.2

vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddRGBPoint      0.0 0.0 0.0 0.0
    colorTransferFunction AddRGBPoint     64.0 1.0 0.0 0.0
    colorTransferFunction AddRGBPoint    128.0 0.0 0.0 1.0
    colorTransferFunction AddRGBPoint    192.0 0.0 1.0 0.0
    colorTransferFunction AddRGBPoint    255.0 0.0 0.2 0.0

# Create properties, mappers, volume actors, and ray cast function
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction

vtkVolumeRayCastCompositeFunction  compositeFunction

vtkVolumeRayCastMapper volumeMapper
    volumeMapper SetInput [reader GetOutput]
    volumeMapper SetVolumeRayCastFunction compositeFunction

vtkVolume volume
    volume SetMapper volumeMapper
    volume SetProperty volumeProperty

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
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 256 256
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

#ren1 AddActor outlineActor
ren1 AddVolume volume
ren1 SetBackground 0.1 0.2 0.4
renWin Render

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName "valid/piecewiseFunction.ppm"
#renWin SaveImageAsPPM

wm withdraw .


catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Simple volume rendering example.
vtkSLCReader reader
    reader SetFileName "../../../vtkdata/nut.slc"

vtkStructuredPointsReader rgbreader
    rgbreader SetFileName rgbdata.vtk

# Create transfer functions for opacity and color
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  100   0.0
    opacityTransferFunction AddPoint  128   1.0

vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddRGBPoint   0 1.0 1.0 1.0
    colorTransferFunction AddRGBPoint 255 1.0 1.0 1.0

# Create properties, mappers, volume actors, and ray cast function
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction
    volumeProperty ShadeOn
    volumeProperty SetInterpolationTypeToLinear

vtkVolumeRayCastIsosurfaceFunction  isoFunction
    isoFunction SetIsoValue 128.0

vtkVolumeRayCastMapper volumeMapper
    volumeMapper SetScalarInput [reader GetOutput]
    volumeMapper SetRGBTextureInput [rgbreader GetOutput]
    volumeMapper SetVolumeRayCastFunction isoFunction

vtkVolume volume
    volume SetVolumeMapper volumeMapper
    volume SetVolumeProperty volumeProperty

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
    renWin SetSize 200 200
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

#ren1 AddActor outlineActor
ren1 AddVolume volume
ren1 SetBackground 0.1 0.2 0.4
[ren1 GetActiveCamera] Elevation 30.0
[ren1 GetActiveCamera] Zoom 1.3
renWin Render

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .


for { set i 0.1 } { $i <= 0.8 } { set i [expr $i + 0.1] } {
    volumeProperty SetRGBTextureCoefficient $i
    renWin Render
}

for { set i 0 } { $i <= 12 } { incr i 2 } {
    [rgbreader GetOutput] SetOrigin $i $i $i
    renWin Render
}


#renWin SetFileName "valid/volIsoTexture.tcl.ppm"
#renWin SaveImageAsPPM


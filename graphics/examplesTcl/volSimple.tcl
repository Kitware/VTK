catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Simple volume rendering example.
vtkSLCReader reader
    reader SetFileName "../../../vtkdata/poship.slc"

# Create transfer functions for opacity and color
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  20   0.0
    opacityTransferFunction AddPoint  255  0.2

vtkColorTransferFunction colorTransferFunction
    colorTransferFunction AddRedPoint      0.0 0.0
    colorTransferFunction AddRedPoint     64.0 1.0
    colorTransferFunction AddRedPoint    128.0 0.0
    colorTransferFunction AddRedPoint    255.0 0.0
    colorTransferFunction AddBluePoint    0.0 0.0
    colorTransferFunction AddBluePoint   64.0 0.0
    colorTransferFunction AddBluePoint  128.0 1.0
    colorTransferFunction AddBluePoint  192.0 0.0
    colorTransferFunction AddBluePoint  255.0 0.0
    colorTransferFunction AddGreenPoint     0.0 0.0
    colorTransferFunction AddGreenPoint   128.0 0.0
    colorTransferFunction AddGreenPoint   192.0 1.0
    colorTransferFunction AddGreenPoint   255.0 0.2

# Create properties, mappers, volume actors, and ray cast function
vtkVolumeProperty volumeProperty
    volumeProperty SetColor colorTransferFunction
    volumeProperty SetScalarOpacity opacityTransferFunction

vtkVolumeRayCastCompositeFunction  compositeFunction

vtkVolumeRayCastMapper volumeMapper
    volumeMapper SetScalarInput [reader GetOutput]
    volumeMapper SetVolumeRayCastFunction compositeFunction

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
    ren1 SetBackground 0.1 0.2 0.4
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 256 256
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

#ren1 AddActor outlineActor
ren1 AddVolume volume
renWin Render

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#renWin SetFileName "valid/volSimple.ppm"
#renWin SaveImageAsPPM

wm withdraw .


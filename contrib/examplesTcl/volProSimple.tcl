catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Simple volume rendering example.
vtkSLCReader reader
    reader SetFileName "../../../vtkdata/poship.slc"
    reader Update

# Create transfer functions for opacity and color
vtkPiecewiseFunction opacityTransferFunction
    opacityTransferFunction AddPoint  20   0.0
    opacityTransferFunction AddPoint  255  0.5

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
    volumeProperty ShadeOff

vtkVolumeProMapper volumeMapper
    volumeMapper SetInput [reader GetOutput]

vtkVolume volume
    volume SetMapper volumeMapper
    volume SetProperty volumeProperty

# Okay now the graphics stuff
vtkRenderer ren1
ren1 SetBackground 0.1 0.2 0.4

vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetSize 256 256
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddVolume volume

[ren1 GetActiveCamera] ParallelProjectionOn
[ren1 GetActiveCamera] SetParallelScale 40

iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render

wm withdraw .


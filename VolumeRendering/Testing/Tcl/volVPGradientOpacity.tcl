package require vtk
package require vtkinteraction

vtkSLCReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/neghip.slc"

vtkImageShiftScale ss
ss SetInputConnection [reader GetOutputPort]
ss SetOutputScalarTypeToUnsignedShort
ss SetScale 256

vtkPiecewiseFunction opacity_transfer_function
opacity_transfer_function AddPoint 5120  0.0
opacity_transfer_function AddPoint 65280 0.2

vtkColorTransferFunction color_transfer_function
color_transfer_function AddRGBPoint 0     0 0 0
color_transfer_function AddRGBPoint 16384 1 0 0
color_transfer_function AddRGBPoint 32768 0 0 1
color_transfer_function AddRGBPoint 49152 0 1 0
color_transfer_function AddRGBPoint 65280 0 .2 0

vtkPiecewiseFunction gradient_opacity_function
gradient_opacity_function AddPoint 0     0
gradient_opacity_function AddPoint 3092  0
gradient_opacity_function AddPoint 6528  1
gradient_opacity_function AddPoint 65280 1

vtkVolumeProMapper mapper
mapper SetInputConnection [ss GetOutputPort]
mapper GradientOpacityModulationOn

vtkVolumeProperty volProp
volProp SetColor color_transfer_function
volProp SetScalarOpacity opacity_transfer_function
volProp SetGradientOpacity gradient_opacity_function

vtkVolume volume
volume SetMapper mapper
volume SetProperty volProp

vtkRenderer ren
ren AddViewProp volume
[ren GetActiveCamera] ParallelProjectionOn
ren SetBackground 0.1 0.2 0.4

vtkRenderWindow renWin
renWin AddRenderer ren
renWin SetSize 300 300

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin
iren AddObserver UserEvent {wm withdraw .}

renWin Render
ren ResetCamera

wm withdraw .

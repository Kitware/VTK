# A simple example of a three-dimensional noise pattern.

# first we load in the standard vtk packages into tcl
package require vtk
package require vtkinteraction
package require vtktesting

vtkPerlinNoise perlin
perlin SetFrequency 2 1.25 1.5
perlin SetPhase 0 0 0

vtkSampleFunction sample
    sample SetImplicitFunction perlin
    sample SetSampleDimensions 65 65 20
    sample ComputeNormalsOff
vtkContourFilter surface
    surface SetInputConnection [sample GetOutputPort]
    surface SetValue 0 0.0

vtkPolyDataMapper mapper
    mapper SetInputConnection [surface GetOutputPort]
    mapper ScalarVisibilityOff
vtkActor actor
    actor SetMapper mapper
    eval [actor GetProperty] SetColor 0.2 0.4 0.6

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 300 300
ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 1.35
ren1 ResetCameraClippingRange
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

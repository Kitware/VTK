# This example demonstrates how to combine a "geometric"
# implicit function with noise at different frequencies
# to produce the appearance of a landscape.

# first we load in the standard vtk packages into tcl
package require vtk
package require vtkinteraction
package require vtktesting

vtkPlane plane

vtkPerlinNoise p1
p1 SetFrequency 1 1 0

vtkPerlinNoise p2
p2 SetFrequency 3 5 0
p2 SetPhase 0.5 0.5 0

vtkPerlinNoise p3
p3 SetFrequency 16 16 0

vtkImplicitSum sum
sum SetNormalizeByWeight 1
sum AddFunction plane
sum AddFunction p1 0.2
sum AddFunction p2 0.1
sum AddFunction p3 0.02

vtkSampleFunction sample
    sample SetImplicitFunction sum
    sample SetSampleDimensions 65 65 20
    sample SetModelBounds -1 1 -1 1 -0.5 0.5
    sample ComputeNormalsOff
vtkContourFilter surface
    surface SetInputConnection [sample GetOutputPort]
    surface SetValue 0 0.0

vtkPolyDataNormals smooth
    smooth SetInputConnection [surface GetOutputPort]
    smooth SetFeatureAngle 90

vtkPolyDataMapper mapper
    mapper SetInputConnection [smooth GetOutputPort]
    mapper ScalarVisibilityOff
vtkActor actor
    actor SetMapper mapper
    eval [actor GetProperty] SetColor 0.4 0.2 0.1

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
ren1 ResetCamera
[ren1 GetActiveCamera] Elevation -45
[ren1 GetActiveCamera] Azimuth 10
[ren1 GetActiveCamera] Dolly 1.35
ren1 ResetCameraClippingRange
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
iren Start
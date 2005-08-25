# This example demonstrates adding two implicit models
# to produce an (unexpected!) result

# first we load in the standard vtk packages into tcl
package require vtk
package require vtkinteraction
package require vtktesting

vtkCone geomObject1

vtkSphere geomObject2
    geomObject2 SetRadius 0.5
    geomObject2 SetCenter 0.5 0 0

vtkImplicitSum sum
    sum SetNormalizeByWeight 1
    sum AddFunction geomObject1 2
    sum AddFunction geomObject2 1

vtkSampleFunction sample
    sample SetImplicitFunction sum
    sample SetSampleDimensions 60 60 60
    sample ComputeNormalsOn

vtkContourFilter surface
    surface SetInputConnection [sample GetOutputPort]
    surface SetValue 0 0.0

vtkPolyDataMapper mapper
    mapper SetInputConnection [surface GetOutputPort]
    mapper ScalarVisibilityOff
vtkActor actor
    actor SetMapper mapper
    eval [actor GetProperty] SetDiffuseColor 0.2 0.4 0.6
    eval [actor GetProperty] SetSpecular 0.4
    eval [actor GetProperty] SetDiffuse  0.7
    eval [actor GetProperty] SetSpecularPower 40

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
[ren1 GetActiveCamera] Azimuth 60
[ren1 GetActiveCamera] Elevation -10
[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCameraClippingRange
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render


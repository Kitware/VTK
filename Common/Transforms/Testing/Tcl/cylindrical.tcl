package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# avoid the singularity at the Z axis using 0.0001 radian offset
vtkPlaneSource plane
    plane SetOrigin 1.0 0.0           -1.0
    plane SetPoint1 1.0 6.28318530719 -1.0
    plane SetPoint2 1.0 0.0            1.0
    plane SetXResolution 19
    plane SetYResolution 9

vtkCylindricalTransform transform

vtkTransformPolyDataFilter tpoly
    tpoly SetInputConnection [plane GetOutputPort]
    tpoly SetTransform transform

vtkTransformPolyDataFilter tpoly2
    tpoly2 SetInputConnection [tpoly GetOutputPort]
    tpoly2 SetTransform [transform GetInverse]

# also cover the inverse transformation by going back and forth
vtkTransformPolyDataFilter tpoly3
    tpoly3 SetInputConnection [tpoly2 GetOutputPort]
    tpoly3 SetTransform transform

vtkDataSetMapper mapper
    mapper SetInputConnection [tpoly3 GetOutputPort]

vtkPNMReader earth
    earth SetFileName "$VTK_DATA_ROOT/Data/earth.ppm"

vtkTexture texture
    texture SetInputConnection [earth GetOutputPort]
    texture InterpolateOn

vtkActor world
    world SetMapper mapper
    world SetTexture texture

# Add the actors to the renderer, set the background and size
#
ren1 AddActor world
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300
[ren1 GetActiveCamera] SetPosition 8 -10 6
[ren1 GetActiveCamera] SetFocalPoint 0 0 0
[ren1 GetActiveCamera] SetViewAngle 15
[ren1 GetActiveCamera] SetViewUp 0.0 0.0 1.0

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
ren1 ResetCameraClippingRange

iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .



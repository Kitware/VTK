package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
vtkCamera camera
    ren1 SetActiveCamera camera

# create a sphere source and actor
#
vtkSphereSource sphere
    sphere SetThetaResolution 80
    sphere SetPhiResolution 40
    sphere SetRadius 1
    sphere SetCenter 0 0 0
vtkSphereSource sphere2
    sphere2 SetThetaResolution 80
    sphere2 SetPhiResolution 40
    sphere2 SetRadius 0.5
    sphere2 SetCenter 1 0 0
vtkSphereSource sphere3
    sphere3 SetThetaResolution 80
    sphere3 SetPhiResolution 40
    sphere3 SetRadius 0.5
    sphere3 SetCenter -1 0 0
vtkSphereSource sphere4
    sphere4 SetThetaResolution 80
    sphere4 SetPhiResolution 40
    sphere4 SetRadius 0.5
    sphere4 SetCenter 0 1 0
vtkSphereSource sphere5
    sphere5 SetThetaResolution 80
    sphere5 SetPhiResolution 40
    sphere5 SetRadius 0.5
    sphere5 SetCenter 0 -1 0
vtkAppendPolyData appendData
    appendData AddInput [sphere GetOutput]
    appendData AddInput [sphere2 GetOutput]
    appendData AddInput [sphere3 GetOutput]
    appendData AddInput [sphere4 GetOutput]
    appendData AddInput [sphere5 GetOutput]

vtkDepthSortPolyData depthSort
    depthSort SetInputConnection [appendData GetOutputPort]
    depthSort SetDirectionToBackToFront
    depthSort SetVector 1 1 1
    depthSort SetCamera camera
    depthSort SortScalarsOn
    depthSort Update

vtkPolyDataMapper mapper
    mapper SetInputConnection [depthSort GetOutputPort]
    mapper SetScalarRange 0 [[depthSort GetOutput] GetNumberOfCells]
vtkActor actor
    actor SetMapper mapper
    [actor GetProperty] SetOpacity 0.5
    [actor GetProperty] SetColor 1 0 0
    actor RotateX -72

depthSort SetProp3D actor

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 300 200

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 2.2
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


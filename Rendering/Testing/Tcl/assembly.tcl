package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create four parts: a top level assembly and three primitives
#
vtkSphereSource sphere
vtkPolyDataMapper sphereMapper
    sphereMapper SetInputConnection [sphere GetOutputPort]
vtkActor sphereActor
    sphereActor SetMapper sphereMapper
    sphereActor SetOrigin 2 1 3
    sphereActor RotateY 6
    sphereActor SetPosition 2.25 0 0
    [sphereActor GetProperty] SetColor 1 0 1

vtkCubeSource cube
vtkPolyDataMapper cubeMapper
    cubeMapper SetInputConnection [cube GetOutputPort]
vtkActor cubeActor
    cubeActor SetMapper cubeMapper
    cubeActor SetPosition 0.0 .25 0
    [cubeActor GetProperty] SetColor 0 0 1

vtkConeSource cone
vtkPolyDataMapper coneMapper
    coneMapper SetInputConnection [cone GetOutputPort]
vtkActor coneActor
    coneActor SetMapper coneMapper
    coneActor SetPosition 0 0 .25
    [coneActor GetProperty] SetColor 0 1 0

vtkCylinderSource cylinder;#top part
vtkPolyDataMapper cylinderMapper
    cylinderMapper SetInputConnection [cylinder GetOutputPort]
    cylinderMapper SetResolveCoincidentTopologyToPolygonOffset
vtkActor cylinderActor
    cylinderActor SetMapper cylinderMapper
    [cylinderActor GetProperty] SetColor 1 0 0

vtkAssembly assembly
    assembly AddPart cylinderActor
    assembly AddPart sphereActor
    assembly AddPart cubeActor
    assembly AddPart coneActor
    assembly SetOrigin 5 10 15
    assembly AddPosition 5 0 0
    assembly RotateX 15

# Add the actors to the renderer, set the background and size
#
ren1 AddActor assembly
ren1 AddActor coneActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 200 200

# Get handles to some useful objects
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
vtkCamera camera
    camera SetClippingRange 21.9464 30.0179
    camera SetFocalPoint 3.49221 2.28844 -0.970866
    camera SetPosition 3.49221 2.28844 24.5216
    camera SetViewAngle 30
    camera SetViewUp 0 1 0
ren1 SetActiveCamera camera
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



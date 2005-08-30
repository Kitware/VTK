package require vtk
package require vtkinteraction

# demonstrates the use of vtkPropAssembly

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
vtkActor cylinderActor
    cylinderActor SetMapper cylinderMapper
    [cylinderActor GetProperty] SetColor 1 0 0
vtkAssembly compositeAssembly
    compositeAssembly AddPart cylinderActor
    compositeAssembly AddPart sphereActor
    compositeAssembly AddPart cubeActor
    compositeAssembly AddPart coneActor
    compositeAssembly SetOrigin 5 10 15
    compositeAssembly AddPosition 5 0 0
    compositeAssembly RotateX 15

# Build the prop assembly out of a vtkActor and a vtkAssembly
vtkPropAssembly assembly
  assembly AddPart compositeAssembly
  assembly AddPart coneActor

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddViewProp assembly
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# Get handles to some useful objects
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
renWin Render

# should create the same image as assembly.tcl

# prevent the tk window from showing up then start the event loop
wm withdraw .

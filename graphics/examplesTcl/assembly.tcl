catch {load vtktcl}
# this demonstrates use of assemblies
# include get the vtk interactor ui
source vtkInt.tcl

# Create the render master
#
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# create four parts: a top level assembly and three primitives
#
vtkSphereSource sphere
vtkPolyMapper sphereMapper
    sphereMapper SetInput [sphere GetOutput]
vtkActor sphereActor
    sphereActor SetMapper sphereMapper
    sphereActor SetOrigin 2 1 3
    sphereActor RotateY 6
    sphereActor SetPosition 2.25 0 0
    [sphereActor GetProperty] SetColor 1 0 1

vtkCubeSource cube
vtkPolyMapper cubeMapper
    cubeMapper SetInput [cube GetOutput]
vtkActor cubeActor
    cubeActor SetMapper cubeMapper
    cubeActor SetPosition 0.0 .25 0
    [cubeActor GetProperty] SetColor 0 0 1

vtkConeSource cone
vtkPolyMapper coneMapper
    coneMapper SetInput [cone GetOutput]
vtkActor coneActor
    coneActor SetMapper coneMapper
    coneActor SetPosition 0 0 .25
    [coneActor GetProperty] SetColor 0 1 0

vtkCylinderSource cylinder;#top part
vtkPolyMapper cylinderMapper
    cylinderMapper SetInput [cylinder GetOutput]
vtkAssembly cylinderActor
    cylinderActor SetMapper cylinderMapper
    cylinderActor AddPart sphereActor
    cylinderActor AddPart cubeActor
    cylinderActor AddPart coneActor
    cylinderActor SetOrigin 5 10 15
    cylinderActor AddPosition 5 0 0
    cylinderActor RotateX 15
    [cylinderActor GetProperty] SetColor 1 0 0

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors cylinderActor
$ren1 AddActors coneActor
$ren1 SetBackground 0.1 0.2 0.4
$renWin SetSize 450 450

# Get handles to some useful objects
#
$iren SetUserMethod {wm deiconify .vtkInteract}
$iren Initialize
$renWin Render

#$renWin SetFileName assembly.tcl.ppm
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



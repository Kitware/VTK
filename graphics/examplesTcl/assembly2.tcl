catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# this demonstrates assemblies hierarchies
# include get the vtk interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkSphereSource sphere
vtkPolyDataMapper sphereMapper
    sphereMapper SetInput [sphere GetOutput]
vtkActor sphereActor
    sphereActor SetMapper sphereMapper
    sphereActor SetOrigin 2 1 3
    sphereActor RotateY 6
    sphereActor SetPosition 2.25 0 0
    [sphereActor GetProperty] SetColor 1 1 0

vtkCubeSource cube
vtkPolyDataMapper cubeMapper
    cubeMapper SetInput [cube GetOutput]
vtkActor cubeActor
    cubeActor SetMapper cubeMapper
    cubeActor SetPosition 0.0 .25 0
    [cubeActor GetProperty] SetColor 0 1 1

vtkConeSource cone
vtkPolyDataMapper coneMapper
    coneMapper SetInput [cone GetOutput]
vtkActor coneActor
    coneActor SetMapper coneMapper
    coneActor SetPosition 0 0 .25
    [coneActor GetProperty] SetColor 1 0 1

vtkCylinderSource cylinder;#top part
vtkPolyDataMapper cylinderMapper
    cylinderMapper SetInput [cylinder GetOutput]
vtkActor cylActor
    cylActor SetMapper cylinderMapper
    [cylActor GetProperty] SetColor 1 0 0
vtkAssembly cylinderActor
    cylinderActor AddPart cylActor
    cylinderActor AddPart sphereActor
    cylinderActor AddPart cubeActor
    cylinderActor AddPart coneActor
    cylinderActor SetOrigin 5 10 15
    cylinderActor AddPosition 5 0 0
    cylinderActor RotateX 45

vtkActor cylActor2
    cylActor2 SetMapper cylinderMapper
    [cylActor2 GetProperty] SetColor 0 1 0
vtkAssembly cylinderActor2
    cylinderActor2 AddPart cylActor2
    cylinderActor2 AddPart sphereActor
    cylinderActor2 AddPart cubeActor
    cylinderActor2 AddPart coneActor
    cylinderActor2 SetOrigin 5 10 15
    cylinderActor2 AddPosition 6 0 0
    cylinderActor2 RotateX 50

vtkAssembly twoGroups
    twoGroups AddPart cylinderActor
    twoGroups AddPart cylinderActor2
    twoGroups AddPosition 0 0 2
    twoGroups RotateX 15
    
vtkAssembly twoGroups2
    twoGroups2 AddPart cylinderActor
    twoGroups2 AddPart cylinderActor2
    twoGroups2 AddPosition 3 0 0

vtkAssembly twoGroups3
    twoGroups3 AddPart cylinderActor
    twoGroups3 AddPart cylinderActor2
    twoGroups3 AddPosition 0 4 0
    twoGroups3 VisibilityOff

vtkAssembly threeGroups
    threeGroups AddPart twoGroups
    threeGroups AddPart twoGroups2
    threeGroups AddPart twoGroups3

vtkAssembly threeGroups2
    threeGroups2 AddPart twoGroups
    threeGroups2 AddPart twoGroups2
    threeGroups2 AddPart twoGroups3
    threeGroups2 AddPosition 5 5 5

vtkAssembly topLevel
    topLevel AddPart threeGroups
    topLevel AddPart threeGroups2

# Add the actors to the renderer, set the background and size
#
ren1 AddActor threeGroups
ren1 AddActor threeGroups2
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 450 450

# Get handles to some useful objects
#
iren SetUserMethod {wm deiconify .vtkInteract}
vtkCamera camera
    camera SetClippingRange 29.8577 59.9358
    camera SetFocalPoint 10.4922 18.7858 3.11418
    camera SetPosition 10.4922 18.7858 46.2418
    camera SetViewAngle 30
    camera SetViewUp 0 1 0
#ren1 SetActiveCamera camera
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

#renWin SetFileName assembly2.tcl.ppm
#renWin SaveImageAsPPM


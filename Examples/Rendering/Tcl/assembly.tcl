# This example demonstrates the use of vtkAssembly.  In an assembly, the motion
# of one actor affects the position of other actors.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

# Create four parts: a top level assembly (in this case, a vtkCylinder)
# and three primitives (using vtkSphereSource, vtkCubeSource, and
# vtkConeSource).  Set up mappers and actors for each part of the assembly to
# carry information about material properties and associated geometry.
#
vtkSphereSource sphere
vtkPolyDataMapper sphereMapper
    sphereMapper SetInput [sphere GetOutput]
vtkActor sphereActor
    sphereActor SetMapper sphereMapper
    sphereActor SetOrigin 2 1 3
    sphereActor RotateY 6
    sphereActor SetPosition 2.25 0 0
    [sphereActor GetProperty] SetColor 1 0 1

vtkCubeSource cube
vtkPolyDataMapper cubeMapper
    cubeMapper SetInput [cube GetOutput]
vtkActor cubeActor
    cubeActor SetMapper cubeMapper
    cubeActor SetPosition 0.0 .25 0
    [cubeActor GetProperty] SetColor 0 0 1

vtkConeSource cone
vtkPolyDataMapper coneMapper
    coneMapper SetInput [cone GetOutput]
vtkActor coneActor
    coneActor SetMapper coneMapper
    coneActor SetPosition 0 0 .25
    [coneActor GetProperty] SetColor 0 1 0

# top part of the assembly
vtkCylinderSource cylinder;
vtkPolyDataMapper cylinderMapper
    cylinderMapper SetInput [cylinder GetOutput]
    cylinderMapper SetResolveCoincidentTopologyToPolygonOffset
vtkActor cylinderActor
    cylinderActor SetMapper cylinderMapper
    [cylinderActor GetProperty] SetColor 1 0 0

# Create the assembly and add the 4 parts to it.  Also set the origin, position
# and orientation in space.
vtkAssembly assembly
    assembly AddPart cylinderActor
    assembly AddPart sphereActor
    assembly AddPart cubeActor
    assembly AddPart coneActor
    assembly SetOrigin 5 10 15
    assembly AddPosition 5 0 0
    assembly RotateX 15

# Create the Renderer, RenderWindow, and RenderWindowInteractor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor assembly
ren1 AddActor coneActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 200 200

# Set up the camera to get a particular view of the scene
vtkCamera camera
    camera SetClippingRange 21.9464 30.0179
    camera SetFocalPoint 3.49221 2.28844 -0.970866
    camera SetPosition 3.49221 2.28844 24.5216
    camera SetViewAngle 30
    camera SetViewUp 0 1 0
ren1 SetActiveCamera camera

# Set the user method (bound to key 'u')
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# Withdraw the default tk window
wm withdraw .



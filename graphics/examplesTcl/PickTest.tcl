catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Demonstrate how to use picking on vtkActor2D and on vtkAssembly.

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# A text string rendered as a vtkActor2D
vtkTextMapper textMapper
    textMapper SetFontFamilyToArial
    textMapper SetFontSize 18
    textMapper BoldOn
    textMapper SetInput "Any Old String"
vtkActor2D textActor
    textActor VisibilityOn
    textActor SetMapper textMapper
    [textActor GetPositionCoordinate] SetCoordinateSystemToNormalizedDisplay
    [textActor GetPositionCoordinate] SetValue .1 .25
    [textActor GetProperty] SetColor 0 1 0
vtkActor2D textActor2
    textActor2 VisibilityOn
    textActor2 SetMapper textMapper
    [textActor2 GetPositionCoordinate] SetCoordinateSystemToNormalizedDisplay
    [textActor2 GetPositionCoordinate] SetValue .1 .5
    [textActor2 GetProperty] SetColor 0 1 0
vtkActor2D textActor3
    textActor3 VisibilityOn
    textActor3 SetMapper textMapper
    [textActor3 GetPositionCoordinate] SetCoordinateSystemToNormalizedDisplay
    [textActor3 GetPositionCoordinate] SetValue .1 .75
    [textActor3 GetProperty] SetColor 0 1 0

#Create some 3D stuff
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

vtkCylinderSource cylinder;#top part
vtkPolyDataMapper cylinderMapper
    cylinderMapper SetInput [cylinder GetOutput]
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

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor2D textActor
ren1 AddActor2D textActor2
ren1 AddActor2D textActor3
ren1 AddActor2D assembly
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 400 200

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

vtkCamera aCamera
aCamera SetClippingRange 2.50592 10.0
aCamera SetFocalPoint 2.82901 2.94624 -1.98717
aCamera SetPosition 9.8563 7.7298 1.39316
aCamera SetViewAngle 30
aCamera SetViewUp -0.40512 0.843822 -0.351913
ren1 SetActiveCamera aCamera
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

# Manually pick the prop (vtkActor2D) and highlight it. Note that picking
# returns a "pick path" which reflects a hierarchical list of (possibly
# transformed) props. The list is made up of vtkAssemblyNode's. If the path
# has more than node, then the first node would be an assembly, and the last
# node one of the leafs of the assembly.  In the first pick the path is just
# one vtkAssemblyNode long, since we aren't pickign an assembly.
[iren GetPicker] Pick 100 100 0 ren1
[iren GetInteractorStyle] HighlightProp \
      [[[[iren GetPicker] GetPath] GetFirstNode] GetProp]

# Here we pick an assembly. Since we are invoking the "HighlightProp"
# method by hand, we have to make sure that some internal varables
# are set (e.g., CurrentRenderer). We use the FindPokedRenderer method.
[iren GetPicker] Pick 350 100 0 ren1
[iren GetInteractorStyle] FindPokedRenderer 350 100
[iren GetInteractorStyle] HighlightProp \
      [[[[iren GetPicker] GetPath] GetFirstNode] GetProp]

# Final note: in the previous pick if we were to GetLastNode(),
# we would get a leaf of the assembly. To highlight it with a
# bounding box, we'd have to also do GetMatrix() and use this
# matrix to position the bounding box. Otherwise, the bounding
# box is going to end up in the wrong spot. NOTE: there is a 
# trick here. The initial bounds of the sphere source are
# known to be (-0.5,0.5,-0.5,0.5,-0.5,0.5). This is set in the
# vtkOutlineSource. Later on the SetUserMatrix call uses the
# matrix obtained from GetLastNode()->GetMatrix() to actually
# transform the outline of the picked prop.
set prop3d [[[[iren GetPicker] GetPath] GetLastNode] GetProp]
set matrix [[[[iren GetPicker] GetPath] GetLastNode] GetMatrix]
vtkOutlineSource os
    eval os SetBounds -0.5 0.5 -0.5 0.5 -0.5 0.5
vtkPolyDataMapper osMapper
   osMapper SetInput [os GetOutput]
vtkActor a
   a SetMapper osMapper
   [a GetProperty] SetColor 1 0 0
   a SetUserMatrix $matrix
ren1 AddActor a
renWin Render      


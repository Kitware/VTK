# This example shows how to align a set of objects together using the Procrustes algorithm.
#
# We make three ellipsoids by distorting and translating a sphere and then align them together,
# using the different modes of Procrustes alignment: rigid-body, similarity and affine.
#
package require vtk
package require vtkinteraction

vtkSphereSource sphere

# make two copies of the shape and distort them a little

vtkTransform transform1
    transform1 Translate 0.2 0.1 0.3
    transform1 Scale 1.3 1.1 0.8

vtkTransform transform2
    transform2 Translate 0.3 0.7 0.1 
    transform2 Scale 1.0 0.1 1.8

vtkTransformPolyDataFilter transformer1
    transformer1 SetInputConnection [sphere GetOutputPort]
    transformer1 SetTransform transform1

vtkTransformPolyDataFilter transformer2
    transformer2 SetInputConnection [sphere GetOutputPort]
    transformer2 SetTransform transform2

# map these three shapes into the first renderer
vtkPolyDataMapper map1a
    map1a SetInputConnection [sphere GetOutputPort]
vtkActor Actor1a
    Actor1a SetMapper map1a
    [Actor1a GetProperty] SetDiffuseColor 1.0000 0.3882 0.2784

vtkPolyDataMapper map1b
    map1b SetInputConnection [transformer1 GetOutputPort]
vtkActor Actor1b
    Actor1b SetMapper map1b
    [Actor1b GetProperty] SetDiffuseColor 0.3882 1.0000 0.2784

vtkPolyDataMapper map1c
    map1c SetInputConnection [transformer2 GetOutputPort]
vtkActor Actor1c
    Actor1c SetMapper map1c
    [Actor1c GetProperty] SetDiffuseColor 0.3882 0.2784 1.0000 

# -- align the shapes using Procrustes (using SetModeToRigidBody) --
vtkProcrustesAlignmentFilter procrustes1
    procrustes1 SetNumberOfInputs 3
    procrustes1 SetInput 0 [sphere GetOutput]
    procrustes1 SetInput 1 [transformer1 GetOutput]
    procrustes1 SetInput 2 [transformer2 GetOutput]
    [procrustes1 GetLandmarkTransform] SetModeToRigidBody

# map the aligned shapes into the second renderer
vtkPolyDataMapper map2a
    map2a SetInputConnection [procrustes1 GetOutputPort 0]
vtkActor Actor2a
    Actor2a SetMapper map2a
    [Actor2a GetProperty] SetDiffuseColor 1.0000 0.3882 0.2784

vtkPolyDataMapper map2b
    map2b SetInputConnection [procrustes1 GetOutputPort 1]
vtkActor Actor2b
    Actor2b SetMapper map2b
    [Actor2b GetProperty] SetDiffuseColor 0.3882 1.0000 0.2784

vtkPolyDataMapper map2c
    map2c SetInputConnection [procrustes1 GetOutputPort 2]
vtkActor Actor2c
    Actor2c SetMapper map2c
    [Actor2c GetProperty] SetDiffuseColor 0.3882 0.2784 1.0000

# -- align the shapes using Procrustes (using SetModeToSimilarity (default)) --
vtkProcrustesAlignmentFilter procrustes2
    procrustes2 SetNumberOfInputs 3
    procrustes2 SetInput 0 [sphere GetOutput]
    procrustes2 SetInput 1 [transformer1 GetOutput]
    procrustes2 SetInput 2 [transformer2 GetOutput]

# map the aligned shapes into the third renderer
vtkPolyDataMapper map3a
    map3a SetInputConnection [procrustes2 GetOutputPort 0]
vtkActor Actor3a
    Actor3a SetMapper map3a
    [Actor3a GetProperty] SetDiffuseColor 1.0000 0.3882 0.2784

vtkPolyDataMapper map3b
    map3b SetInputConnection [procrustes2 GetOutputPort 1]
vtkActor Actor3b
    Actor3b SetMapper map3b
    [Actor3b GetProperty] SetDiffuseColor 0.3882 1.0000 0.2784

vtkPolyDataMapper map3c
    map3c SetInputConnection [procrustes2 GetOutputPort 2]
vtkActor Actor3c
    Actor3c SetMapper map3c
    [Actor3c GetProperty] SetDiffuseColor 0.3882 0.2784 1.0000

# -- align the shapes using Procrustes (using SetModeToAffine) --
vtkProcrustesAlignmentFilter procrustes3
    procrustes3 SetNumberOfInputs 3
    procrustes3 SetInput 0 [sphere GetOutput]
    procrustes3 SetInput 1 [transformer1 GetOutput]
    procrustes3 SetInput 2 [transformer2 GetOutput]
    [procrustes3 GetLandmarkTransform] SetModeToAffine

# map the aligned shapes into the fourth renderer
vtkPolyDataMapper map4a
    map4a SetInputConnection [procrustes3 GetOutputPort 0]
vtkActor Actor4a
    Actor4a SetMapper map4a
    [Actor4a GetProperty] SetDiffuseColor 1.0000 0.3882 0.2784

vtkPolyDataMapper map4b
    map4b SetInputConnection [procrustes3 GetOutputPort 1]
vtkActor Actor4b
    Actor4b SetMapper map4b
    [Actor4b GetProperty] SetDiffuseColor 0.3882 1.0000 0.2784

vtkPolyDataMapper map4c
    map4c SetInputConnection [procrustes3 GetOutputPort 2]
vtkActor Actor4c
    Actor4c SetMapper map4c
    [Actor4c GetProperty] SetDiffuseColor 0.3882 0.2784 1.0000

# Create the RenderWindow and its four Renderers

vtkRenderer ren1
vtkRenderer ren2
vtkRenderer ren3
vtkRenderer ren4
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin AddRenderer ren2
    renWin AddRenderer ren3
    renWin AddRenderer ren4
    renWin SetSize 400 100
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer

ren1 AddActor Actor1a
ren1 AddActor Actor1b
ren1 AddActor Actor1c

ren2 AddActor Actor2a
ren2 AddActor Actor2b
ren2 AddActor Actor2c

ren3 AddActor Actor3a
ren3 AddActor Actor3b
ren3 AddActor Actor3c

ren4 AddActor Actor4a
ren4 AddActor Actor4b
ren4 AddActor Actor4c

# set the properties of the renderers

ren1 SetBackground 1 1 1
ren1 SetViewport 0.0 0.0 0.25 1.0
[ren1 GetActiveCamera] SetPosition 1 -1 0
ren1 ResetCamera

ren2 SetBackground 1 1 1
ren2 SetViewport 0.25 0.0 0.5 1.0
[ren2 GetActiveCamera] SetPosition 1 -1 0   
ren2 ResetCamera

ren3 SetBackground 1 1 1
ren3 SetViewport 0.5 0.0 0.75 1.0
[ren3 GetActiveCamera] SetPosition 1 -1 0
ren3 ResetCamera

ren4 SetBackground 1 1 1
ren4 SetViewport 0.75 0.0 1.0 1.0
[ren4 GetActiveCamera] SetPosition 1 -1 0
ren4 ResetCamera

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



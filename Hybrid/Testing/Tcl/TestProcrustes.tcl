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
    map2a SetInput [procrustes1 GetOutput 0]
vtkActor Actor2a
    Actor2a SetMapper map2a
    [Actor2a GetProperty] SetDiffuseColor 1.0000 0.3882 0.2784

vtkPolyDataMapper map2b
    map2b SetInput [procrustes1 GetOutput 1]
vtkActor Actor2b
    Actor2b SetMapper map2b
    [Actor2b GetProperty] SetDiffuseColor 0.3882 1.0000 0.2784

vtkPolyDataMapper map2c
    map2c SetInput [procrustes1 GetOutput 2]
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
    map3a SetInput [procrustes2 GetOutput 0]
vtkActor Actor3a
    Actor3a SetMapper map3a
    [Actor3a GetProperty] SetDiffuseColor 1.0000 0.3882 0.2784

vtkPolyDataMapper map3b
    map3b SetInput [procrustes2 GetOutput 1]
vtkActor Actor3b
    Actor3b SetMapper map3b
    [Actor3b GetProperty] SetDiffuseColor 0.3882 1.0000 0.2784

vtkPolyDataMapper map3c
    map3c SetInput [procrustes2 GetOutput 2]
vtkActor Actor3c
    Actor3c SetMapper map3c
    [Actor3c GetProperty] SetDiffuseColor 0.3882 0.2784 1.0000

# Create the RenderWindow and its three Renderers

vtkRenderer ren1
vtkRenderer ren2
vtkRenderer ren3
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin AddRenderer ren2
    renWin AddRenderer ren3
    renWin SetSize 300 100
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

# set the properties of the renderers

ren1 SetBackground 1 1 1
ren1 SetViewport 0.0 0.0 0.33 1.0
ren1 ResetCamera
[ren1 GetActiveCamera] SetPosition 1 -1 0
ren1 ResetCamera

ren2 SetBackground 1 1 1
ren2 SetViewport 0.33 0.0 0.66 1.0
ren2 ResetCamera
[ren2 GetActiveCamera] SetPosition 1 -1 0   
ren2 ResetCamera

ren3 SetBackground 1 1 1
ren3 SetViewport 0.66 0.0 1.0 1.0
ren3 ResetCamera
[ren3 GetActiveCamera] SetPosition 1 -1 0
ren3 ResetCamera

renWin Render

catch {
    iren AddObserver UserEvent {wm deiconify .vtkInteract}
}

wm withdraw .
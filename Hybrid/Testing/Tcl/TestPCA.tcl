# This example shows how to visualise the variation in shape in a set of objects using
# vtkPCAAnalysisFilter.
#
# We make three ellipsoids by distorting and translating a sphere and then align them together
# using vtkProcrustesAlignmentFilter, and then pass the output to vtkPCAAnalysisFilter. We visualise
# the first and second modes - the major sources of variation that were in the training set.

package require vtk
package require vtkinteraction

vtkSphereSource sphere
  sphere SetPhiResolution 36
  sphere SetThetaResolution 36

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

#------------------------------------------------------------------
# map these three shapes into the first renderer
#------------------------------------------------------------------
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

#------------------------------------------------------------------
# align the shapes using Procrustes (using SetModeToRigidBody)
# and map the aligned shapes into the second renderer
#------------------------------------------------------------------
vtkProcrustesAlignmentFilter procrustes
    procrustes SetNumberOfInputs 3
    procrustes SetInput 0 [sphere GetOutput]
    procrustes SetInput 1 [transformer1 GetOutput]
    procrustes SetInput 2 [transformer2 GetOutput]
    [procrustes GetLandmarkTransform] SetModeToRigidBody

vtkPolyDataMapper map2a
    map2a SetInput [procrustes GetOutput 0]
vtkActor Actor2a
    Actor2a SetMapper map2a
    [Actor2a GetProperty] SetDiffuseColor 1.0000 0.3882 0.2784

vtkPolyDataMapper map2b
    map2b SetInput [procrustes GetOutput 1]
vtkActor Actor2b
    Actor2b SetMapper map2b
    [Actor2b GetProperty] SetDiffuseColor 0.3882 1.0000 0.2784

vtkPolyDataMapper map2c
    map2c SetInput [procrustes GetOutput 2]
vtkActor Actor2c
    Actor2c SetMapper map2c
    [Actor2c GetProperty] SetDiffuseColor 0.3882 0.2784 1.0000

#------------------------------------------------------------------
# pass the output of Procrustes to vtkPCAAnalysisFilter
#------------------------------------------------------------------
vtkPCAAnalysisFilter pca
    pca SetNumberOfInputs 3
    pca SetInput 0 [procrustes GetOutput 0]
    pca SetInput 1 [procrustes GetOutput 1]
    pca SetInput 2 [procrustes GetOutput 2]

pca Update
# we need to call Update because GetParameterisedShape is not
# part of the normal SetInput/GetOutput pipeline

#------------------------------------------------------------------
# map the first mode into the third renderer:
# -3,0,3 standard deviations on the first mode
# illustrate the extremes around the average shape
#------------------------------------------------------------------
vtkFloatArray params
    params SetNumberOfComponents 1
    params SetNumberOfTuples 1
    params SetTuple1 0 0.0
vtkPolyData shapea
  shapea DeepCopy [sphere GetOutput]
pca GetParameterisedShape params shapea
vtkPolyDataNormals normalsa
  normalsa SetInput shapea
vtkPolyDataMapper map3a
    map3a SetInputConnection [normalsa GetOutputPort]
vtkActor Actor3a
    Actor3a SetMapper map3a
    [Actor3a GetProperty] SetDiffuseColor 1 1 1

   params SetTuple1 0 -3.0
vtkPolyData shapeb
  shapeb DeepCopy [sphere GetOutput]
pca GetParameterisedShape params shapeb
vtkPolyDataNormals normalsb
  normalsb SetInput shapeb
vtkPolyDataMapper map3b
    map3b SetInputConnection [normalsb GetOutputPort]
vtkActor Actor3b
    Actor3b SetMapper map3b
    [Actor3b GetProperty] SetDiffuseColor 1 1 1

   params SetTuple1 0 3.0
vtkPolyData shapec
  shapec DeepCopy [sphere GetOutput]
pca GetParameterisedShape params shapec
vtkPolyDataNormals normalsc
  normalsc SetInput shapec
vtkPolyDataMapper map3c
    map3c SetInputConnection [normalsc GetOutputPort]
vtkActor Actor3c
    Actor3c SetMapper map3c
    [Actor3c GetProperty] SetDiffuseColor 1 1 1

#------------------------------------------------------------------
# map the second mode into the fourth renderer:
#------------------------------------------------------------------
vtkFloatArray params4
    params4 SetNumberOfComponents 1
    params4 SetNumberOfTuples 2
    params4 SetTuple1 0 0.0
    params4 SetTuple1 1 -3.0
vtkPolyData shape4a
  shape4a DeepCopy [sphere GetOutput]
pca GetParameterisedShape params4 shape4a
vtkPolyDataNormals normals4a
  normals4a SetInput shape4a
vtkPolyDataMapper map4a
    map4a SetInputConnection [normals4a GetOutputPort]
vtkActor Actor4a
    Actor4a SetMapper map4a
    [Actor4a GetProperty] SetDiffuseColor 1 1 1

   params4 SetTuple1 1 0.0
vtkPolyData shape4b
  shape4b DeepCopy [sphere GetOutput]
pca GetParameterisedShape params4 shape4b
vtkPolyDataNormals normals4b
  normals4b SetInput shape4b
vtkPolyDataMapper map4b
    map4b SetInputConnection [normals4b GetOutputPort]
vtkActor Actor4b
    Actor4b SetMapper map4b
    [Actor4b GetProperty] SetDiffuseColor 1 1 1

   params4 SetTuple1 1 3.0
vtkPolyData shape4c
  shape4c DeepCopy [sphere GetOutput]
pca GetParameterisedShape params4 shape4c
vtkPolyDataNormals normals4c
  normals4c SetInput shape4c
vtkPolyDataMapper map4c
    map4c SetInputConnection [normals4c GetOutputPort]
vtkActor Actor4c
    Actor4c SetMapper map4c
    [Actor4c GetProperty] SetDiffuseColor 1 1 1

#------------------------------------------------------------------
# Create the RenderWindow and its four Renderers
#------------------------------------------------------------------

vtkRenderer ren1
vtkRenderer ren2
vtkRenderer ren3
vtkRenderer ren4
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin AddRenderer ren2
    renWin AddRenderer ren3
    renWin AddRenderer ren4
    renWin SetSize 600 200
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
ren1 ResetCamera
[ren1 GetActiveCamera] SetPosition 1 -1 0
ren1 ResetCamera

ren2 SetBackground 1 1 1
ren2 SetViewport 0.25 0.0 0.5 1.0
ren2 ResetCamera
[ren2 GetActiveCamera] SetPosition 1 -1 0   
ren2 ResetCamera

ren3 SetBackground 1 1 1
ren3 SetViewport 0.5 0.0 0.75 1.0
ren3 ResetCamera
[ren3 GetActiveCamera] SetPosition 1 -1 0
ren3 ResetCamera

ren4 SetBackground 1 1 1
ren4 SetViewport 0.75 0.0 1.0 1.0
ren4 ResetCamera
[ren4 GetActiveCamera] SetPosition 1 -1 0
ren4 ResetCamera

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# output the image to file (used to generate the initial regression image)
#vtkWindowToImageFilter to_image
#to_image SetInput renWin
#vtkPNGWriter to_png
#to_png SetFileName "TestPCA.png"
#to_png SetInputConnection [to_image GetOutputPort]
#to_png Write

# prevent the tk window from showing up then start the event loop
wm withdraw .



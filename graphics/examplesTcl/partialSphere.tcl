catch {load vtktcl}
# Manipulate/test vtkSphereSource
#
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# create pipeline
vtkSphereSource sphere
    sphere SetRadius 1
vtkSphereSource sphere2
    sphere2 SetCenter 2.5 0 0
    sphere2 SetRadius 1
    sphere2 SetStartTheta 90
    sphere2 SetEndTheta 270
vtkSphereSource sphere3
    sphere3 SetCenter 0 2.5 0
    sphere3 SetRadius 1
    sphere3 SetStartPhi 90
    sphere3 SetEndPhi 135
vtkSphereSource sphere4
    sphere4 SetCenter 2.5 2.5 0
    sphere4 SetRadius 1
    sphere4 SetEndTheta 180
    sphere4 SetStartPhi 90
    sphere4 SetEndPhi 135
vtkAppendPolyData appendSpheres
    appendSpheres AddInput [sphere GetOutput]
    appendSpheres AddInput [sphere2 GetOutput]
    appendSpheres AddInput [sphere3 GetOutput]
    appendSpheres AddInput [sphere4 GetOutput]
   
vtkPolyDataMapper sphereMapper
    sphereMapper SetInput [appendSpheres GetOutput]
    sphereMapper ScalarVisibilityOff
vtkActor sphereActor
    sphereActor SetMapper sphereMapper
    eval [sphereActor GetProperty] SetColor $peacock

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
iren Initialize
renWin Render
[ren1 GetActiveCamera] Zoom 1.5

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName "sphere.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



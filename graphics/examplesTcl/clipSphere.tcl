catch {load vtktcl}
#
# Demonstrate the use of clipping on polygonal data
#
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# create pipeline
#
vtkSphereSource sphere
    sphere SetRadius 1
    sphere SetPhiResolution 25
    sphere SetThetaResolution 25
vtkPlane plane
    plane SetOrigin 0.25 0 0
    plane SetNormal -1 -1 0
vtkClipPolyData clipper
    clipper SetInput [sphere GetOutput]
    clipper SetClipFunction plane
    clipper GenerateClipScalarsOn
    clipper SetValue 0.0
vtkPolyDataMapper clipMapper
    clipMapper SetInput [clipper GetOutput]
    clipMapper ScalarVisibilityOff
vtkProperty backProp
    eval backProp SetDiffuseColor $tomato
vtkActor clipActor
    clipActor SetMapper clipMapper
    eval [clipActor GetProperty] SetColor $peacock
    clipActor SetBackfaceProperty backProp

# Create graphics stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor clipActor
ren1 SetBackground 1 1 1
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 30
[ren1 GetActiveCamera] Dolly 1.2

renWin SetSize 400 400
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin SetFileName "clipSphere.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



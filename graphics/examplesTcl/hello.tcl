catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl
source "colors.tcl"
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# create lines
vtkPolyReader reader
    reader SetFileName "../../../data/hello.vtk"
vtkPolyMapper lineMapper
    lineMapper SetInput [reader GetOutput]
vtkActor lineActor
    lineActor SetMapper lineMapper
    eval [lineActor GetProperty] SetColor $red

# create implicit model
vtkImplicitModeller imp
    imp SetInput [reader GetOutput]
    imp SetSampleDimensions 110 40 20
    imp SetMaximumDistance 0.25
    imp SetModelBounds -1.0 10.0 -1.0 3.0 -1.0 1.0
vtkContourFilter contour
    contour SetInput [imp GetOutput]
    contour SetValue 0 0.25
vtkPolyMapper impMapper
    impMapper SetInput [contour GetOutput]
    impMapper ScalarsVisibleOff
vtkActor impActor;  
    impActor SetMapper impMapper
    eval [impActor GetProperty] SetColor $peacock
    [impActor GetProperty] SetOpacity 0.5

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors lineActor
$ren1 AddActors impActor
$ren1 SetBackground 1 1 1
$renWin SetSize 600 250

vtkCamera camera
  camera SetClippingRange 1.81325 90.6627
  camera SetFocalPoint  4.5  1  0
  camera SetPosition  4.5 1.0 6.73257
  camera CalcViewPlaneNormal
  camera SetViewUp  0  1  0
  camera Zoom 0.8
$ren1 SetActiveCamera camera

$iren Initialize

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
#$renWin SetFileName "hello.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



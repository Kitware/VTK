catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create lines
vtkPolyDataReader reader
    reader SetFileName "$VTK_DATA/hello.vtk"
vtkPolyDataMapper lineMapper
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
vtkPolyDataMapper impMapper
    impMapper SetInput [contour GetOutput]
    impMapper ScalarVisibilityOff
vtkActor impActor;  
    impActor SetMapper impMapper
    eval [impActor GetProperty] SetColor $peacock
    [impActor GetProperty] SetOpacity 0.5

# Add the actors to the renderer, set the background and size
#
ren1 AddActor lineActor
ren1 AddActor impActor
ren1 SetBackground 1 1 1
renWin SetSize 600 250

vtkCamera camera
  camera SetClippingRange 1.81325 90.6627
  camera SetFocalPoint  4.5  1  0
  camera SetPosition  4.5 1.0 6.73257
  camera SetViewUp  0  1  0
  camera Zoom 0.8
ren1 SetActiveCamera camera

iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
#renWin SetFileName "hello.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



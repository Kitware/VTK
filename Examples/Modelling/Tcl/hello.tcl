# This example demonstrates how to use implicit modelling.

# first we load in the standard vtk packages into tcl
package require vtk
package require vtkinteraction
package require vtktesting

# Create lines which serve as the "seed" geometry. The lines spell the
# word "hello".
#
vtkPolyDataReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/hello.vtk"
vtkPolyDataMapper lineMapper
    lineMapper SetInput [reader GetOutput]
vtkActor lineActor
    lineActor SetMapper lineMapper
    eval [lineActor GetProperty] SetColor $red

# Create implicit model with vtkImplicitModeller. This computes a scalar
# field which is the distance from the generating geometry. The contour
# filter then extracts the geoemtry at the distance value 0.25 from the
# generating geometry.
#
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

# Create the usual graphics stuff.
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

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
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



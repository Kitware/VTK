package require vtk
package require vtkinteraction
package require vtktesting

# use implicit modeller / interpolation to perform 3D morphing
#
# make the letter v
vtkVectorText letterV
  letterV SetText v

# read the geometry file containing the letter t
vtkVectorText letterT
  letterT SetText t

# read the geometry file containing the letter k
vtkVectorText letterK
  letterK SetText k

# create implicit models of each
vtkImplicitModeller blobbyV
  blobbyV SetInput [letterV GetOutput]
  blobbyV SetMaximumDistance .2
  blobbyV SetSampleDimensions 50 50 12
  blobbyV SetModelBounds -0.5 1.5 -0.5 1.5 -0.5 0.5

# create implicit models of each
vtkImplicitModeller blobbyT
  blobbyT SetInput [letterT GetOutput]
  blobbyT SetMaximumDistance .2
  blobbyT SetSampleDimensions 50 50 12
  blobbyT SetModelBounds -0.5 1.5 -0.5 1.5 -0.5 0.5

# create implicit models of each
vtkImplicitModeller blobbyK
  blobbyK SetInput [letterK GetOutput]
  blobbyK SetMaximumDistance .2
  blobbyK SetSampleDimensions 50 50 12
  blobbyK SetModelBounds -0.5 1.5 -0.5 1.5 -0.5 0.5

# Interpolate the data
vtkInterpolateDataSetAttributes interpolate
  interpolate AddInput [blobbyV GetOutput]
  interpolate AddInput [blobbyT GetOutput]
  interpolate AddInput [blobbyK GetOutput]
  interpolate SetT 0.0

# extract an iso surface
vtkContourFilter blobbyIso
  blobbyIso SetInput [interpolate GetOutput]
  blobbyIso SetValue 0 0.1

# map to rendering primitives
vtkPolyDataMapper blobbyMapper
  blobbyMapper SetInput [blobbyIso GetOutput]
  blobbyMapper ScalarVisibilityOff

# now an actor
vtkActor blobby
  blobby SetMapper blobbyMapper
  eval [blobby GetProperty] SetDiffuseColor $banana

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

vtkCamera camera
    camera SetClippingRange 0.265 13.2
    camera SetFocalPoint 0.539 0.47464 0
    camera SetPosition 0.539 0.474674 2.644
    camera SetViewUp 0 1 0
ren1 SetActiveCamera camera

#  now  make a renderer and tell it about lights and actors
renWin SetSize 300 350
  
ren1 AddActor blobby
ren1 SetBackground 1 1 1
renWin Render 

set subIters 4.0
for {set i 0} {$i < 2} {incr i} {
   for {set j 1} {$j <= $subIters} {incr j} {
      set t [expr $i + $j/$subIters]
      interpolate SetT $t
      renWin Render
   }
}


# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


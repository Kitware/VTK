catch {load vtktcl}

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a plane 
#
vtkPlaneSource plane
  plane SetResolution 5 5

vtkTriangleFilter planeTris
  planeTris SetInput [plane GetOutput]

vtkPlane halfPlane
  halfPlane SetOrigin -.13 -.03 0
  halfPlane SetNormal 1 .2 0

vtkCutter planeCutter
  planeCutter SetCutFunction halfPlane
  planeCutter SetInput [planeTris GetOutput]
  planeCutter SetValue 0 0

vtkPolyDataMapper   planeMapper
  planeMapper SetInput [planeCutter GetOutput]
  planeMapper ScalarVisibilityOff

vtkActor planeActor
  planeActor SetMapper planeMapper
  [planeActor GetProperty] SetDiffuseColor 0 0 0
  [planeActor GetProperty] SetRepresentationToWireframe
# Add the actors to the renderer, set the background and size
#
ren1 AddActor planeActor
ren1 SetBackground 1 1 1
renWin SetSize 300 300

#renWin SetFileName "cutPlane.tcl.ppm"
#renWin SaveImageAsPPM


# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .



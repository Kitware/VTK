#
# clip a textured plane
#

catch {load vtktcl}

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Create a plane source
vtkPlaneSource plane

vtkTransform aTransform
  aTransform RotateX 30
  aTransform RotateY 30

vtkTransformPolyDataFilter transformPlane
  transformPlane SetInput [plane GetOutput]
  transformPlane SetTransform aTransform

vtkPlane clipPlane1
  clipPlane1 SetNormal  0 0 1

vtkClipPolyData clipper
  clipper GenerateClipScalarsOn
  clipper SetValue 0
  clipper SetClipFunction clipPlane1
  clipper SetInput [transformPlane GetOutput]

vtkPolyDataMapper  planeMapper
  planeMapper SetInput [clipper GetOutput]
  planeMapper ScalarVisibilityOff

vtkActor planeActor
  planeActor SetMapper planeMapper

vtkPolyDataMapper  plane2Mapper
  plane2Mapper SetInput [plane GetOutput]

vtkActor plane2Actor
  plane2Actor SetMapper plane2Mapper


# load in the texture map
#
vtkTexture atext
vtkPNMReader pnmReader
  pnmReader SetFileName "../../../vtkdata/masonry.ppm"

atext SetInput [pnmReader GetOutput]
atext InterpolateOn
planeActor SetTexture atext

# Add the actors to the renderer, set the background and size
ren1 AddActor planeActor
ren1 AddActor plane2Actor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# render the image
iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Elevation -30
$cam1 Roll -20
renWin Render

#renWin SetFileName "clipTexPlane.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .






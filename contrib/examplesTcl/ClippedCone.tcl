catch {load vtktcl}
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkConeSource cone
  cone SetResolution 50

vtkPolyDataMapper   coneMapper
    coneMapper SetInput [cone GetOutput]
    coneMapper GlobalImmediateModeRenderingOn

vtkActor coneActor
    coneActor SetMapper coneMapper
[coneActor GetProperty] SetDiffuseColor 1 1 1

vtkPlane plane
    plane SetOrigin 0 0 0
    plane SetNormal -1 0 0
vtkClipPolyData clipper
    clipper SetInput [cone GetOutput]
    clipper SetClipFunction plane
    clipper GenerateClipScalarsOn
    clipper GenerateClippedOutputOn
    clipper SetValue 0
vtkPolyDataMapper clipMapper
    clipMapper SetInput [clipper GetClippedOutput]
    clipMapper ScalarVisibilityOff
vtkProperty backProp
    eval backProp SetDiffuseColor $tomato
vtkActor clipActor
    clipActor SetMapper clipMapper
    eval [clipActor GetProperty] SetColor $peacock
    clipActor SetBackfaceProperty backProp

ren1 AddActor clipActor 

# Create polygons outlining clipped areas and triangulate them to generate cut surface
vtkCutter cutEdges; #Generate cut lines
  cutEdges SetInput [cone GetOutput]
  cutEdges SetCutFunction plane
  cutEdges GenerateCutScalarsOn
  cutEdges SetValue 0 0
vtkStripper cutStrips; #Forms loops (closed polylines) from cutter
  cutStrips SetInput [cutEdges GetOutput]
  cutStrips Update
vtkPolyData cutPoly; #This trick defines polygons as polyline loop
  cutPoly SetPoints [[cutStrips GetOutput] GetPoints]
  cutPoly SetPolys [[cutStrips GetOutput] GetLines]
vtkTriangleFilter cutTriangles; #Triangulates the polygons to create cut surface
  cutTriangles SetInput cutPoly
vtkAppendPolyData coneAppend
  coneAppend AddInput [clipper GetClippedOutput]
  coneAppend AddInput [cutTriangles GetOutput]

vtkPolyDataMapper cutMapper
  cutMapper SetInput [coneAppend GetOutput]

vtkActor cutActor
  cutActor SetMapper cutMapper
  eval [cutActor GetProperty] SetColor $peacock

ren1 AddActor cutActor

ren1 AddActor coneActor
coneActor SetPosition 0 -1 0
vtkMassProperties cone1Mass
  cone1Mass SetInput [coneAppend GetOutput]
#  puts "[cone1Mass Print]"

ren1 SetBackground .4 .2 .1

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Azimuth -30
$cam1 Elevation -30

iren Initialize

#renWin SetFileName "ClippedCone.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

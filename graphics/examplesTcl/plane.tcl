catch {load vtktcl}
# this is a tcl version: tests polygonal planes
# include get the vtk interactor ui
source vtkInt.tcl

vtkPlaneSource plane
    plane SetResolution 4 5
    plane SetOrigin 0 0 1
    plane SetPoint1 2 0 1
    plane SetPoint2 0 3 1
    plane SetCenter 3 2 1
    plane SetNormal 1 2 3
    plane Update
vtkPolyMapper planeMapper
    planeMapper SetInput [plane GetOutput]
vtkActor planeActor
    planeActor SetMapper planeMapper
    [planeActor GetProperty] SetRepresentationToWireframe
# create simple poly data so we can apply glyph
vtkFloatPoints pts
    eval pts InsertPoint 0 [plane GetCenter]
vtkFloatNormals normal
    eval normal InsertNormal 0 [plane GetNormal]
vtkPolyData pd
    pd SetPoints pts
    [pd GetPointData] SetNormals normal

vtkConeSource cone
    cone SetResolution 6
vtkTransform transform
    transform Scale .2 .2 .2
    transform Translate 0.5 0.0 0.0
vtkTransformPolyFilter transformF
    transformF SetInput [cone GetOutput]
    transformF SetTransform transform
vtkGlyph3D glyph
    glyph SetInput pd
    glyph SetSource [transformF GetOutput]
    glyph SetVectorModeToUseNormal
vtkPolyMapper mapGlyph
    mapGlyph SetInput [glyph GetOutput]
vtkActor glyphActor
    glyphActor SetMapper mapGlyph
    [glyphActor GetProperty] SetColor 1 0 0

#
# Create the rendering stuff
#
vtkRenderMaster rm
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

$ren1 AddActors planeActor
$ren1 AddActors glyphActor
$ren1 SetBackground 0.1 0.2 0.4
$renWin SetSize 450 450

# Get handles to some useful objects
#
$iren SetUserMethod {wm deiconify .vtkInteract}
$renWin Render
#$renWin SetFileName "plane.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



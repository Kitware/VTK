package require vtk
package require vtkinteraction
package require vtktesting

#
# Demonstrate the use of clipping and capping on polyhedral data. Also shows how to
# use triangle filter to triangulate loops.
#

# create pipeline
#
# Read the polygonal data and generate vertex normals
vtkBYUReader cow
  cow SetGeometryFileName "$VTK_DATA_ROOT/Data/Viewpoint/cow.g"
vtkPolyDataNormals cowNormals
  cowNormals SetInputConnection [cow GetOutputPort]

# Define a clip plane to clip the cow in half
vtkPlane plane
    plane SetOrigin 0.25 0 0
    plane SetNormal -1 -1 0
vtkClipPolyData clipper
    clipper SetInputConnection [cowNormals GetOutputPort]
    clipper SetClipFunction plane
    clipper GenerateClipScalarsOn
    clipper GenerateClippedOutputOn
    clipper SetValue 0.5
vtkPolyDataMapper clipMapper
    clipMapper SetInputConnection [clipper GetOutputPort]
    clipMapper ScalarVisibilityOff
vtkProperty backProp
    eval backProp SetDiffuseColor $tomato
vtkActor clipActor
    clipActor SetMapper clipMapper
    eval [clipActor GetProperty] SetColor $peacock
    clipActor SetBackfaceProperty backProp

# Create polygons outlining clipped areas and triangulate them to generate cut surface
vtkCutter cutEdges; #Generate cut lines
  cutEdges SetInputConnection [cowNormals GetOutputPort]
  cutEdges SetCutFunction plane
  cutEdges GenerateCutScalarsOn
  cutEdges SetValue 0 0.5
vtkStripper cutStrips; #Forms loops (closed polylines) from cutter
  cutStrips SetInputConnection [cutEdges GetOutputPort]
  cutStrips Update
vtkPolyData cutPoly; #This trick defines polygons as polyline loop
  cutPoly SetPoints [[cutStrips GetOutput] GetPoints]
  cutPoly SetPolys [[cutStrips GetOutput] GetLines]
vtkTriangleFilter cutTriangles; #Triangulates the polygons to create cut surface
  cutTriangles SetInputData cutPoly
vtkPolyDataMapper cutMapper
  cutMapper SetInputData cutPoly
  cutMapper SetInputConnection [cutTriangles GetOutputPort]
vtkActor cutActor
  cutActor SetMapper cutMapper
  eval [cutActor GetProperty] SetColor $peacock

# Create the rest of the cow in wireframe
vtkPolyDataMapper restMapper
  restMapper SetInputData [clipper GetClippedOutput]
  restMapper ScalarVisibilityOff
vtkActor restActor
  restActor SetMapper restMapper
  [restActor GetProperty] SetRepresentationToWireframe

# Create graphics stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
ren1 AddActor clipActor
ren1 AddActor cutActor
ren1 AddActor restActor
ren1 SetBackground 1 1 1
ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 30
[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCameraClippingRange

renWin SetSize 300 300
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


# Lets you move the cut plane back and forth
proc Cut {v} {
    clipper SetValue $v
    cutEdges SetValue 0 $v
    cutStrips Update
    cutPoly SetPoints [[cutStrips GetOutput] GetPoints]
    cutPoly SetPolys [[cutStrips GetOutput] GetLines]
    cutMapper Update
    renWin Render
}

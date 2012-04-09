# In this example vtkClipPolyData is used to cut a polygonal model
# of a cow in half. In addition, the open clip is closed by triangulating
# the resulting complex polygons.

package require vtk
package require vtkinteraction
package require vtktesting

# First start by reading a cow model. We also generate surface normals for
# prettier rendering.
vtkBYUReader cow
  cow SetGeometryFileName "$VTK_DATA_ROOT/Data/Viewpoint/cow.g"
vtkPolyDataNormals cowNormals
  cowNormals SetInputConnection [cow GetOutputPort]

# We clip with an implicit function. Here we use a plane positioned near
# the center of the cow model and oriented at an arbitrary angle.
vtkPlane plane
    plane SetOrigin 0.25 0 0
    plane SetNormal -1 -1 0

# vtkClipPolyData requires an implicit function to define what it is to
# clip with. Any implicit function, including complex boolean combinations
# can be used. Notice that we can specify the value of the implicit function
# with the SetValue method.
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

# Here we are cutting the cow. Cutting creates lines where the cut function
# intersects the model. (Clipping removes a portion of the model but the
# dimension of the data does not change.)
#
# The reason we are cutting is to generate a closed polygon at the boundary
# of the clipping process. The cutter generates line segments, the stripper
# then puts them together into polylines. We then pull a trick and define
# polygons using the closed line segements that the stripper created.
#
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

# Triangle filter is robust enough to ignore the duplicate point at the
# beginning and end of the polygons and triangulate them.
vtkTriangleFilter cutTriangles
  cutTriangles SetInputData cutPoly
vtkPolyDataMapper cutMapper
  cutMapper SetInputData cutPoly
  cutMapper SetInputConnection [cutTriangles GetOutputPort]
vtkActor cutActor
  cutActor SetMapper cutMapper
  eval [cutActor GetProperty] SetColor $peacock

# The clipped part of the cow is rendered wireframe.
vtkPolyDataMapper restMapper
  restMapper SetInputConnection [clipper GetClippedOutputPort]
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

# Lets you move the cut plane back and forth by invoking the proc Cut with
# the appropriate plane value (essentially a distance from the original
# plane.
#
proc Cut {v} {
    clipper SetValue $v
    cutEdges SetValue 0 $v
    cutStrips Update
    cutPoly SetPoints [[cutStrips GetOutput] GetPoints]
    cutPoly SetPolys [[cutStrips GetOutput] GetLines]
    cutMapper Update
    renWin Render
}

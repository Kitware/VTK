package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer, and RenderWindowInteractor
#
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# create pipeline
#
set PIECE 0
set NUMBER_OF_PIECES 8

vtkImageReader reader
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 63 0 63 1 64
  reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  reader SetDataMask 0x7fff
  reader SetDataSpacing 1.6 1.6 1.5

vtkImageClip clipper
  clipper SetInputConnection [reader GetOutputPort]
  clipper SetOutputWholeExtent 30 36 30 36 30 36

vtkImageClip clipper2
  clipper2 SetInputConnection [reader GetOutputPort]
  clipper2 SetOutputWholeExtent 30 36 30 36 30 36

vtkDataSetTriangleFilter tris
  tris SetInputConnection [clipper GetOutputPort]

vtkDataSetTriangleFilter tris2
  tris2 SetInputConnection [clipper2 GetOutputPort]

vtkGeometryFilter geom
  geom SetInputConnection [tris GetOutputPort]

vtkExtractEdges edges
  edges SetInputConnection [tris2 GetOutputPort]

vtkPolyDataMapper mapper1
  mapper1 SetInputConnection [geom GetOutputPort]
  mapper1 ScalarVisibilityOn
  mapper1 SetScalarRange 0 1200
  mapper1 SetPiece $PIECE
  mapper1 SetNumberOfPieces $NUMBER_OF_PIECES

vtkPolyDataMapper mapper2
  mapper2 SetInputConnection [edges GetOutputPort]
  mapper2 SetPiece $PIECE
  mapper2 SetNumberOfPieces $NUMBER_OF_PIECES

vtkActor actor1
  actor1 SetMapper mapper1

vtkActor actor2
  actor2 SetMapper mapper2

# add the actor to the renderer; set the size
#
ren1 AddActor actor1
ren1 AddActor actor2
renWin SetSize 450 450
ren1 SetBackground 1 1 1

renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .










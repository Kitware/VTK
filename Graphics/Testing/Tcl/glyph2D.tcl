package require vtk
package require vtkinteraction

# Create some random points, scalars, and vectors to glyph
#
vtkPolyData pd
vtkPoints pts
vtkFloatArray scalars
vtkFloatArray vectors
vectors SetNumberOfComponents 3

pd SetPoints pts
  [pd GetPointData] SetScalars scalars
  [pd GetPointData] SetVectors vectors

vtkMath math
set size 500
for {set i 0} {$i < 100} {incr i} {
  pts InsertNextPoint [math Random 0 [expr $size - 1]] \
                      [math Random 0 [expr $size - 1]] 0.0
  scalars InsertNextValue [math Random 0.0 5]
    vectors InsertNextTuple3 [math Random -1 1] [math Random -1 1] 0.0
}

vtkGlyphSource2D gs
  gs SetGlyphTypeToCircle
  gs SetScale 20
  gs FilledOff
  gs CrossOn
vtkGlyphSource2D gs1
  gs1 SetGlyphTypeToTriangle
  gs1 SetScale 20
  gs1 FilledOff
  gs1 CrossOn
vtkGlyphSource2D gs2
  gs2 SetGlyphTypeToSquare
  gs2 SetScale 20
  gs2 FilledOff
  gs2 CrossOn
vtkGlyphSource2D gs3
  gs3 SetGlyphTypeToDiamond
  gs3 SetScale 20
  gs3 FilledOff
  gs3 CrossOn
vtkGlyphSource2D gs4
  gs4 SetGlyphTypeToDiamond
  gs4 SetScale 20
  gs4 FilledOn
  gs4 DashOn
  gs4 CrossOff
vtkGlyphSource2D gs5
  gs5 SetGlyphTypeToThickArrow
  gs5 SetScale 20
  gs5 FilledOn
  gs5 CrossOff

# Create a table of glyphs
vtkGlyph2D glypher
  glypher SetInput pd
  glypher SetSource 0 [gs GetOutput]
  glypher SetSource 1 [gs1 GetOutput]
  glypher SetSource 2 [gs2 GetOutput]
  glypher SetSource 3 [gs3 GetOutput]
  glypher SetSource 4 [gs4 GetOutput]
  glypher SetSource 5 [gs5 GetOutput]
  glypher SetIndexModeToScalar 
  glypher SetRange 0 5
  glypher SetScaleModeToDataScalingOff

vtkPolyDataMapper2D mapper
    mapper SetInputConnection [glypher GetOutputPort]
    mapper SetScalarRange 0 5

vtkActor2D glyphActor
    glyphActor SetMapper mapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin SetMultiSamples 0
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor glyphActor
ren1 SetBackground 1 1 1

renWin SetSize $size $size
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .




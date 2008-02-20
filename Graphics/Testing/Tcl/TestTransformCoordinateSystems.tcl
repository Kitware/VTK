package require vtk
package require vtkinteraction
package require vtktesting

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# pipeline stuff
#
vtkSphereSource sphere
  sphere SetPhiResolution 10
  sphere SetThetaResolution 20

vtkTransformCoordinateSystems xform
  xform SetInputConnection [sphere GetOutputPort]
  xform SetInputCoordinateSystemToWorld
  xform SetOutputCoordinateSystemToDisplay
  xform SetViewport ren1

vtkGlyphSource2D gs
  gs SetGlyphTypeToCircle
  gs SetScale 20
  gs FilledOff
  gs CrossOn

# Create a table of glyphs
vtkGlyph2D glypher
  glypher SetInputConnection [xform GetOutputPort]
  glypher SetSource 0 [gs GetOutput]
  glypher SetScaleModeToDataScalingOff

vtkPolyDataMapper2D mapper
    mapper SetInputConnection [glypher GetOutputPort]

vtkActor2D glyphActor
    glyphActor SetMapper mapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor glyphActor
ren1 SetBackground 0 0 0

renWin SetSize 300 300
iren Initialize
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .




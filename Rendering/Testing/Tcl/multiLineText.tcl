package require vtk
package require vtkinteraction

# demonstrate the use of multiline 2D text

vtkTextMapper singleLineTextB
    singleLineTextB SetInput "Single line (bottom)"
    singleLineTextB SetFontSize 14
    singleLineTextB SetFontFamilyToArial
    singleLineTextB BoldOff
    singleLineTextB ItalicOff
    singleLineTextB ShadowOff
    singleLineTextB SetVerticalJustificationToBottom
vtkActor2D singleLineTextActorB
    singleLineTextActorB SetMapper singleLineTextB    
    [singleLineTextActorB GetPositionCoordinate] \
          SetCoordinateSystemToNormalizedDisplay
    [singleLineTextActorB GetPositionCoordinate] SetValue 0.05 0.85
    [singleLineTextActorB GetProperty] SetColor 1 0 0

vtkTextMapper singleLineTextC
    singleLineTextC SetInput "Single line (centered)"
    singleLineTextC SetFontSize 14
    singleLineTextC SetFontFamilyToArial
    singleLineTextC BoldOff
    singleLineTextC ItalicOff
    singleLineTextC ShadowOff
    singleLineTextC SetVerticalJustificationToCentered
vtkActor2D singleLineTextActorC
    singleLineTextActorC SetMapper singleLineTextC    
    [singleLineTextActorC GetPositionCoordinate] \
          SetCoordinateSystemToNormalizedDisplay
    [singleLineTextActorC GetPositionCoordinate] SetValue 0.05 0.75
    [singleLineTextActorC GetProperty] SetColor 0 1 0

vtkTextMapper singleLineTextT
    singleLineTextT SetInput "Single line (top)"
    singleLineTextT SetFontSize 14
    singleLineTextT SetFontFamilyToArial
    singleLineTextT BoldOff
    singleLineTextT ItalicOff
    singleLineTextT ShadowOff
    singleLineTextT SetVerticalJustificationToTop
vtkActor2D singleLineTextActorT
    singleLineTextActorT SetMapper singleLineTextT    
    [singleLineTextActorT GetPositionCoordinate] \
          SetCoordinateSystemToNormalizedDisplay
    [singleLineTextActorT GetPositionCoordinate] SetValue 0.05 0.65
    [singleLineTextActorT GetProperty] SetColor 0 0 1

vtkTextMapper textMapperL
    textMapperL SetInput "This is\nmulti-line\ntext output\n(left-top)"
    textMapperL SetFontSize 14
    textMapperL SetFontFamilyToArial
    textMapperL BoldOn
    textMapperL ItalicOn
    textMapperL ShadowOn
    textMapperL SetJustificationToLeft
    textMapperL SetVerticalJustificationToTop
    textMapperL SetLineSpacing 0.8
vtkActor2D textActorL
    textActorL SetMapper textMapperL    
    [textActorL GetPositionCoordinate] SetCoordinateSystemToNormalizedDisplay
    [textActorL GetPositionCoordinate] SetValue 0.05 0.5
    [textActorL GetProperty] SetColor 1 0 0

vtkTextMapper textMapperC
    textMapperC SetInput "This is\nmulti-line\ntext output\n(centered)"
    textMapperC SetFontSize 14
    textMapperC SetFontFamilyToArial
    textMapperC BoldOn
    textMapperC ItalicOn
    textMapperC ShadowOn
    textMapperC SetJustificationToCentered
    textMapperC SetVerticalJustificationToCentered
    textMapperC SetLineSpacing 0.8
vtkActor2D textActorC
    textActorC SetMapper textMapperC    
    [textActorC GetPositionCoordinate] SetCoordinateSystemToNormalizedDisplay
    [textActorC GetPositionCoordinate] SetValue 0.5 0.5
    [textActorC GetProperty] SetColor 0 1 0

vtkTextMapper textMapperR
    textMapperR SetInput "This is\nmulti-line\ntext output\n(right-bottom)"
    textMapperR SetFontSize 14
    textMapperR SetFontFamilyToArial
    textMapperR BoldOn
    textMapperR ItalicOn
    textMapperR ShadowOn
    textMapperR SetJustificationToRight
    textMapperR SetVerticalJustificationToBottom
    textMapperR SetLineSpacing 0.8
vtkActor2D textActorR
    textActorR SetMapper textMapperR    
    [textActorR GetPositionCoordinate] SetCoordinateSystemToNormalizedDisplay
    [textActorR GetPositionCoordinate] SetValue 0.95 0.5
    [textActorR GetProperty] SetColor 0 0 1

vtkPoints Pts
    Pts InsertNextPoint 0.05 0.0 0.0
    Pts InsertNextPoint 0.05 1.0 0.0
    Pts InsertNextPoint 0.5 0.0 0.0
    Pts InsertNextPoint 0.5 1.0 0.0
    Pts InsertNextPoint 0.95 0.0 0.0
    Pts InsertNextPoint 0.95 1.0 0.0
    Pts InsertNextPoint 0.0 0.5 0.0
    Pts InsertNextPoint 1.0 0.5 0.0
    Pts InsertNextPoint 0.00 0.85 0.0
    Pts InsertNextPoint 0.50 0.85 0.0
    Pts InsertNextPoint 0.00 0.75 0.0
    Pts InsertNextPoint 0.50 0.75 0.0
    Pts InsertNextPoint 0.00 0.65 0.0
    Pts InsertNextPoint 0.50 0.65 0.0
vtkCellArray Lines
    Lines InsertNextCell 2
    Lines InsertCellPoint 0
    Lines InsertCellPoint 1
    Lines InsertNextCell 2
    Lines InsertCellPoint 2
    Lines InsertCellPoint 3
    Lines InsertNextCell 2
    Lines InsertCellPoint 4
    Lines InsertCellPoint 5
    Lines InsertNextCell 2
    Lines InsertCellPoint 6
    Lines InsertCellPoint 7
    Lines InsertNextCell 2
    Lines InsertCellPoint 8
    Lines InsertCellPoint 9
    Lines InsertNextCell 2
    Lines InsertCellPoint 10
    Lines InsertCellPoint 11
    Lines InsertNextCell 2
    Lines InsertCellPoint 12
    Lines InsertCellPoint 13
vtkPolyData Grid
    Grid SetPoints Pts
    Grid SetLines Lines
vtkCoordinate normCoords
    normCoords SetCoordinateSystemToNormalizedViewport
vtkPolyDataMapper2D mapper
    mapper SetInput Grid
    mapper SetTransformCoordinate normCoords
vtkActor2D gridActor
    gridActor SetMapper mapper
    [gridActor GetProperty] SetColor 0.1 0.1 0.1

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor2D textActorL
ren1 AddActor2D textActorC
ren1 AddActor2D textActorR
ren1 AddActor2D singleLineTextActorB
ren1 AddActor2D singleLineTextActorC
ren1 AddActor2D singleLineTextActorT
ren1 AddActor2D gridActor

ren1 SetBackground 1 1 1
renWin SetSize 500 300
[ren1 GetActiveCamera] Zoom 1.5
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .




# This example demonstrates the use of multiline 2D text using
# vtkTextMappers.  It shows several justifications as well as single-line
# and multiple-line text inputs.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

set font_size 14

# Create the text mappers and the associated Actor2Ds.

# The font and text properties (except justification) are the same for each
# single line mapper. Let's create a common text property object

vtkTextProperty singleLineTextProp
    singleLineTextProp SetFontSize $font_size
    singleLineTextProp SetFontFamilyToArial
    singleLineTextProp BoldOff
    singleLineTextProp ItalicOff
    singleLineTextProp ShadowOff

# The font and text properties (except justification) are the same for each
# multi line mapper. Let's create a common text property object

vtkTextProperty multiLineTextProp
    multiLineTextProp ShallowCopy singleLineTextProp
    multiLineTextProp BoldOn
    multiLineTextProp ItalicOn
    multiLineTextProp ShadowOn
    multiLineTextProp SetLineSpacing 0.8

# The text is on a single line and bottom-justified.
vtkTextMapper singleLineTextB
    singleLineTextB SetInput "Single line (bottom)"
    set tprop [singleLineTextB GetTextProperty]
    $tprop ShallowCopy singleLineTextProp
    $tprop SetVerticalJustificationToBottom
    $tprop SetColor 1 0 0
vtkActor2D singleLineTextActorB
    singleLineTextActorB SetMapper singleLineTextB
    [singleLineTextActorB GetPositionCoordinate] \
          SetCoordinateSystemToNormalizedDisplay
    [singleLineTextActorB GetPositionCoordinate] SetValue 0.05 0.85

# The text is on a single line and center-justified (vertical justification).
vtkTextMapper singleLineTextC
    singleLineTextC SetInput "Single line (centered)"
    set tprop [singleLineTextC GetTextProperty]
    $tprop ShallowCopy singleLineTextProp
    $tprop SetVerticalJustificationToCentered
    $tprop SetColor 0 1 0
vtkActor2D singleLineTextActorC
    singleLineTextActorC SetMapper singleLineTextC
    [singleLineTextActorC GetPositionCoordinate] \
          SetCoordinateSystemToNormalizedDisplay
    [singleLineTextActorC GetPositionCoordinate] SetValue 0.05 0.75

# The text is on a single line and top-justified.
vtkTextMapper singleLineTextT
    singleLineTextT SetInput "Single line (top)"
    set tprop [singleLineTextT GetTextProperty]
    $tprop ShallowCopy singleLineTextProp
    $tprop SetVerticalJustificationToTop
    $tprop SetColor 0 0 1
vtkActor2D singleLineTextActorT
    singleLineTextActorT SetMapper singleLineTextT
    [singleLineTextActorT GetPositionCoordinate] \
          SetCoordinateSystemToNormalizedDisplay
    [singleLineTextActorT GetPositionCoordinate] SetValue 0.05 0.65

# The text is on multiple lines and left- and top-justified.
vtkTextMapper textMapperL
    textMapperL SetInput "This is\nmulti-line\ntext output\n(left-top)"
    set tprop [textMapperL GetTextProperty]
    $tprop ShallowCopy multiLineTextProp
    $tprop SetJustificationToLeft
    $tprop SetVerticalJustificationToTop
    $tprop SetColor 1 0 0
vtkActor2D textActorL
    textActorL SetMapper textMapperL
    [textActorL GetPositionCoordinate] SetCoordinateSystemToNormalizedDisplay
    [textActorL GetPositionCoordinate] SetValue 0.05 0.5

# The text is on multiple lines and center-justified (both horizontal and
# vertical).
vtkTextMapper textMapperC
    textMapperC SetInput "This is\nmulti-line\ntext output\n(centered)"
    set tprop [textMapperC GetTextProperty]
    $tprop ShallowCopy multiLineTextProp
    $tprop SetJustificationToCentered
    $tprop SetVerticalJustificationToCentered
    $tprop SetColor 0 1 0
vtkActor2D textActorC
    textActorC SetMapper textMapperC
    [textActorC GetPositionCoordinate] SetCoordinateSystemToNormalizedDisplay
    [textActorC GetPositionCoordinate] SetValue 0.5 0.5

# The text is on multiple lines and right- and bottom-justified.
vtkTextMapper textMapperR
    textMapperR SetInput "This is\nmulti-line\ntext output\n(right-bottom)"
    set tprop [textMapperR GetTextProperty]
    $tprop ShallowCopy multiLineTextProp
    $tprop SetJustificationToRight
    $tprop SetVerticalJustificationToBottom
    $tprop SetColor 0 0 1
vtkActor2D textActorR
    textActorR SetMapper textMapperR
    [textActorR GetPositionCoordinate] SetCoordinateSystemToNormalizedDisplay
    [textActorR GetPositionCoordinate] SetValue 0.95 0.5

# Draw the grid to demonstrate the placement of the text.

# Set up the necessary points.
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
# Set up the lines that use these points.
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
# Create a grid that uses these points and lines.
vtkPolyData Grid
    Grid SetPoints Pts
    Grid SetLines Lines
# Set up the coordinate system.
vtkCoordinate normCoords
    normCoords SetCoordinateSystemToNormalizedViewport

# Set up the mapper and actor (2D) for the grid.
vtkPolyDataMapper2D mapper
    mapper SetInputData Grid
    mapper SetTransformCoordinate normCoords
vtkActor2D gridActor
    gridActor SetMapper mapper
    [gridActor GetProperty] SetColor 0.1 0.1 0.1

# Create the Renderer, RenderWindow, and RenderWindowInteractor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer; set the background and size; zoom in
# closer to the image; render
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

# Set the user method (bound to key 'u')
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# Withdraw the default tk window.
wm withdraw .
iren Start



# This example demonstrates how to use the legend box
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create some legend symbols
vtkSphereSource ss
vtkGlyphSource2D gs2
  gs2 SetGlyphTypeToTriangle
  gs2 FilledOff
  gs2 SetColor 0 1 0
vtkGlyphSource2D gs3
  gs3 SetGlyphTypeToSquare
  gs3 FilledOff
  gs3 SetColor 0 0 1
vtkGlyphSource2D gs4
  gs4 SetGlyphTypeToDiamond
  gs4 FilledOff
  gs4 SetColor 1 1 0
vtkGlyphSource2D gs5
  gs5 SetGlyphTypeToCross
  gs5 FilledOff
  gs5 SetColor 0 1 1
vtkGlyphSource2D gs6
  gs6 SetGlyphTypeToThickCross
  gs6 FilledOff
  gs6 SetColor 1 0 1

# Create a 1D axis
vtkLegendBoxActor legend
    [legend GetPositionCoordinate] SetValue 0.25 0.25 0
    [legend GetPosition2Coordinate] SetValue 0.5 0.5 0;#relative to Position
    legend SetNumberOfEntries 6
    legend SetEntrySymbol 0 [ss GetOutput]
    legend SetEntryString 0 "First entry"
    legend SetEntrySymbol 1 [gs2 GetOutput]
    legend SetEntryString 1 "Second entry"
    legend SetEntrySymbol 2 [gs3 GetOutput]
    legend SetEntryString 2 "Third entry"
    eval legend SetEntryColor 2 [gs3 GetColor]
    legend SetEntrySymbol 3 [gs4 GetOutput]
    legend SetEntryString 3 "Fourth entry"
    legend SetEntrySymbol 4 [gs5 GetOutput]
    legend SetEntryString 4 "Fifth entry"
    legend SetEntrySymbol 5 [gs6 GetOutput]
    legend SetEntryString 5 "Sixth entry"
    legend SetPadding 5
    legend ShadowOff
    [legend GetProperty] SetColor 0 1 0

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor2D legend
renWin SetSize 200 250

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render
iren Initialize

#renWin SetFileName "plot.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


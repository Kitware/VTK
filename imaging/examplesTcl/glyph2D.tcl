# This example demonstrates how to generate and manipulate 2D glyphs
catch {load vtktcl}

if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# create the visualization pipeline
#
vtkMath math

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 SetBackground 1 1 1
renWin SetSize 300 150

#procedure used to create actors and such
proc MakeActor {glyph} {
   vtkPolyDataMapper $glyph{Mapper}
     $glyph{Mapper} SetInput [$glyph GetOutput]
   vtkActor $glyph{Actor}
     $glyph{Actor} SetMapper $glyph{Mapper}
   ren1 AddActor $glyph{Actor}
}

#wire glyphs (not filled)
vtkGlyphSource2D g0
    g0 SetGlyphTypeToNone
    g0 SetCenter 0 0 0
    g0 SetScale 0.8
    g0 FilledOff
    g0 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor g0
vtkGlyphSource2D g1
    g1 SetGlyphTypeToDash
    g1 SetCenter 1 0 0
    g1 SetScale 0.8
    g1 FilledOff
    g1 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor g1
vtkGlyphSource2D g2
    g2 SetGlyphTypeToCross
    g2 SetCenter 2 0 0
    g2 SetScale 0.8
    g2 FilledOff
    g2 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor g2
vtkGlyphSource2D g3
    g3 SetGlyphTypeToThickCross
    g3 SetCenter 3 0 0
    g3 SetScale 0.8
    g3 FilledOff
    g3 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor g3
vtkGlyphSource2D g4
    g4 SetGlyphTypeToTriangle
    g4 SetCenter 4 0 0
    g4 SetScale 0.8
    g4 FilledOff
    g4 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor g4
vtkGlyphSource2D g5
    g5 SetGlyphTypeToSquare
    g5 SetCenter 5 0 0
    g5 SetScale 0.8
    g5 FilledOff
    g5 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor g5
vtkGlyphSource2D g6
    g6 SetGlyphTypeToCircle
    g6 SetCenter 6 0 0
    g6 SetScale 0.8
    g6 FilledOff
    g6 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor g6
vtkGlyphSource2D g7
    g7 SetGlyphTypeToDiamond
    g7 SetCenter 7 0 0
    g7 SetScale 0.8
    g7 FilledOff
    g7 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor g7
vtkGlyphSource2D g8
    g8 SetGlyphTypeToArrow
    g8 SetCenter 8 0 0
    g8 SetScale 0.8
    g8 FilledOff
    g8 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor g8
vtkGlyphSource2D g9
    g9 SetGlyphTypeToThickArrow
    g9 SetCenter 9 0 0
    g9 SetScale 0.8
    g9 FilledOff
    g9 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor g9
vtkGlyphSource2D g10
    g10 SetGlyphTypeToHookedArrow
    g10 SetCenter 10 0 0
    g10 SetScale 0.8
    g10 FilledOff
    g10 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor g10
#filled glyphs
vtkGlyphSource2D gf0
    gf0 SetGlyphTypeToNone
    gf0 SetCenter 0 1 0
    gf0 SetScale 0.8
    gf0 FilledOn
    gf0 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gf0
vtkGlyphSource2D gf1
    gf1 SetGlyphTypeToDash
    gf1 SetCenter 1 1 0
    gf1 SetScale 0.8
    gf1 FilledOn
    gf1 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gf1
vtkGlyphSource2D gf2
    gf2 SetGlyphTypeToCross
    gf2 SetCenter 2 1 0
    gf2 SetScale 0.8
    gf2 FilledOn
    gf2 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gf2
vtkGlyphSource2D gf3
    gf3 SetGlyphTypeToThickCross
    gf3 SetCenter 3 1 0
    gf3 SetScale 0.8
    gf3 FilledOn
    gf3 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gf3
vtkGlyphSource2D gf4
    gf4 SetGlyphTypeToTriangle
    gf4 SetCenter 4 1 0
    gf4 SetScale 0.8
    gf4 FilledOn
    gf4 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gf4
vtkGlyphSource2D gf5
    gf5 SetGlyphTypeToSquare
    gf5 SetCenter 5 1 0
    gf5 SetScale 0.8
    gf5 FilledOn
    gf5 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gf5
vtkGlyphSource2D gf6
    gf6 SetGlyphTypeToCircle
    gf6 SetCenter 6 1 0
    gf6 SetScale 0.8
    gf6 FilledOn
    gf6 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gf6
vtkGlyphSource2D gf7
    gf7 SetGlyphTypeToDiamond
    gf7 SetCenter 7 1 0
    gf7 SetScale 0.8
    gf7 FilledOn
    gf7 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gf7
vtkGlyphSource2D gf8
    gf8 SetGlyphTypeToArrow
    gf8 SetCenter 8 1 0
    gf8 SetScale 0.8
    gf8 FilledOn
    gf8 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gf8
vtkGlyphSource2D gf9
    gf9 SetGlyphTypeToThickArrow
    gf9 SetCenter 9 1 0
    gf9 SetScale 0.8
    gf9 FilledOn
    gf9 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gf9
vtkGlyphSource2D gf10
    gf10 SetGlyphTypeToHookedArrow
    gf10 SetCenter 10 1 0
    gf10 SetScale 0.8
    gf10 FilledOn
    gf10 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gf10
#rotated glyphs
vtkGlyphSource2D gr0
    gr0 SetGlyphTypeToNone
    gr0 SetCenter 0 2 0
    gr0 SetScale 0.8
    gr0 FilledOn
    gr0 SetRotationAngle 45
    gr0 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gr0
vtkGlyphSource2D gr1
    gr1 SetGlyphTypeToDash
    gr1 SetCenter 1 2 0
    gr1 SetScale 0.8
    gr1 FilledOn
    gr1 SetRotationAngle 45
    gr1 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gr1
vtkGlyphSource2D gr2
    gr2 SetGlyphTypeToCross
    gr2 SetCenter 2 2 0
    gr2 SetScale 0.8
    gr2 FilledOn
    gr2 SetRotationAngle 45
    gr2 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gr2
vtkGlyphSource2D gr3
    gr3 SetGlyphTypeToThickCross
    gr3 SetCenter 3 2 0
    gr3 SetScale 0.8
    gr3 FilledOn
    gr3 SetRotationAngle 45
    gr3 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gr3
vtkGlyphSource2D gr4
    gr4 SetGlyphTypeToTriangle
    gr4 SetCenter 4 2 0
    gr4 SetScale 0.8
    gr4 FilledOn
    gr4 SetRotationAngle 45
    gr4 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gr4
vtkGlyphSource2D gr5
    gr5 SetGlyphTypeToSquare
    gr5 SetCenter 5 2 0
    gr5 SetScale 0.8
    gr5 FilledOn
    gr5 SetRotationAngle 45
    gr5 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gr5
vtkGlyphSource2D gr6
    gr6 SetGlyphTypeToCircle
    gr6 SetCenter 6 2 0
    gr6 SetScale 0.8
    gr6 FilledOn
    gr6 SetRotationAngle 45
    gr6 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gr6
vtkGlyphSource2D gr7
    gr7 SetGlyphTypeToDiamond
    gr7 SetCenter 7 2 0
    gr7 SetScale 0.8
    gr7 FilledOn
    gr7 SetRotationAngle 45
    gr7 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gr7
vtkGlyphSource2D gr8
    gr8 SetGlyphTypeToArrow
    gr8 SetCenter 8 2 0
    gr8 SetScale 0.8
    gr8 FilledOn
    gr8 SetRotationAngle 45
    gr8 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gr8
vtkGlyphSource2D gr9
    gr9 SetGlyphTypeToThickArrow
    gr9 SetCenter 9 2 0
    gr9 SetScale 0.8
    gr9 FilledOn
    gr9 SetRotationAngle 45
    gr9 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gr9
vtkGlyphSource2D gr10
    gr10 SetGlyphTypeToHookedArrow
    gr10 SetCenter 10 2 0
    gr10 SetScale 0.8
    gr10 FilledOn
    gr10 SetRotationAngle 45
    gr10 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor gr10
#Dash/Cross combinations
vtkGlyphSource2D p0
    p0 SetGlyphTypeToTriangle
    p0 SetCenter 2 3 0
    p0 SetScale 0.5
    p0 FilledOff
    p0 DashOn
    p0 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor p0
vtkGlyphSource2D p1
    p1 SetGlyphTypeToSquare
    p1 SetCenter 3 3 0
    p1 SetScale 0.5
    p1 FilledOff
    p1 DashOn
    p1 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor p1
vtkGlyphSource2D p2
    p2 SetGlyphTypeToCircle
    p2 SetCenter 4 3 0
    p2 SetScale 0.5
    p2 FilledOff
    p2 DashOn
    p2 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor p2
vtkGlyphSource2D p3
    p3 SetGlyphTypeToDiamond
    p3 SetCenter 5 3 0
    p3 SetScale 0.5
    p3 FilledOff
    p3 DashOn
    p3 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor p3
vtkGlyphSource2D p4
    p4 SetGlyphTypeToTriangle
    p4 SetCenter 6 3 0
    p4 SetScale 0.5
    p4 FilledOff
    p4 CrossOn
    p4 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor p4
vtkGlyphSource2D p5
    p5 SetGlyphTypeToSquare
    p5 SetCenter 7 3 0
    p5 SetScale 0.5
    p5 FilledOff
    p5 CrossOn
    p5 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor p5
vtkGlyphSource2D p6
    p6 SetGlyphTypeToCircle
    p6 SetCenter 8 3 0
    p6 SetScale 0.5
    p6 FilledOff
    p6 CrossOn
    p6 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor p6
vtkGlyphSource2D p7
    p7 SetGlyphTypeToDiamond
    p7 SetCenter 9 3 0
    p7 SetScale 0.5
    p7 FilledOff
    p7 CrossOn
    p7 SetColor [math Random 0 1] [math Random 0 1] [math Random 0 1]
MakeActor p7

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 3
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


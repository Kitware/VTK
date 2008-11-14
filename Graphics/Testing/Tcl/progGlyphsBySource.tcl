package require vtk
package require vtkinteraction

set res 6
vtkPlaneSource plane
    plane SetResolution $res $res
vtkElevationFilter colors
    colors SetInputConnection [plane GetOutputPort]
    colors SetLowPoint -0.25 -0.25 -0.25
    colors SetHighPoint 0.25 0.25 0.25
vtkPolyDataMapper planeMapper
    planeMapper SetInput [colors GetPolyDataOutput]
vtkActor planeActor
    planeActor SetMapper planeMapper
    [planeActor GetProperty] SetRepresentationToWireframe

# create simple poly data so we can apply glyph
vtkSuperquadricSource squad

vtkElevationFilter squadColors
    squadColors SetInputConnection [squad GetOutputPort]
    squadColors SetLowPoint -0.25 -0.25 -0.25
    squadColors SetHighPoint 0.25 0.25 0.25
vtkCastToConcrete squadCaster
  squadCaster SetInputConnection [squadColors GetOutputPort]
vtkTransform squadTransform
vtkTransformPolyDataFilter transformSquad
  transformSquad SetInput [squadColors GetPolyDataOutput]
  transformSquad SetTransform squadTransform

# procedure for generating glyphs
proc Glyph {} {
   global res
   set ptId [glypher GetPointId]
   set pd [glypher GetPointData]
   set xyz [glypher GetPoint]
   set x [lindex $xyz 0] 
   set y [lindex $xyz 1] 
   set length [[glypher GetInput 0] GetLength]
   set scale [expr $length / (2.0*$res)]

   squadTransform Identity
    if { $x == $y } {
	squad ToroidalOn
        eval squadTransform Translate $xyz
        squadTransform RotateX 90
    } else {
        eval squadTransform Translate $xyz
	squad ToroidalOff
    }
   squadTransform Scale $scale $scale $scale

   squad SetPhiRoundness [expr abs($x)*5.0]
   squad SetThetaRoundness [expr abs($y)*5.0]
}

vtkProgrammableGlyphFilter glypher
    glypher SetInputConnection [colors GetOutputPort]
    glypher SetSource [transformSquad GetOutput]
    glypher SetGlyphMethod Glyph
    glypher SetColorModeToColorBySource

vtkPolyDataMapper glyphMapper
    glyphMapper SetInputConnection [glypher GetOutputPort]
vtkActor glyphActor
    glyphActor SetMapper glyphMapper



# Create the rendering stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin SetMultiSamples 0
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor planeActor
ren1 AddActor glyphActor
ren1 SetBackground 1 1 1
renWin SetSize 450 450
renWin Render
[ren1 GetActiveCamera] Zoom 1.3

# Get handles to some useful objects
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and interactive renderer
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPoints points
  points SetNumberOfPoints 0
  points InsertNextPoint 1 0 0
  points InsertNextPoint 1 3 0
  points InsertNextPoint 0 3 0
  points InsertNextPoint -1 3 0
  points InsertNextPoint -1 0 0
  points InsertNextPoint -1 -3 0
  points InsertNextPoint 0 -3 0
  points InsertNextPoint 1 -3 0



vtkCellArray strips
  strips Reset

set c [points GetNumberOfPoints]
strips InsertNextCell $c
for { set i 0 } { $i < $c } { incr i } {
  strips InsertCellPoint $i
}

vtkPolyData profile
  profile SetPoints points
  profile SetLines strips

vtkGlyphSource2D gs
  gs SetGlyphTypeToCircle
  gs FilledOff
  gs CrossOn

vtkGlyph2D glyph
  glyph SetInput profile
  glyph SetSource 0 [gs GetOutput]

vtkPolyDataMapper map
  map SetInput [glyph GetOutput]

vtkActor strip
    strip SetMapper map
    [strip GetProperty] SetColor 0.3800 0.7000 0.1600
    [strip GetProperty] BackfaceCullingOff


vtkPolyLine polyline

set Coefficients [polyline FitEllipse points 0 1]
for { set i 0 } { $i < [llength $Coefficients] } { incr i } {
  set a$i [lindex $Coefficients $i]
}

vtkQuadric ellipse
  ellipse SetCoefficients $a0 $a2 0.0 $a1 0.0 0.0 $a3 $a4 0.0 $a5

vtkSampleFunction sample
sample SetImplicitFunction ellipse
sample SetSampleDimensions 200 200 1
sample SetModelBounds 10 -10 10 -10 -1 1

vtkContourFilter iso
    iso SetInput [sample GetOutput]
    iso SetValue 0 0.0
vtkPolyDataMapper isoMapper
    isoMapper SetInput [iso GetOutput]
    isoMapper ScalarVisibilityOff
vtkActor isoActor
    isoActor SetMapper isoMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor strip
ren1 AddActor isoActor
ren1 SetBackground 1 1 1
renWin SetSize 400 400
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


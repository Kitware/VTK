catch {load vtktcl}

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# create pipeline
vtkConeSource cone
  cone SetResolution 20
  cone Update

vtkPolyDataNormals normals
  normals SetInput [cone GetOutput]
  normals SetFeatureAngle 15

vtkTriangleFilter tris
  tris SetInput [normals GetOutput]

vtkMath rn
vtkUnsignedCharArray cellColors
  cellColors SetNumberOfComponents 3
  cellColors SetNumberOfTuples [[cone GetOutput] GetNumberOfCells]

for { set i 0 } { $i < [cellColors GetNumberOfTuples]} { incr i } {
    cellColors InsertComponent $i 0 [rn Random 100 255]
    cellColors InsertComponent $i 1 [rn Random 100 255]
    cellColors InsertComponent $i 2 [rn Random 100 255]
}

vtkScalars cellScalars
  cellScalars SetData cellColors

[[cone GetOutput] GetCellData] SetScalars cellScalars

# mapper and actor
vtkPolyDataMapper mapper
    mapper SetInput [normals GetOutput]

vtkActor actor
    actor SetMapper mapper

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor actor
ren1 SetBackground .1 .2 .4

renWin SetSize 300 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Roll 90
[ren1 GetActiveCamera] Elevation 30
[ren1 GetActiveCamera] Zoom 1.2
renWin Render
renWin SetFileName "cellDataPolyDataNormals.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

catch {load vtktcl}

# include get the vtk interactor ui
source ../../../vtk/examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a tetrahedron
#
vtkPoints tetraPoints
  tetraPoints SetNumberOfPoints 4
  tetraPoints InsertPoint 0 0        0         1.73205
  tetraPoints InsertPoint 1 0        1.63299   -0.57735
  tetraPoints InsertPoint 2 -1.41421 -0.816497 -0.57735
  tetraPoints InsertPoint 3 1.41421  -0.816497 -0.57735

vtkTetra aTetra
  [aTetra GetPointIds] SetId 0 0
  [aTetra GetPointIds] SetId 1 1
  [aTetra GetPointIds] SetId 2 2
  [aTetra GetPointIds] SetId 3 3

vtkUnstructuredGrid aTetraGrid
  aTetraGrid Allocate 1 1
  aTetraGrid InsertNextCell [aTetra GetCellType] [aTetra GetPointIds]
  aTetraGrid SetPoints tetraPoints

vtkLookupTable lut
  lut SetNumberOfColors 3
  lut Build
  lut SetTableValue 0 0 0 0 0
  lut SetTableValue 1 1 .3 .3 1
  lut SetTableValue 2 .8 .8 .9 1
  lut SetTableRange 0 2

vtkGeometryFilter tris
  tris SetInput aTetraGrid

vtkLoopSubdivisionFilter loopS
  loopS SetInput [tris GetOutput]
  loopS SetNumberOfSubdivisions 4

vtkPolyDataNormals normals
  normals SetInput [loopS GetOutput]

vtkDataSetMapper mapper
    mapper SetInput [normals GetOutput]
    mapper SetScalarModeToUseCellData
    mapper SetLookupTable lut
    mapper SetScalarRange 0 2

vtkActor tetraActor
    tetraActor SetMapper mapper
[tetraActor GetProperty] SetDiffuse .8
[tetraActor GetProperty] SetSpecular .4
[tetraActor GetProperty] SetSpecularPower 20
[tetraActor GetProperty] SetDiffuseColor 1 .6 .3

# Add the actors to the renderer, set the background and size
#
ren1 AddActor tetraActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize
renWin SetFileName "subdivideTetra.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



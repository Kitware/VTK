catch {load vtktcl}
# Append datasets
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow etc.
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a 2*2 cell/3*3 pt structuredgrid
vtkPoints points
  points InsertNextPoint    -1  1  0   
  points InsertNextPoint     0  1  0
  points InsertNextPoint     1  1  0
  points InsertNextPoint    -1  0  0
  points InsertNextPoint     0  0  0
  points InsertNextPoint     1  0  0
  points InsertNextPoint    -1 -1  0 
  points InsertNextPoint     0 -1  0
  points InsertNextPoint     1 -1  0

vtkScalars faceColors
  faceColors InsertNextScalar 0
  faceColors InsertNextScalar 1
  faceColors InsertNextScalar 1
  faceColors InsertNextScalar 2

vtkStructuredGrid grid
  grid SetDimensions 3 3 1
  grid SetPoints points
  [grid GetCellData] SetScalars faceColors

# create a 1*2 cell grid
vtkPoints points2
  points2 InsertNextPoint     1  1  0   
  points2 InsertNextPoint     2  1  0
  points2 InsertNextPoint     1  0  0
  points2 InsertNextPoint     2  0  0
  points2 InsertNextPoint     1 -1  0 
  points2 InsertNextPoint     2 -1  0

vtkScalars faceColors2
  faceColors2 InsertNextScalar 2
  faceColors2 InsertNextScalar 0

vtkStructuredGrid grid2
  grid2 SetDimensions 2 3 1
  grid2 SetPoints points2
  [grid2 GetCellData] SetScalars faceColors2

vtkAppendFilter Append
  Append AddInput grid
  Append AddInput grid2

vtkDataSetMapper mapper
  mapper SetInput [Append GetOutput]
  mapper SetScalarModeToUseCellData
  mapper SetScalarRange 0 2

vtkActor Checker
  Checker SetMapper mapper

# Add the actors to the renderer, set the background and size
ren1 AddActor Checker
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 255 255

# render the image
renWin Render
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize
renWin SetFileName appendCellData.tcl.ppm
#renWin SaveImageAsPPM

wm withdraw .

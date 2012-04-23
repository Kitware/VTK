package require vtk
package require vtkinteraction


# cell scalars to point scalars
# get the interactor ui

# Create the RenderWindow, Renderer and RenderWindowInteractor
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

vtkFloatArray faceColors
  faceColors InsertNextValue 0
  faceColors InsertNextValue 1
  faceColors InsertNextValue 1
  faceColors InsertNextValue 2

vtkStructuredGrid sgrid
  sgrid SetDimensions 3 3 1
  sgrid SetPoints points
  [sgrid GetCellData] SetScalars faceColors

vtkCellDataToPointData Cell2Point
  Cell2Point SetInputData sgrid
  Cell2Point PassCellDataOn
  Cell2Point Update

vtkDataSetMapper mapper
  mapper SetInputData [Cell2Point GetStructuredGridOutput]
  mapper SetScalarModeToUsePointData
  mapper SetScalarRange 0 2

vtkActor actor
    actor SetMapper mapper

# Add the actors to the renderer, set the background and size
ren1 AddActor actor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 256 256

# render the image
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .


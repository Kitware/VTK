package require vtk
package require vtkinteraction
package require vtktesting

### SetUp the pipeline

vtkRectilinearGridToTetrahedra FormMesh
  FormMesh SetInput 4 2 2 1 1 1 0.001
  FormMesh RememberVoxelIdOn
vtkExtractEdges TetraEdges
  TetraEdges SetInput [FormMesh GetOutput]
vtkTubeFilter tubes
  tubes SetInput [TetraEdges GetOutput]
  tubes SetRadius 0.05
  tubes SetNumberOfSides 6

### Run the pipeline 3 times, with different conversions to TetMesh

vtkPolyData Tubes1
  FormMesh SetTetraPerCellTo5
  tubes Update
  Tubes1 DeepCopy [tubes GetOutput]
vtkPolyData Tubes2
  FormMesh SetTetraPerCellTo6
  tubes Update
  Tubes2 DeepCopy [tubes GetOutput]
vtkPolyData Tubes3
  FormMesh SetTetraPerCellTo12
  tubes Update
  Tubes3 DeepCopy [tubes GetOutput]

### Run the pipeline once more, this time converting some cells to
### 5 and some data to 12 TetMesh

### Determine which cells are which
vtkIntArray DivTypes
  set numCell [[FormMesh GetInput] GetNumberOfCells]
  DivTypes SetNumberOfValues $numCell
  for {set i 0} {$i<$numCell} {incr i 1} {
    DivTypes SetValue $i [expr 5+[expr 7*[expr $i % 4]]]
  }

### Finish this pipeline
vtkPolyData Tubes4
  FormMesh SetTetraPerCellTo5And12
  [[FormMesh GetInput] GetCellData] SetScalars DivTypes
  tubes Update
  Tubes4 DeepCopy [tubes GetOutput]

### Finish the 4 pipelines
for {set i 1} {$i<5} {incr i 1} {
  vtkPolyDataMapper mapEdges$i
      mapEdges$i SetInput Tubes$i 
  vtkActor edgeActor$i
    edgeActor$i SetMapper mapEdges$i
    eval [edgeActor$i GetProperty] SetColor $peacock
    [edgeActor$i GetProperty] SetSpecularColor 1 1 1
    [edgeActor$i GetProperty] SetSpecular 0.3
    [edgeActor$i GetProperty] SetSpecularPower 20
    [edgeActor$i GetProperty] SetAmbient 0.2
    [edgeActor$i GetProperty] SetDiffuse 0.8
  vtkRenderer ren$i
    ren$i AddActor edgeActor$i
    ren$i SetBackground 0 0 0
    [ren$i GetActiveCamera] Zoom 1
    [ren$i GetActiveCamera] SetPosition 1.73906 12.7987 -0.257808
    [ren$i GetActiveCamera] SetViewUp 0.992444 0.00890284 -0.122379
    [ren$i GetActiveCamera] SetClippingRange 9.36398 15.0496
}

# Create graphics objects
# Create the rendering window, renderer, and interactive renderer
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin AddRenderer ren2
    renWin AddRenderer ren3
    renWin AddRenderer ren4
    renWin SetSize 600 300
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size

ren1 SetViewport .75 0 1 1
ren2 SetViewport .50 0 .75 1
ren3 SetViewport .25 0 .50 1
ren4 SetViewport 0 0 .25 1

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

package require vtk
package require vtkinteraction

# read in a Chaco file
vtkChacoReader chReader
  chReader SetBaseName "$VTK_DATA_ROOT/Data/vwgt"
  chReader SetGenerateGlobalElementIdArray 1
  chReader SetGenerateGlobalNodeIdArray 1
  chReader SetGenerateEdgeWeightArrays 1
  chReader SetGenerateVertexWeightArrays 1

vtkGeometryFilter geom
  geom SetInputConnection [chReader GetOutputPort]

vtkPolyDataMapper mapper
  mapper SetInputConnection [geom GetOutputPort]
  mapper SetColorModeToMapScalars
  mapper SetScalarModeToUsePointFieldData
  mapper SelectColorArray "VertexWeight1"
  mapper SetScalarRange 1 5

vtkActor actor0
  actor0 SetMapper mapper

# Create the RenderWindow, Renderer and interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actor to the renderer, set the background and size
#
ren1 AddActor actor0
ren1 SetBackground 0 0 0

renWin SetSize 300 300
iren Initialize
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

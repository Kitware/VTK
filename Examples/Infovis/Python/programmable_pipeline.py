from vtk import *

source = vtkRandomGraphSource()
source.SetNumberOfVertices(75)
source.SetEdgeProbability(0.02)
source.SetUseEdgeProbability(True)
source.SetStartWithTree(True)

create_index = vtkProgrammableFilter()
create_index.AddInputConnection(source.GetOutputPort())

def create_index_callback():
  input = create_index.GetInput()
  output = create_index.GetOutput()

  output.ShallowCopy(input)

  vertex_id_array = vtkIdTypeArray()
  vertex_id_array.SetName("vertex_id")
  vertex_id_array.SetNumberOfTuples(output.GetNumberOfVertices())
  for i in range(output.GetNumberOfVertices()):
    vertex_id_array.SetValue(i, i)
  output.GetVertexData().AddArray(vertex_id_array)

  edge_target_array = vtkIdTypeArray()
  edge_target_array.SetName("edge_target")
  edge_target_array.SetNumberOfTuples(output.GetNumberOfEdges())
  edge_iterator = vtkEdgeListIterator()
  output.GetEdges(edge_iterator)
  while edge_iterator.HasNext():
    edge = edge_iterator.NextGraphEdge()
    edge_target_array.SetValue(edge.GetId(), edge.GetTarget())
  output.GetEdgeData().AddArray(edge_target_array)

create_index.SetExecuteMethod(create_index_callback)

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(create_index.GetOutputPort())
view.SetVertexLabelArrayName("vertex_id")
view.SetVertexLabelVisibility(True)
view.SetEdgeLabelArrayName("edge_target")
view.SetEdgeLabelVisibility(True)

theme = vtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
theme.FastDelete()

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)

window.GetInteractor().Start()
                      

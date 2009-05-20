from vtk import *

source = vtkRandomGraphSource()
source.SetNumberOfVertices(10)
source.SetEdgeProbability(0.1)
source.SetUseEdgeProbability(True)
source.SetStartWithTree(True)
source.SetIncludeEdgeWeights(True)

create_index = vtkProgrammableFilter()
create_index.SetInputConnection(source.GetOutputPort())

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

create_index.SetExecuteMethod(create_index_callback)

selection = vtkSelectionSource()
selection.SetContentType(7) # vtkSelection::THRESHOLDS
selection.SetFieldType(4) # vtkSelection::EDGE
selection.SetArrayName("weight")
selection.AddThreshold(0.8, 1.0)

create_index.Update()
selection.Update()
index_selection = vtkConvertSelection.ToIndexSelection(selection.GetOutput(), create_index.GetOutput())

print index_selection

subgraph = vtkExtractSelectedGraph()
subgraph.SetRemoveIsolatedVertices(False)
subgraph.SetInputConnection(create_index.GetOutputPort())
subgraph.SetSelectionConnection(selection.GetOutputPort())

view1 = vtkGraphLayoutView()
view1.AddRepresentationFromInputConnection(create_index.GetOutputPort())
view1.SetVertexLabelArrayName("vertex_id")
view1.SetVertexLabelVisibility(True)
view1.SetEdgeLabelArrayName("weight")
view1.SetEdgeLabelVisibility(True)

view2 = vtkGraphLayoutView()
view2.AddRepresentationFromInputConnection(subgraph.GetOutputPort())
view2.SetVertexLabelArrayName("vertex_id")
view2.SetVertexLabelVisibility(True)
view2.SetEdgeLabelArrayName("weight")
view2.SetEdgeLabelVisibility(True)

theme = vtkViewTheme.CreateMellowTheme()
view1.ApplyViewTheme(theme)
view2.ApplyViewTheme(theme)
theme.FastDelete()

view1.GetRenderWindow().SetSize(600, 600)
view1.ResetCamera()
view1.Render()

view2.GetRenderWindow().SetSize(600, 600)
view2.ResetCamera()
view2.Render()

view1.GetInteractor().Start()
                      

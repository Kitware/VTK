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
  index_array = vtkIntArray()
  index_array.SetName("index")
  index_array.SetNumberOfTuples(output.GetNumberOfVertices())
  for i in range(output.GetNumberOfVertices()):
    index_array.SetValue(i, i)
  output.GetVertexData().AddArray(index_array)

create_index.SetExecuteMethod(create_index_callback)

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(create_index.GetOutputPort())
view.SetVertexLabelArrayName("index")
view.SetVertexLabelVisibility(True)

theme = vtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
theme.FastDelete()

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)

window.GetInteractor().Start()
                      

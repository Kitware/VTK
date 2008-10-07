from vtk import *

data_dir = "../../../../VTKData/Data/Infovis/Images/"

source = vtkGeoRandomGraphSource()
source.DirectedOff()
source.SetNumberOfVertices(50)
source.SetEdgeProbability(0.01)
source.SetUseEdgeProbability(True)
source.AllowParallelEdgesOn()
source.AllowSelfLoopsOn() 
source.SetStartWithTree(True)
source.Update()
print source.GetOutput()

# Create a 3D geospatial view
view = vtkGeoView()
window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)
#view.GetRenderer().ResetCamera()

# Create the background image
view.AddDefaultImageRepresentation(data_dir + "NE2_ps_bath.jpg")

# Create graph
graph_rep = vtkGeoGraphRepresentation()
graph_rep.SetInputConnection(source.GetOutputPort())
graph_rep.SetVertexColorArrayName("vertex id")
graph_rep.ColorVerticesOn()
graph_rep.SetVertexColorArrayName("edge id")
graph_rep.ColorEdgesOn()
graph_rep.SetVertexLabelArrayName("vertex id")
graph_rep.VertexLabelVisibilityOn()
view.AddRepresentation(graph_rep)

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
theme.SetCellOpacity(0.1)
theme.SetCellColor(0, 0, 0)
view.ApplyViewTheme(theme)
graph_rep.ApplyViewTheme(theme)
theme.FastDelete()
view.Update()

window.GetInteractor().Initialize()
window.GetInteractor().Start()


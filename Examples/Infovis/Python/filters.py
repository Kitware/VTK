from vtk import *

source = vtkRandomGraphSource()
source.SetIncludeEdgeWeights(True)

bfs = vtkBoostBreadthFirstSearch()
bfs.AddInputConnection(source.GetOutputPort())
bfs.SetOriginVertex(0)

degree = vtkVertexDegree()
degree.AddInputConnection(bfs.GetOutputPort())

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(degree.GetOutputPort())
view.SetVertexLabelArrayName("BFS")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("VertexDegree")
view.SetColorVertices(True)
view.SetEdgeLabelArrayName("edge_weights")
view.SetEdgeLabelVisibility(True)
view.SetEdgeColorArrayName("edge_weights")
view.SetColorEdges(True)

theme = vtkViewTheme.CreateMellowTheme()
theme.SetCellHueRange(0.7, 0.7)
theme.SetCellSaturationRange(0, 1)
theme.SetCellOpacity(1)
view.ApplyViewTheme(theme)
theme.FastDelete()

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)
window.GetInteractor().Start()


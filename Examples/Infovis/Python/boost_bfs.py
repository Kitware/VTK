from vtk import *

source = vtkRandomGraphSource()
source.SetIncludeEdgeWeights(True)

bfs = vtkBoostBreadthFirstSearch()
bfs.AddInputConnection(source.GetOutputPort())
bfs.SetOriginVertex(0)

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(bfs.GetOutputPort())
view.SetVertexLabelArrayName("BFS")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("BFS")
view.SetColorVertices(True)
view.SetEdgeLabelArrayName("edge weight")
view.SetEdgeLabelVisibility(True)
view.SetEdgeColorArrayName("edge weight")
view.SetColorEdges(True)
view.SetLayoutStrategyToSimple2D()

theme = vtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(5)
theme.SetPointSize(10)
view.ApplyViewTheme(theme)
theme.FastDelete()
view.SetVertexLabelFontSize(20)
view.SetEdgeLabelFontSize(12)

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()

view.GetInteractor().Start()


from vtk import *

source = vtkRandomGraphSource()
source.SetIncludeEdgeWeights(True)

degree = vtkVertexDegree()
degree.AddInputConnection(source.GetOutputPort())

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(degree.GetOutputPort())
view.SetVertexLabelArrayName("VertexDegree")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("VertexDegree")
view.SetColorVertices(True)
view.SetEdgeLabelArrayName("edge weight")
view.SetEdgeLabelVisibility(True)
view.SetEdgeColorArrayName("edge weight")
view.SetColorEdges(True)
view.SetLayoutStrategyToSimple2D()

theme = vtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(4)
view.ApplyViewTheme(theme)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()

view.GetInteractor().Start()


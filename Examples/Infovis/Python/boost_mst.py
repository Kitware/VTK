#!/usr/bin/env python
from vtk import *

source = vtkRandomGraphSource()
source.DirectedOff()
source.SetNumberOfVertices(10)
source.SetEdgeProbability(0.22)
source.SetUseEdgeProbability(True)
source.AllowParallelEdgesOn()
source.SetStartWithTree(True)

# Connect to the centrality filter.
centrality = vtkBoostBrandesCentrality ()
centrality.SetInputConnection(source.GetOutputPort())

# Find the minimal spanning tree
mstTreeSelection = vtkBoostKruskalMinimumSpanningTree()
mstTreeSelection.SetInputConnection(centrality.GetOutputPort())
mstTreeSelection.SetEdgeWeightArrayName("centrality")
mstTreeSelection.NegateEdgeWeightsOn()
mstTreeSelection.Update()

# Create a graph layout view
view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(centrality.GetOutputPort())
view.SetVertexLabelArrayName("centrality")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("centrality")
view.SetColorVertices(True)
view.SetEdgeLabelArrayName("centrality")
view.SetEdgeLabelVisibility(True)
view.SetEdgeColorArrayName("centrality")
view.SetColorEdges(True)
view.SetLayoutStrategyToSimple2D()
view.SetVertexLabelFontSize(14)
view.SetEdgeLabelFontSize(12)

# Make sure the representation is using a pedigree id selection
view.GetRepresentation(0).SetSelectionType(2)

# Set the selection to be the MST
view.GetRepresentation(0).GetAnnotationLink().SetCurrentSelection(mstTreeSelection.GetOutput())

# Set the theme on the view
theme = vtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(5)
theme.SetPointSize(10)
theme.SetCellOpacity(1)
theme.SetSelectedCellColor(1,0,1)
view.ApplyViewTheme(theme)
theme.FastDelete()


view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()

view.GetInteractor().Start()


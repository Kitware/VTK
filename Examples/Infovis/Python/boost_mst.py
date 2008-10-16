from vtk import *

source = vtkRandomGraphSource()
source.DirectedOff()
source.SetNumberOfVertices(50)
source.SetEdgeProbability(0.01)
source.SetUseEdgeProbability(True)
source.AllowParallelEdgesOn()
source.AllowSelfLoopsOn() 
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
view.SetEdgeColorArrayName("centrality")
view.SetColorEdges(True)
view.SetLayoutStrategyToSimple2D()

# Make sure the view is using a pedigree id selection
view.SetSelectionType(2)

# Set the selection to be the MST
view.GetRepresentation(0).GetSelectionLink().SetSelection(mstTreeSelection.GetOutput())

# Set the theme on the view
theme = vtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(5)
theme.SetCellOpacity(0.99)
theme.SetCellAlphaRange(0.5,0.5)
theme.SetPointSize(10)
theme.SetSelectedCellColor(1,0,1)
theme.SetSelectedPointColor(1,0,1)
view.ApplyViewTheme(theme)
view.SetVertexLabelFontSize(20)
view.SetEdgeLabelFontSize(18)

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)
view.GetRenderer().ResetCamera()

window.GetInteractor().Start()


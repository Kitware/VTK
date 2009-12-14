
# Python examples script that uses the R calculator filter to find the
# maxiumum spanning tree of a random input graph by inverting the edge
# weights of the graph in R.  The MST algorithm then finds the maximum
# spanning tree instead of the minimum spanning tree.

# VTK must be built with VTK_USE_GNU_R turned on for this example to work!

from vtk import *

if __name__ == "__main__":
    
  # Generate a random graph with 20 vertices and a random number of edges
  source = vtkRandomGraphSource()
  source.SetNumberOfVertices(20)
  source.SetEdgeProbability(0.2)
  source.SetUseEdgeProbability(True)
  source.SetStartWithTree(True)
  source.IncludeEdgeWeightsOn()
  source.AllowParallelEdgesOn()

  # Create RCalculatorFilter for interaction with R
  rcalculator = vtkRCalculatorFilter()
  rcalculator.SetInputConnection(source.GetOutputPort())
  # Display R output on the terminal
  rcalculator.SetRoutput(1)
  # Copy edge weight array to R as variable ew
  rcalculator.PutArray("edge weight","ew")
  # Invert the edge weight data.
  rcalculator.SetRscript("ew = 1.0/ew\n")
  # Copy edge weight array back to VTK.
  rcalculator.GetArray("edge weight","ew")

  # Find the minimal spanning tree (will be maximal spanning tree)
  mstTreeSelection = vtkBoostKruskalMinimumSpanningTree()
  mstTreeSelection.SetInputConnection(rcalculator.GetOutputPort())
  mstTreeSelection.SetEdgeWeightArrayName("edge weight")
  mstTreeSelection.NegateEdgeWeightsOn()
  mstTreeSelection.Update()

  # Create a graph layout view
  view = vtkGraphLayoutView()
  view.AddRepresentationFromInputConnection(rcalculator.GetOutputPort())
  view.SetColorVertices(True)
  view.SetEdgeLabelArrayName("edge weight")
  view.SetEdgeLabelVisibility(True)
  view.SetEdgeColorArrayName("edge weight")
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


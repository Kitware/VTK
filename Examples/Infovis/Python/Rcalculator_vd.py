
# Python example script that uses the RcalculatorFilter to compute the mean
# vertex degree of the input graph and the distance from the mean for each
# vertex in the entire graph.  The computed result is then used to label
# the displayed graph in VTK.

# VTK must be built with VTK_USE_GNU_R turned on for this example to work!

from vtk import *

if __name__ == "__main__":

  # Generate a random graph with 20 vertices and a random number of edges
  source = vtkRandomGraphSource()
  source.SetNumberOfVertices(20)
  source.SetEdgeProbability(0.1)
  source.SetUseEdgeProbability(True)
  source.SetStartWithTree(True)
  source.IncludeEdgeWeightsOn()
  source.AllowParallelEdgesOn()

  # Connect to the vtkVertexDegree filter to compute vertex degree
  degree_filter = vtkVertexDegree()
  degree_filter.SetOutputArrayName("vertex_degree")
  degree_filter.SetInputConnection(source.GetOutputPort())

  # Pass the vertex degree data to R and compute the distance from the mean
  # vertex degree for every vertex in the graph.
  rcalculator = vtkRCalculatorFilter()
  rcalculator.SetInputConnection(degree_filter.GetOutputPort())
  # Shows R output on the terminal
  rcalculator.SetRoutput(1)
  # Copy vertex_degree array to R as variable vd
  rcalculator.PutArray("vertex_degree","vd")
  # Run R script to perform mean and distance calculation for each vertex
  rcalculator.SetRscript("vd = abs(mean(vd) - vd)\n")
  # Copy new vertex degree array back into the VTK graph
  rcalculator.GetArray("vertex_degree_mean_dist","vd")

  # Create a graph layout view
  view = vtkGraphLayoutView()
  view.AddRepresentationFromInputConnection(rcalculator.GetOutputPort())
  view.SetVertexLabelArrayName("vertex_degree_mean_dist")
  view.SetVertexLabelVisibility(True)
  view.SetVertexColorArrayName("vertex_degree_mean_dist")
  view.SetColorVertices(True)
  view.SetLayoutStrategyToSimple2D()
  view.SetVertexLabelFontSize(14)
  view.SetEdgeLabelFontSize(12)

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


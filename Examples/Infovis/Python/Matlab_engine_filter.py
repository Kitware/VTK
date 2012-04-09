
# Python example script that uses the Matlab Engine filter to compute the
# degree centraility for an input graph and display the result.

# VTK must be built with VTK_USE_MATLAB_MEX turned on for this example to work!

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

  # Create a Matlab Engine Filter and connect it to the VTK pipeline
  mengfilter = vtkMatlabEngineFilter()
  mengfilter.SetInputConnection(degree_filter.GetOutputPort())
  # Show Matlab output on the terminal
  mengfilter.SetEngineOutput(1)
  # Copy vertex_degree array to Matlab as variable vd
  mengfilter.PutArray("vertex_degree","vd")
  # Compute the degree centrality in Matlab
  mengfilter.SetMatlabScript("vd, length(vd), size(vd), l = length(vd), vd = double(vd)/(l - 1)")
  # Copy result back to VTK
  mengfilter.GetArray("vertex_degree_centrality","vd")

  # Create a graph layout view
  view = vtkGraphLayoutView()
  view.AddRepresentationFromInputConnection(mengfilter.GetOutputPort())
  view.SetVertexLabelArrayName("vertex_degree_centrality")
  view.SetVertexLabelVisibility(True)
  view.SetVertexColorArrayName("vertex_degree_centrality")
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


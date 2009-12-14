from vtk import *

source = vtkRandomGraphSource()
source.DirectedOff()
source.SetNumberOfVertices(100)
source.SetEdgeProbability(0) # Basically generates a tree
source.SetUseEdgeProbability(True)
source.SetStartWithTree(True)
source.IncludeEdgeWeightsOn()

# Connect to the Boost centrality filter.
centrality = vtkBoostBrandesCentrality ()
centrality.SetInputConnection(source.GetOutputPort())

# Create force directed layout
forceStrat = vtkSimple2DLayoutStrategy()
forceStrat.SetInitialTemperature(5)

# Create circular layout
fastStrat = vtkFast2DLayoutStrategy()


# Create a graph layout view
view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(centrality.GetOutputPort())
view.SetVertexLabelArrayName("vertex id")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("vertex id")
view.SetColorVertices(True)
view.SetEdgeColorArrayName("edge weight")
view.SetColorEdges(True)
view.SetLayoutStrategy(forceStrat)

# Create a second shrubery!
view2 = vtkGraphLayoutView()
view2.AddRepresentationFromInputConnection(centrality.GetOutputPort())
view2.SetVertexLabelArrayName("vertex id")
view2.SetVertexLabelVisibility(True)
view2.SetVertexColorArrayName("vertex id")
view2.SetColorVertices(True)
view2.SetEdgeColorArrayName("centrality")
view2.SetColorEdges(True)
view2.SetLayoutStrategy(fastStrat)

# Demonstrate value based selection on edges
sel = vtkSelectionSource()
sel.SetContentType(7)               # Thresholds
sel.SetFieldType(4)                 # Edge
sel.SetArrayName("centrality")
sel.AddThreshold(500,5000)          # High centrality edges
sel.Update()

# Take selection and extract a graph
extract_graph = vtkExtractSelectedGraph()
extract_graph.AddInputConnection(centrality.GetOutputPort())
extract_graph.SetSelectionConnection(sel.GetOutputPort())

# Create a view for the extracted graph
view3 = vtkGraphLayoutView()
view3.AddRepresentationFromInputConnection(extract_graph.GetOutputPort())
view3.SetVertexLabelArrayName("vertex id")
view3.SetVertexLabelVisibility(True)
view3.SetVertexColorArrayName("vertex id")
view3.SetColorVertices(True)
view3.SetEdgeColorArrayName("centrality")
view3.SetColorEdges(True)
view3.SetLayoutStrategyToSimple2D()

# Make sure the views are using a pedigree id selection
view.GetRepresentation(0).SetSelectionType(2)
view2.GetRepresentation(0).SetSelectionType(2)
view3.GetRepresentation(0).SetSelectionType(2)

# Create a selection link and set both view to use it
annotationLink = vtkAnnotationLink()
view.GetRepresentation(0).SetAnnotationLink(annotationLink)
view2.GetRepresentation(0).SetAnnotationLink(annotationLink)
view3.GetRepresentation(0).SetAnnotationLink(annotationLink)
annotationLink.SetCurrentSelection(sel.GetOutput())

updater = vtkViewUpdater()
updater.AddAnnotationLink(annotationLink)
updater.AddView(view)
updater.AddView(view2)
updater.AddView(view3)

# Set the theme on the view
theme = vtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(5)
theme.SetPointSize(10)
theme.SetCellOpacity(.99)
theme.SetSelectedCellColor(1,0,1)
theme.SetSelectedPointColor(1,0,1)
view.ApplyViewTheme(theme)
view2.ApplyViewTheme(theme)
view3.ApplyViewTheme(theme)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()

view2.GetRenderWindow().SetSize(600, 600)
view2.ResetCamera()
view2.Render()

view3.GetRenderWindow().SetSize(600, 600)
view3.ResetCamera()
view3.Render()

view.GetInteractor().Start()


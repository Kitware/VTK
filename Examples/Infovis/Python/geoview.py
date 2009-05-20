from vtk import *
import os.path

data_dir = "../../../../VTKData/Data/Infovis/Images/"
if not os.path.exists(data_dir):
  data_dir = "../../../../../VTKData/Data/Infovis/Images/"

source = vtkGeoRandomGraphSource()
source.DirectedOff()
source.SetNumberOfVertices(100)
source.SetEdgeProbability(0.00) # produces a tree
source.SetUseEdgeProbability(True)
source.AllowParallelEdgesOn()
source.AllowSelfLoopsOn() 
source.SetStartWithTree(True)

# Create a 3D geospatial view
view = vtkGeoView()
view.GetRenderWindow().SetSize(600, 600)

# Create the background image
reader = vtkJPEGReader()
reader.SetFileName(data_dir + "NE2_ps_bath.jpg")
reader.Update()
view.AddDefaultImageRepresentation(reader.GetOutput())

# Create graph
graph_rep = vtkRenderedGraphRepresentation()
graph_rep.SetInputConnection(source.GetOutputPort())
graph_rep.SetVertexColorArrayName("vertex id")
graph_rep.ColorVerticesByArrayOn()
graph_rep.SetEdgeColorArrayName("edge id")
graph_rep.ColorEdgesByArrayOn()
graph_rep.SetVertexLabelArrayName("vertex id")
graph_rep.VertexLabelVisibilityOn()
graph_rep.SetLayoutStrategyToAssignCoordinates("longitude", "latitude", None)
strategy = vtkGeoEdgeStrategy()
strategy.SetExplodeFactor(.1)
graph_rep.SetEdgeLayoutStrategy(strategy)
view.AddRepresentation(graph_rep)

# Make a normal graph layout view
view2 = vtkGraphLayoutView()
view2.GetRenderWindow().SetSize(600, 600)
view2.AddRepresentationFromInputConnection(source.GetOutputPort())
view2.SetVertexColorArrayName("vertex id")
view2.ColorVerticesOn()
view2.SetEdgeColorArrayName("edge id")
view2.ColorEdgesOn()
view2.SetVertexLabelArrayName("vertex id")
view2.VertexLabelVisibilityOn()

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(4)
theme.SetPointSize(8)
theme.SetCellSaturationRange(.5,.5)
theme.SetSelectedCellColor(1,0,1)
theme.SetSelectedPointColor(1,0,1)
view.ApplyViewTheme(theme)
graph_rep.ApplyViewTheme(theme)
view2.ApplyViewTheme(theme)
theme.FastDelete()

link = vtkAnnotationLink()
graph_rep.SetAnnotationLink(link)
view2.GetRepresentation(0).SetAnnotationLink(link)

updater = vtkViewUpdater()
updater.AddView(view)
updater.AddView(view2)

view.ResetCamera()
view2.ResetCamera()

view.Render()
view2.Render()

view.GetInteractor().Initialize()
view.GetInteractor().Start()


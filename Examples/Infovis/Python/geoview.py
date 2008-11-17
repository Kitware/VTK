from vtk import *

data_dir = "../../../../VTKData/Data/Infovis/Images/"

source = vtkGeoRandomGraphSource()
source.DirectedOff()
source.SetNumberOfVertices(100)
source.SetEdgeProbability(0.00) # produces a tree
source.SetUseEdgeProbability(True)
source.AllowParallelEdgesOn()
source.AllowSelfLoopsOn() 
source.SetStartWithTree(True)
source.Update()
print source.GetOutput()

# Create a 3D geospatial view
view = vtkGeoView()
window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)

# Create the background image
reader = vtkJPEGReader()
reader.SetFileName(data_dir + "NE2_ps_bath.jpg")
reader.Update()
view.AddDefaultImageRepresentation(reader.GetOutput())

# Create graph
graph_rep = vtkGeoGraphRepresentation()
graph_rep.SetInputConnection(source.GetOutputPort())
graph_rep.SetVertexColorArrayName("vertex id")
graph_rep.ColorVerticesOn()
graph_rep.SetEdgeColorArrayName("edge id")
graph_rep.ColorEdgesOn()
graph_rep.SetVertexLabelArrayName("vertex id")
graph_rep.VertexLabelVisibilityOn()
graph_rep.SetExplodeFactor(.1)
view.AddRepresentation(graph_rep)

# Make a normal graph layout view
view2 = vtkGraphLayoutView()
window2 = vtkRenderWindow()
window2.SetSize(600, 600)
view2.SetupRenderWindow(window2)
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

link = vtkSelectionLink()
graph_rep.SetSelectionLink(link)
view2.GetRepresentation(0).SetSelectionLink(link)

updater = vtkViewUpdater()
updater.AddView(view)
updater.AddView(view2)

view.Update()
view2.GetRenderer().ResetCamera()

window.GetInteractor().Initialize()
window.GetInteractor().Start()


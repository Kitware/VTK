from vtk import *

database = vtkSQLDatabase.CreateFromURL("sqlite://ports_protocols.db")
database.Open("")

edge_query = database.GetQueryInstance()
edge_query.SetQuery("select src, dst from tcp")

vertex_query = database.GetQueryInstance()
vertex_query.SetQuery("select ip, hostname from dnsnames")

edge_table = vtkRowQueryToTable()
edge_table.SetQuery(edge_query)

vertex_table = vtkRowQueryToTable()
vertex_table.SetQuery(vertex_query)

graph = vtkTableToGraph()
graph.AddInputConnection(edge_table.GetOutputPort())
graph.AddLinkVertex("src", "ip", False)
graph.AddLinkVertex("dst", "ip", False)
graph.AddLinkEdge("src", "dst")
graph.SetVertexTableConnection(vertex_table.GetOutputPort())
graph.Update()
print graph.GetOutput()

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(graph.GetOutputPort())
view.SetVertexLabelArrayName("hostname")
view.SetVertexLabelVisibility(True)
#view.SetVertexColorArrayName("hostname")
view.SetColorVertices(True)
view.SetLayoutStrategyToSimple2D()

theme = vtkViewTheme.CreateMellowTheme()
theme.SetCellColor(.2,.2,.6)
theme.SetLineWidth(2)
view.ApplyViewTheme(theme)

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)
view.GetRenderer().ResetCamera()
window.GetInteractor().Start()

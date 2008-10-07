from vtk import *

data_dir = "../../../../VTKData/Data/Infovis/SQLite/"
sqlite_file = data_dir + "ports_protocols.db"
database = vtkSQLDatabase.CreateFromURL("sqlite://" + sqlite_file)
database.Open("")

edge_query = database.GetQueryInstance()
edge_query.SetQuery("select * from tcp")

edge_table = vtkRowQueryToTable()
edge_table.SetQuery(edge_query)

cs = vtkContingencyStatistics()
cs.AddInputConnection(edge_table.GetOutputPort())
contingency_table = cs.GetOutputPort()

vertex_query = database.GetQueryInstance()
vertex_query.SetQuery("select ip, hostname from dnsnames")

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

view.SetEdgeLabelArrayName("protocol")
view.SetEdgeLabelVisibility(True)
view.SetEdgeColorArrayName("port")
view.SetColorEdges(True)

view.SetLayoutStrategyToSimple2D()

theme = vtkViewTheme.CreateMellowTheme()
theme.SetCellColor(.2,.2,.6)
theme.SetLineWidth(2)
view.ApplyViewTheme(theme)

window = vtkRenderWindow()
window.SetSize(1000, 1000)
view.SetupRenderWindow(window)
view.GetRenderer().ResetCamera()
window.GetInteractor().Start()

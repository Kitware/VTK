from vtk import *

data_dir = "../../../../VTKData/Data/Infovis/SQLite/"
sqlite_file = data_dir + "ports_protocols.db"
database = vtkSQLDatabase.CreateFromURL("sqlite://" + sqlite_file)
database.Open("")

edge_query = database.GetQueryInstance()
edge_query.SetQuery("select src, dst, dport, protocol, port_protocol from tcp")

edge_table = vtkRowQueryToTable()
edge_table.SetQuery(edge_query)

cs = vtkContingencyStatistics()
cs.AddInputConnection(edge_table.GetOutputPort())
cs.AddColumnPair("dport","protocol")
cs.SetAssess(1)

vertex_query = database.GetQueryInstance()
vertex_query.SetQuery("select ip, hostname from dnsnames")

vertex_table = vtkRowQueryToTable()
vertex_table.SetQuery(vertex_query)

graph = vtkTableToGraph()
graph.AddInputConnection(cs.GetOutputPort())
graph.AddLinkVertex("src", "ip", False)
graph.AddLinkVertex("dst", "ip", False)
graph.AddLinkEdge("src", "dst")
graph.SetVertexTableConnection(vertex_table.GetOutputPort())
graph.Update()
print graph.GetOutput()


view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(graph.GetOutputPort())

view.SetVertexLabelArrayName("ip")
view.SetVertexLabelVisibility(True)
view.SetEdgeLabelArrayName("port_protocol")
view.SetEdgeLabelVisibility(True)
view.SetEdgeColorArrayName("p(dport | protocol)")
view.SetColorEdges(True)
view.SetLayoutStrategyToSimple2D()

theme = vtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(4)
theme.SetCellHueRange(0,.667)
theme.SetCellAlphaRange(1,1)
theme.SetCellSaturationRange(1,1)
theme.SetCellValueRange(1,1)
view.ApplyViewTheme(theme)

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)
view.GetRenderer().ResetCamera()
window.GetInteractor().Start()

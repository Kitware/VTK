from vtk import *

data_dir = "../../../../VTKData/Data/Infovis/SQLite/"
sqlite_file = data_dir + "ports_protocols.db"

# Pull the table (that represents relationships/edges) from the database
databaseToEdgeTable = vtkSQLDatabaseTableSource()
databaseToEdgeTable.SetURL("sqlite://" + sqlite_file)
databaseToEdgeTable.SetQuery("select src, dst, dport, protocol, port_protocol from tcp")

# Pull the table (that represents entities/vertices) from the database
databaseToVertexTable = vtkSQLDatabaseTableSource()
databaseToVertexTable.SetURL("sqlite://" + sqlite_file)
databaseToVertexTable.SetQuery("select ip, hostname from dnsnames")

cs = vtkContingencyStatistics()
cs.AddInputConnection(databaseToEdgeTable.GetOutputPort())
cs.AddColumnPair("dport","protocol")
cs.SetAssess(1)

graph = vtkTableToGraph()
graph.AddInputConnection(cs.GetOutputPort())
graph.SetVertexTableConnection(databaseToVertexTable.GetOutputPort())
graph.AddLinkVertex("src", "ip", False)
graph.AddLinkVertex("dst", "ip", False)
graph.AddLinkEdge("src", "dst")
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
theme.SetLineWidth(5)
theme.SetCellHueRange(0,.667)
theme.SetCellAlphaRange(0.5,0.5)
theme.SetCellSaturationRange(1,1)
theme.SetCellValueRange(1,1)
theme.SetCellOpacity(0.9)
theme.SetPointSize(10)
theme.SetSelectedCellColor(1,0,1)
theme.SetSelectedPointColor(1,0,1)
view.ApplyViewTheme(theme)
view.SetVertexLabelFontSize(20)
view.SetEdgeLabelFontSize(20)

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)
view.GetRenderer().ResetCamera()
window.GetInteractor().Start()

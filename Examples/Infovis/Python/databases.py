from vtk import *

database = vtkSQLDatabase.CreateFromURL("mysql://enron:enron@vizdb.srn.sandia.gov:3306/enron")
database.Open()

edge_query = database.GetQueryInstance()
edge_query.SetQuery("select sendID, recvID, weight from email_arcs")

vertex_query = database.GetQueryInstance()
vertex_query.SetQuery("select firstName, lastName, eid from employeelist")

edge_table = vtkRowQueryToTable()
edge_table.SetQuery(edge_query)

vertex_table = vtkRowQueryToTable()
vertex_table.SetQuery(vertex_query)

source = vtkTableToGraph()
source.AddInputConnection(edge_table.GetOutputPort())
source.AddLinkVertex("sendID", "eid", False)
source.AddLinkVertex("recvID", "eid", False)
source.AddLinkEdge("sendID", "recvID")
source.SetVertexTableConnection(vertex_table.GetOutputPort())

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(source.GetOutputPort())
view.SetVertexLabelArrayName("lastName")
view.SetVertexLabelVisibility(True)
view.SetEdgeColorArrayName("weight")
view.SetColorEdges(True)

theme = vtkViewTheme.CreateMellowTheme()
theme.SetCellValueRange(0, 0)
theme.SetCellAlphaRange(0.1, 2)
theme.SetCellOpacity(1)
view.ApplyViewTheme(theme)

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)
window.GetInteractor().Start()

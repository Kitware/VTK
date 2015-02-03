#!/usr/bin/env python
from vtk import *
import os.path

data_dir = "../../../../VTKData/Data/Infovis/"
if not os.path.exists(data_dir):
  data_dir = "../../../../../VTKData/Data/Infovis/"
csv_file = data_dir + "matrix.csv"

reader = vtkDelimitedTextReader()
reader.SetFileName(csv_file)
reader.SetHaveHeaders(True)

array = vtkTableToArray()
array.SetInputConnection(0, reader.GetOutputPort())
array.AddColumn("A")
array.AddColumn("B")
array.AddColumn("C")
array.AddColumn("D")
array.AddColumn("E")
array.AddColumn("F")

edges = vtkAdjacencyMatrixToEdgeTable()
edges.SetInputConnection(0, array.GetOutputPort())
edges.SetSourceDimension(0)
edges.SetMinimumThreshold(0.5)

graph = vtkTableToGraph()
graph.SetInputConnection(0, edges.GetOutputPort())
graph.AddLinkVertex("row", "index", False)
graph.AddLinkVertex("column", "index", False)
graph.AddLinkEdge("row", "column")

graph.Update()
reader.GetOutput().Dump(10)
edges.GetOutput().Dump(10)

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(graph.GetOutputPort())
view.SetVertexLabelArrayName("label")
view.SetVertexLabelVisibility(True)
view.SetEdgeLabelArrayName("value")
view.SetEdgeLabelVisibility(True)
view.SetLayoutStrategyToSimple2D()

theme = vtkViewTheme.CreateMellowTheme()
theme.SetCellColor(.2,.2,.6)
theme.SetLineWidth(5)
theme.SetPointSize(10)
view.ApplyViewTheme(theme)
view.SetVertexLabelFontSize(20)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()
view.GetInteractor().Start()



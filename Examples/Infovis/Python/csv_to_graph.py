#!/usr/bin/env python

from vtk import *

FILENAME_VERT_TABLE = "nodes.csv"
FILENAME_EDGE_TABLE = "edges.csv"

    
# Load the vertex table from CSV file
csv_vert_source = vtkDelimitedTextReader()
csv_vert_source.SetFieldDelimiterCharacters(",")
csv_vert_source.DetectNumericColumnsOn()
csv_vert_source.SetHaveHeaders(True)
csv_vert_source.SetFileName(FILENAME_VERT_TABLE)
    
# Load the edge table from CSV
csv_edge_source = vtkDelimitedTextReader()
csv_edge_source.SetFieldDelimiterCharacters(",")
csv_edge_source.DetectNumericColumnsOn()
csv_edge_source.SetHaveHeaders(True)
csv_edge_source.SetFileName(FILENAME_EDGE_TABLE)
    
# Create a graph from the vertex and edge tables
tbl2graph = vtkTableToGraph()
tbl2graph.SetDirected(True)
tbl2graph.AddInputConnection(csv_edge_source.GetOutputPort())
tbl2graph.AddLinkVertex("source", "label", False)
tbl2graph.AddLinkVertex("target", "label", False)
tbl2graph.AddLinkEdge("source", "target")
tbl2graph.SetVertexTableConnection(csv_vert_source.GetOutputPort())

#
# Draw the graph
#
view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(tbl2graph.GetOutputPort())
    
# Label vertices with the contents in the "label" column
view.SetVertexLabelArrayName("label")
view.SetVertexLabelVisibility(True)
    
# Set vertex coloring based on the "intVal" column in nodes.csv
view.SetVertexColorArrayName("intVal")
view.SetColorVertices(True)
    
# Set edge labels and colors
view.SetEdgeLabelArrayName("lbl")
view.SetEdgeLabelVisibility(True)
view.SetEdgeColorArrayName("value")
view.SetColorEdges(True)
    
theme = vtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
    
# set the layout strategy
view.SetLayoutStrategyToSimple2D()
    
view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()
view.GetInteractor().Start()

#!/usr/bin/env python
"""
This file demonstrates the creation of a directed graph using the
Python interface to VTK.
"""

from __future__ import print_function
from vtk import *

xdim = 600
ydim = 600

#------------------------------------------------------------------------------
# Script Entry Point
#------------------------------------------------------------------------------
if __name__ == "__main__":

    print("vtkGraph Example 1: Building a vtkMutableDirectedGraph from scratch.")

    #----------------------------------------------------------
    # Create a graph (see graph1.py for explanations)

    G1 = vtkMutableDirectedGraph()
    vertID1 = vtkIntArray()
    vertID1.SetName("ID")
    G1.GetVertexData().AddArray( vertID1 )
    G1.GetVertexData().SetPedigreeIds( vertID1 )
    for i in range(10):
        G1.AddVertex()
        vertID1.InsertNextValue(i)
    for i in range(10):
        G1.AddGraphEdge(i, (i+1)%10)

    #----------------------------------------------------------
    # Create a second graph
    # - This one is a triangle, with one node's pedigree id field
    #   ID matching one of the vertices from G1.  These nodes will
    #   be combined as the same node when merge is run.
    vertID2 = vtkIntArray()
    G2 = vtkMutableDirectedGraph()
    vertID2 = vtkIntArray()
    vertID2.SetName("ID")
    G2.GetVertexData().AddArray(vertID2)
    G2.GetVertexData().SetPedigreeIds(vertID2)
    G2.AddVertex()
    vertID2.InsertNextValue(3)
    G2.AddVertex()
    vertID2.InsertNextValue(20)
    G2.AddVertex()
    vertID2.InsertNextValue(21)
    G2.AddGraphEdge(0,1)
    G2.AddGraphEdge(0,2)
    G2.AddGraphEdge(1,2)

    #----------------------------------------------------------
    # Merge graphs takes two graphs on input ports 0 and 1.
    # and combines them according to the pedigree id field.
    merge = vtkMergeGraphs()
    merge.SetInputData(0, G1)
    merge.SetInputData(1, G2)
    merge.Update()

    #----------------------------------------------------------
    # Draw the graph in a window
    theme = vtkViewTheme.CreateMellowTheme()
    theme.SetLineWidth(4)
    theme.SetPointSize(10)
    theme.SetCellOpacity(1)
    theme.FastDelete()

    # View containing graph G1
    view = vtkGraphLayoutView()
    view.AddRepresentationFromInput(G1)
    view.SetVertexLabelArrayName("ID")
    view.SetVertexLabelVisibility(True)
    view.SetLayoutStrategyToSimple2D()
    view.SetVertexLabelFontSize(20)
    view.ApplyViewTheme(theme)
    view.GetRenderWindow().SetSize(xdim, ydim)
    view.ResetCamera()
    view.Render()

    # View containing graph G2
    view2 = vtkGraphLayoutView()
    view2.AddRepresentationFromInput(G2)
    view2.SetVertexLabelArrayName("ID")
    view2.SetVertexLabelVisibility(True)
    view2.SetLayoutStrategyToSimple2D()
    view2.SetVertexLabelFontSize(20)
    view2.ApplyViewTheme(theme)
    view2.GetRenderWindow().SetSize(xdim, ydim)
    view2.ResetCamera()
    view2.Render()

    # View containing the merged graph
    view3 = vtkGraphLayoutView()
    view3.AddRepresentationFromInputConnection(merge.GetOutputPort())
    view3.SetVertexLabelArrayName("ID")
    view3.SetVertexLabelVisibility(True)
    view3.SetLayoutStrategyToSimple2D()
    view3.SetVertexLabelFontSize(20)
    view3.ApplyViewTheme(theme)
    view3.GetRenderWindow().SetSize(xdim, ydim)
    view3.ResetCamera()
    view3.Render()

    view.GetInteractor().Start()

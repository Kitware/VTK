"""
This file demonstrates the creation of a directed graph using the
Python interface to VTK.
"""

from vtk import *

#------------------------------------------------------------------------------
# Script Entry Point
#------------------------------------------------------------------------------
if __name__ == "__main__":

    print "vtkGraph Example 1: Building a vtkMutableDirectedGraph from scratch."

    # Create an empty graph
    G = vtkMutableDirectedGraph()

    # Create an integer array to store vertex id data.
    vertID = vtkIntArray()
    vertID.SetName("ID")

    # Link the vertex id array into the vertex data of the graph
    G.GetVertexData().AddArray( vertID )

    # Add some vertices
    for i in range(10):
        G.AddVertex()
        vertID.InsertNextValue(i)

    # Add some edges in a ring
    for i in range(10):
        G.AddGraphEdge(i, (i+1)%10)

    #----------------------------------------------------------
    # Draw the graph in a window
    view = vtkGraphLayoutView()
    view.AddRepresentationFromInput(G)
    view.SetVertexLabelArrayName("ID")
    view.SetVertexLabelVisibility(True)
    view.SetLayoutStrategyToSimple2D()
    view.SetVertexLabelFontSize(20)

    theme = vtkViewTheme.CreateMellowTheme()
    theme.SetLineWidth(4)
    theme.SetPointSize(10)
    theme.SetCellOpacity(1)
    view.ApplyViewTheme(theme)
    theme.FastDelete()

    view.GetRenderWindow().SetSize(600, 600)
    view.ResetCamera()
    view.Render()

    view.GetInteractor().Start()

    print "vtkGraph Example 1: Finished."

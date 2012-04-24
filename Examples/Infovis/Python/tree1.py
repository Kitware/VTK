"""
This file demonstrates the creation of a tree using the
Python interface to VTK.
"""

from vtk import *

#------------------------------------------------------------------------------
# Script Entry Point
#------------------------------------------------------------------------------
if __name__ == "__main__":

    print "vtkTree Example 1: Building a tree from scratch."

    # Create an empty graph
    G = vtkMutableDirectedGraph()

    vertID = vtkIntArray()
    vertID.SetName("ID")

    G.GetVertexData().AddArray( vertID )

    # Add a root vertex
    root = G.AddVertex()
    vertID.InsertNextValue(root)

    # Add some vertices
    for i in range(3):
        v = G.AddChild(root)
        vertID.InsertNextValue(v)
        for j in range(2):
            u = G.AddChild(v)
            vertID.InsertNextValue(u)

    T = vtkTree()
    T.ShallowCopy(G)


    #----------------------------------------------------------
    # Draw the graph in a window
    view = vtkGraphLayoutView()
    view.AddRepresentationFromInput(G)
    view.SetVertexLabelArrayName("ID")
    view.SetVertexLabelVisibility(True)
    view.SetLayoutStrategyToTree()
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
    print "vtkTree Example 1: Finished."

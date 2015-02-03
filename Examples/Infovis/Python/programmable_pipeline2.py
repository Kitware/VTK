#!/usr/bin/env python
"""
This file demonstrates the creation of a directed graph using the
Python interface to VTK.
"""

from vtk import *
from sys import exit

xdim = 400
ydim = 400

#------------------------------------------------------------------------------
# Script Entry Point
#------------------------------------------------------------------------------
if __name__ == "__main__":

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
    # First Programmable Filter -- Add integer field to vertices
    # with a 1 for every vertex into G1
    G1_labeled = vtkProgrammableFilter()
    G1_labeled.AddInputData( G1 )

    def vertex_labeler_g1():
        input = G1_labeled.GetInput()
        output = G1_labeled.GetOutput()
        output.ShallowCopy(input)
        varray = vtkIntArray()
        varray.SetName("color")

        varray.SetNumberOfTuples( output.GetNumberOfVertices() )
        for i in range( output.GetNumberOfVertices() ):
            varray.SetValue(i, 1)

        output.GetVertexData().AddArray(varray)

    G1_labeled.SetExecuteMethod( vertex_labeler_g1 )



    #----------------------------------------------------------
    # Second Programmable Filter -- Add integer field to vertices
    # with a 2 for every vertex into G2
    G2_labeled = vtkProgrammableFilter()
    G2_labeled.AddInputData( G2 )

    def vertex_labeler_g2():
        input = G2_labeled.GetInput()
        output = G2_labeled.GetOutput()
        output.ShallowCopy(input)
        varray = vtkIntArray()
        varray.SetName("color")

        varray.SetNumberOfTuples( output.GetNumberOfVertices() )
        for i in range( output.GetNumberOfVertices() ):
            varray.SetValue(i, 2)

        output.GetVertexData().AddArray(varray)

    G2_labeled.SetExecuteMethod( vertex_labeler_g2 )


    #----------------------------------------------------------
    # Merge graphs takes two graphs on input ports 0 and 1.
    # and combines them according to the pedigree id field.
    merge = vtkMergeGraphs()
    merge.SetInputConnection(0, G1_labeled.GetOutputPort() )
    merge.SetInputConnection(1, G2_labeled.GetOutputPort() )
    merge.Update()



    #----------------------------------------------------------
    # Draw the graph in a window
    theme = vtkViewTheme.CreateMellowTheme()
    theme.SetLineWidth(4)
    theme.SetPointSize(15)
    theme.SetCellOpacity(1)
    theme.FastDelete()

    # Rendered graph representation to make vertices circles
    rep = vtkRenderedGraphRepresentation(  )
    rep.SetInputConnection(0, merge.GetOutputPort())

    # vtkGraphToGlyph::CIRCLE == 7
    rep.SetGlyphType(7)

    # View containing the merged graph
    view = vtkGraphLayoutView()
    view.SetRepresentation( rep )
    view.SetVertexLabelArrayName("ID")
    view.SetVertexColorArrayName("color")
    view.SetVertexLabelVisibility(True)
    view.SetColorVertices(True)
    view.SetLayoutStrategyToSimple2D()
    view.SetVertexLabelFontSize(20)
    view.ApplyViewTheme(theme)



    view.GetRenderWindow().SetSize(xdim, ydim)
    view.ResetCamera()
    view.Render()


    view.GetInteractor().Start()

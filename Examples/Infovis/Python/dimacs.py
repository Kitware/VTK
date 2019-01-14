#!/usr/bin/env python
"""
Just a demo script to test out the DIMACS reader/writer function
"""
from __future__ import print_function
import time
from vtk import *

# generate a random graph
source = vtkRandomGraphSource()
source.SetNumberOfVertices(100)
source.SetAllowSelfLoops(False)
source.SetEdgeProbability(0.1)
source.SetUseEdgeProbability(True)
source.AllowParallelEdgesOff()
source.Update()

print("Original graph:")
print("\tnum_verts =", source.GetOutput().GetNumberOfVertices())
print("\tnum_edges =", source.GetOutput().GetNumberOfEdges())
print("")

# write the graph to a file
writer = vtkDIMACSGraphWriter()
writer.AddInputConnection(source.GetOutputPort())
writer.SetFileName("graph.gr")
writer.Update()
print("wrote graph to 'graph.gr'")

# let's pause a second to let the disk catch up <shrug/>
time.sleep(1)

# read the graph in from the file
reader = vtkDIMACSGraphReader()
reader.SetFileName("graph.gr")
reader.Update()
print("read graph from 'graph.gr'")

print("Graph from disk:")
print("\tnum_verts =", reader.GetOutput().GetNumberOfVertices())
print("\tnum_edges =", reader.GetOutput().GetNumberOfEdges())

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(reader.GetOutputPort())
view.SetLayoutStrategyToSimple2D()

theme = vtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(1)
theme.SetPointSize(5)
view.ApplyViewTheme(theme)
theme.FastDelete()


view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()

view.GetInteractor().Start()

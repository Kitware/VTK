#!/usr/bin/env python
from vtk import *

graph = vtkMutableDirectedGraph()
a = graph.AddVertex()
b = graph.AddChild(a)
c = graph.AddChild(a)
d = graph.AddChild(b)
e = graph.AddChild(c)
f = graph.AddChild(c)

tree = vtkTree()
tree.CheckedShallowCopy(graph)

view = vtkGraphLayoutView()
view.AddRepresentationFromInput(tree)

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()
view.GetInteractor().Start()



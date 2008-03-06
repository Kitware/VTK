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

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)
window.GetInteractor().Start()


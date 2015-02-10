#!/usr/bin/env python
import vtk
import inspect
import csv
import sys
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def addToTree(cls, class_to_vertex_map, tree, names):
    vertexid = None

    if cls in class_to_vertex_map:
        vertexid = class_to_vertex_map[cls]
    else:
        vertexid = tree.AddVertex()
        class_to_vertex_map[cls] = vertexid
        names.InsertValue(vertexid, cls)

    return vertexid


def parseClassTree(classtree, tree, names):

    class_to_vertex_map = dict()

    for (parent, child) in classtree:
        parentid = addToTree(parent, class_to_vertex_map, tree, names)
        childid = addToTree(child, class_to_vertex_map, tree, names)

        tree.AddGraphEdge(parentid, childid)

# The tree that will hold vtk class hierarchy
builder = vtk.vtkMutableDirectedGraph()
names = vtk.vtkStringArray()
names.SetName("name")

# Allocate 100 tuples to start with. I picked this number
# out of air.
names.SetNumberOfTuples(100)

# load vtk class information
reader = csv.reader(open(VTK_DATA_ROOT + "/Data/Infovis/classes.csv", 'r'))

classes = []

for row in reader:
    classes.append(row)

# Convert list to tree
parseClassTree(classes, builder, names)

tree = vtk.vtkTree()
tree.CheckedShallowCopy(builder)
tree.GetVertexData().AddArray(names)

## Now iterate over the tree and print it
#iter = vtk.vtkTreeDFSIterator()
#iter.SetTree(tree)
#
#while iter.HasNext():
#    id = iter.Next()
#    mystr = tree.GetLevel(id) * "   "
#    print mystr + str(names.GetValue(id))

# Display the tree in the tree map viewer
view = vtk.vtkGraphLayoutView()
view.AddRepresentationFromInput(tree);
view.ResetCamera()
view.Render()
view.SetVertexLabelArrayName("name")
view.SetVertexLabelVisibility(True)
view.GetInteractor().Start()

win = vtk.vtkRenderWindow()
interact = vtk.vtkRenderWindowInteractor()
interact.SetRenderWindow(win)
view.SetRenderWindow(win)
win.Render()

win.GetInteractor().Initialize()
win.GetInteractor().Start();


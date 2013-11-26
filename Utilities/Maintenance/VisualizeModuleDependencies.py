#!/usr/bin/env python

'''
This program takes a list of module files and creates a (possibly disjoint)
directed graph of the modules and their dependencies. Arrows on the
directed graph point to the dependent module.

Typical usage would be as follows:
 VisualizeModuleDependencies.py VTKSourceDir vtkFiltersSources,vtkInteractionStyle,vtkRenderingOpenGL

'''

import os, sys
from collections import defaultdict
import vtk

def GetProgramParameters():
    import argparse
    description = 'Creates a directed graph of the modules and their dependencies.'
    epilogue = '''
        This program takes a list of module files and creates a
        (possibly disjoint) directed graph of the modules and their
        dependencies. Arrows on the directed graph point to the dependent module.
        By default, dependencies of a given module are followed to their maximum
        depth. However you can restrict the depth by specifying the depth to
        which dependent modules are searched.
        The moduleList is a comma-separated list of module names with no
        spaces between the names.
        The treeDepth defaults to 0, this means that for a given module all
        dependent modules will be found. If non-zero, then trees will be only
        searched to that depth.
    '''
    parser = argparse.ArgumentParser(description=description, epilog=epilogue)
    parser.add_argument('vtkSourceDir', help='The path to the vtk Source Directory.')
    parser.add_argument('moduleList', help='The list of modules.')
    parser.add_argument('moduleTreeDepth', help='The depth of the module trees', nargs='?', default=0, type=int)
    args = parser.parse_args()
    vtkSourceDir = args.vtkSourceDir
    moduleList = [x.strip() for x in args.moduleList.split(',')]
    moduleTreeDepth = args.moduleTreeDepth
    return (vtkSourceDir, moduleList, moduleTreeDepth)

def GetProgramParametersOld():
    '''
    Used for Python versions < 2.7
    '''
    if len(sys.argv) < 3:
        s = 'Usage: ' + sys.argv[0] + ' vtkSourceDir moduleList [moduleTreeDepth]'
        print(s)
        exit(0)
    args = dict()
    args['vtkSourceDir'] = sys.argv[1]
    args['moduleList'] = sys.argv[2]
    args['moduleTreeDepth'] = 0
    if len(sys.argv) > 3:
        args['moduleTreeDepth'] = int(sys.argv[3])
    vtkSourceDir = args['vtkSourceDir']
    moduleList = [x.strip() for x in args['moduleList'].split(',')]
    moduleTreeDepth = args['moduleTreeDepth']
    return (vtkSourceDir, moduleList, moduleTreeDepth)

def FindModuleFiles(path):
    '''
    Get a list of module files in the VTK directory.
    '''
    moduleFiles = [os.path.join(root, name)
                 for root, dirs, files in os.walk(path)
                 for name in files
                 if name == ("module.cmake")]
    return moduleFiles

def ParseModuleFile(fileName):
    '''
    Read each module file returning the module name and what
    it depends on or implements.
    '''
    fh = open(fileName, 'rb')
    lines = []
    for line in fh:
        line = line.strip()
        if line.startswith('$'):  # Skip CMake variable names
            continue
        if line.startswith('#'):
            continue
        line = line.split('#')[0].strip()  # inline comments
        if line == "":
            continue
        line = line.split(')')[0].strip()  # closing brace with no space
        if line == "":
            continue
        for l in line.split(" "):
            lines.append(l)
    keywords = ['GROUPS', 'DEPENDS', 'IMPLEMENTS', 'PRIVATE_DEPENDS', 'TEST_DEPENDS',
                 'COMPILE_DEPENDS', 'EXCLUDE_FROM_WRAPPING', 'EXCLUDE_FROM_ALL']
    moduleName = ""
    depends = []
    implements = []
    state = "START";
    for item in lines:
        if state == "START" and item.startswith("vtk_module("):
            moduleName = item.split("(")[1]
            continue
        if item in keywords:
            state = item
            continue
        if state == 'DEPENDS' and item != ')':
            depends.append(item)
            continue
        if state == 'IMPLEMENTS' and item != ')':
            implements.append(item)
            continue
    return [moduleName, depends + implements]

def FindAllNeededModules(modules, foundModules, moduleDepencencies):
    '''
    Recursively search moduleDependencies finding all modules.
    '''
    if modules != None and len(modules) > 0:
        for m in modules:
            foundModules.add(m)
            foundModules = foundModules | set(moduleDepencencies[m])  # Set union
            foundModules = FindAllNeededModules(moduleDepencencies[m],
                                                foundModules, moduleDepencencies)
    return foundModules

def MakeModuleTree(module, index, tree, moduleDependencies, treeDepth, level=0):
    '''
    For a given module make a tree with the module as the root and the
    dependent modules as children.
    '''
    if module:
        index = index + [module]
        if treeDepth == 0 or level < treeDepth:
            for m in moduleDependencies[module]:
                level += 1
                MakeModuleTree(m, index, tree, moduleDependencies, treeDepth, level)
                level -= 1
    Add(tree, index)

# One-line Tree in Python
# See: https:gist.github.com/hrldcpr/2012250
def Tree(): return defaultdict(Tree)

def Add(tree, keys):
    for key in keys:
        tree = tree[key]

def PrettyPrint(tree, level=0):
    '''
    Useful to visualize the tree.
    '''
    result = ''
    for k, v in tree.iteritems():
        s = '  ' * level + k + '\n'
        result += s
        level += 1
        result += PrettyPrint(v, level)
        level -= 1
    return result

def GetAllKeys(tree):
    '''
    Return all the modules in the tree as a set.
    '''
    modules = set()
    for key in tree:
        modules = set(list(modules) + [key] + list(GetAllKeys(tree[key])))
    return modules

def MakeEdgeList(t):
    '''
    Return a set that represents the edges in the tree.
    '''
    edgeList = set()
    for k, v in t.iteritems():
        subKeys = v.keys()
        if subKeys:
            for kk in subKeys:
                edgeList.add((k, kk))
        edg = MakeEdgeList(v)
        if edg:
            edgeList.update(edg)
    return edgeList

def MakeGraph(t, parent='', level=0):
    '''
    Returns a list that has two elements, the vertices and the edge list.
    '''
    return [GetAllKeys(t), MakeEdgeList(t)]

def GenerateGraph(moduleList, moduleDepencencies, moduleTreeDepth):
    '''
    Generate a graph from the module list.
    The resultant graph is a list consisting of two sets, the first set
    is the set of vertices and the second set is the edge list.
    '''
    graph = [set(), set()]
    for m in moduleList:
        t = Tree()
        MakeModuleTree(m, [], t, moduleDepencencies, moduleTreeDepth)
        g = MakeGraph(t)
        graph[0].update(g[0])
        if g[1]:
            graph[1].update(g[1])
    return graph

def GenerateVTKGraph(graph):
    '''
    Take the vertices and edge list in the graph parameter
    and return a VTK graph.
    '''
    g = vtk.vtkMutableDirectedGraph()
    # Label the vertices
    labels = vtk.vtkStringArray()
    labels.SetNumberOfComponents(1)
    labels.SetName("Labels")

    index = dict()
    l = list(graph[0])
    # Make the vertex labels and create a dictionary with the
    # keys as labels and the vertex ids as the values.
    for i in range(0, len(l)):
        # Set the vertex labels
        labels.InsertNextValue(l[i])
        index[l[i]] = g.AddVertex()
    g.GetVertexData().AddArray(labels)
    # Add edges
    l = list(graph[1])
    for i in range(0, len(l)):
        ll = list(l[i])
        g.AddGraphEdge(index[ll[0]], index[ll[1]])
#    g.Dump()
    return g

def DisplayGraph(graph):
    '''
    Display the graph.
    '''
    theme = vtk.vtkViewTheme()
    theme.SetBackgroundColor(0, 0, .1)
    theme.SetBackgroundColor2(0, 0, .5)

    # Layout the graph
    # Pick a strategy you like.
    # strategy = vtk.vtkCircularLayoutStrategy()
    strategy = vtk.vtkSimple2DLayoutStrategy()
    # strategy = vtk.vtkRandomLayoutStrategy()
    layout = vtk.vtkGraphLayout()
    layout.SetLayoutStrategy(strategy)
    layout.SetInputData(graph)

    view = vtk.vtkGraphLayoutView()
    view.AddRepresentationFromInputConnection(layout.GetOutputPort())
    # Tell the view to use the vertex layout we provide.
    view.SetLayoutStrategyToPassThrough()
    view.SetEdgeLabelVisibility(True)
    view.SetVertexLabelArrayName("Labels")
    view.SetVertexLabelVisibility(True)
    view.ApplyViewTheme(theme)

    # Manually create an actor containing the glyphed arrows.
    # Get the edge geometry
    edgeGeom = vtk.vtkGraphToPolyData()
    edgeGeom.SetInputConnection(layout.GetOutputPort())
    edgeGeom.EdgeGlyphOutputOn()

    # Set the position (0: edge start, 1: edge end) where
    # the edge arrows should go.
#        edgeGeom.SetEdgeGlyphPosition(0.8)
    edgeGeom.SetEdgeGlyphPosition(0.85)

    # Make a simple edge arrow for glyphing.
#        arrowSource = vtk.vtkGlyphSource2D()
#        arrowSource.SetGlyphTypeToEdgeArrow()
#        arrowSource.SetScale(0.075)
    # Or use a cone.
    coneSource = vtk.vtkConeSource()
    coneSource.SetRadius(0.025)
    coneSource.SetHeight(0.1)
    coneSource.SetResolution(12)

    # Use Glyph3D to repeat the glyph on all edges.
    arrowGlyph = vtk.vtkGlyph3D()
    arrowGlyph.SetInputConnection(0, edgeGeom.GetOutputPort(1))
#        arrowGlyph.SetInputConnection(1, arrowSource.GetOutputPort())
    arrowGlyph.SetInputConnection(1, coneSource.GetOutputPort())

    # Add the edge arrow actor to the view.
    arrowMapper = vtk.vtkPolyDataMapper()
    arrowMapper.SetInputConnection(arrowGlyph.GetOutputPort())
    arrowActor = vtk.vtkActor()
    arrowActor.SetMapper(arrowMapper)
    view.GetRenderer().AddActor(arrowActor)

    view.ResetCamera()
    view.Render()

    view.SetInteractionModeTo3D()
    view.GetInteractor().Initialize()
    view.GetInteractor().Start()

def main():
    ver = list(sys.version_info[0:2])
    ver = ver[0] + ver[1] / 10.0
    if ver >= 2.7:
        vtkSourceDir, moduleList, moduleTreeDepth = GetProgramParameters()
    else:
        vtkSourceDir, moduleList, moduleTreeDepth = GetProgramParametersOld()

    # Parse the module files making a dictionary of each module and its
    # dependencies or what it implements.
    moduleDepencencies = dict()
    moduleFiles = FindModuleFiles(vtkSourceDir + "/")
    for fname in moduleFiles:
        m = ParseModuleFile(fname)
        moduleDepencencies[m[0]] = m[1]

    # Generate a graph from the module list.
    graph = GenerateGraph(moduleList, moduleDepencencies, moduleTreeDepth)

    # Now build a vtk graph.
    g = GenerateVTKGraph(graph)

    # Display it.
    DisplayGraph(g)

if __name__ == '__main__':
    main()

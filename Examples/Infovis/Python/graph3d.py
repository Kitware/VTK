#!/usr/bin/env python
from vtk import *
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

reader = vtkXGMLReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/Infovis/fsm.gml")
reader.Update()

strategy   = vtkSpanTreeLayoutStrategy()
strategy.DepthFirstSpanningTreeOn()

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(reader.GetOutputPort())
view.SetVertexLabelArrayName("vertex id")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("vertex id")
view.SetColorVertices(True)
view.SetLayoutStrategy( strategy )
view.SetInteractionModeTo3D() # Left mouse button causes 3D rotate instead of zoom

theme = vtkViewTheme.CreateMellowTheme()
theme.SetCellColor(.2,.2,.6)
theme.SetLineWidth(2)
theme.SetPointSize(10)
view.ApplyViewTheme(theme)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()

#Here's the window with David's original layout methodology
#  Aside from the theme elements in the view above, the notable
#  difference between the two views is the angling on the edges.
layout = vtkGraphLayout()
layout.SetLayoutStrategy(strategy)
layout.SetInputConnection(reader.GetOutputPort())

edge_geom = vtkGraphToPolyData()
edge_geom.SetInputConnection(layout.GetOutputPort())

vertex_geom = vtkGraphToPoints()
vertex_geom.SetInputConnection(layout.GetOutputPort())

# Vertex pipeline - mark each vertex with a cube glyph
cube = vtkCubeSource()
cube.SetXLength(0.3)
cube.SetYLength(0.3)
cube.SetZLength(0.3)

glyph = vtkGlyph3D()
glyph.SetInputConnection(vertex_geom.GetOutputPort())
glyph.SetSourceConnection(0, cube.GetOutputPort())

gmap = vtkPolyDataMapper()
gmap.SetInputConnection(glyph.GetOutputPort())

gact = vtkActor()
gact.SetMapper(gmap)
gact.GetProperty().SetColor(0,0,1)

# Edge pipeline - map edges to lines
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(edge_geom.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(0.4,0.4,0.6)

# Renderer, window, and interaction
ren = vtkRenderer()
ren.AddActor(actor)
ren.AddActor(gact)
ren.ResetCamera()

renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(800,550)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
#iren.Start()

view.GetInteractor().Start()

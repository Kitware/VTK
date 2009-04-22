from vtk import *


reader = vtkXMLTreeReader()
reader.SetFileName("vtkclasses.xml")
reader.Update()

# Layout: use 3D cone-tree - large subtrees so compress spacing...
view = vtkConeLayoutStrategy()
view.SetSpacing(0.3);
layout = vtkGraphLayout()
layout.SetLayoutStrategy(view)
layout.SetInputConnection(reader.GetOutputPort())

geom = vtkGraphToPolyData()
geom.SetInputConnection(layout.GetOutputPort())

# Vertex pipeline - mark each vertex with a cube glyph
cube = vtkCubeSource()
cube.SetXLength(1.0)
cube.SetYLength(1.0)
cube.SetZLength(1.0)

glyph = vtkGlyph3D()
glyph.SetInputConnection(geom.GetOutputPort())
glyph.SetSource(cube.GetOutput())

gmap = vtkPolyDataMapper()
gmap.SetInputConnection(glyph.GetOutputPort())

gact = vtkActor()
gact.SetMapper(gmap)
gact.GetProperty().SetColor(0,0,1)


# Edge pipeline - map edges to lines
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(geom.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(0.4,0.4,0.6)


# Label pipeline.
# 1.  Find vertex degrees
# 2.  Select vertices with degree >= 10 (could include parent)
# 3.  Extract those vertices form graph
# 4.  Render labels

degree = vtkVertexDegree()
degree.SetInputConnection(layout.GetOutputPort())

sel = vtkSelectionSource()
sel.SetContentType(7)
sel.SetFieldType(3)
sel.SetArrayName("VertexDegree")
sel.AddThreshold(10,10000)    # classes with "large" numbers of children...
sel.Update()

# Take selection and extract a graph
extract_graph = vtkExtractSelectedGraph()
extract_graph.AddInputConnection(degree.GetOutputPort())
extract_graph.SetSelectionConnection(sel.GetOutputPort())

geom2 = vtkGraphToPolyData()
geom2.SetInputConnection(extract_graph.GetOutputPort())

labels = vtkLabeledDataMapper()
labels.SetFieldDataArray(1)
labels.SetInputConnection(geom2.GetOutputPort())
labels.SetLabelModeToLabelFieldData()

prop = labels.GetLabelTextProperty()
prop.SetColor(1,1,1)
prop.SetJustificationToCentered()
prop.SetVerticalJustificationToCentered()
prop.SetFontSize(10)

text = vtkActor2D()
text.SetMapper(labels)


# Renderer, window, and interaction

ren = vtkRenderer()
ren.AddActor(actor)
ren.AddActor(gact)
ren.AddActor(text)
ren.ResetCamera()

renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(800,550)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
iren.Start()


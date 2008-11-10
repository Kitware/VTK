
from vtk import *

reader1 = vtkXMLTreeReader()
reader1.SetFileName("vtkclasses.xml")
reader2 = vtkXMLTreeReader()
reader2.SetFileName("vtklibrary.xml")
reader1.SetEdgePedigreeIdArrayName("tree edge")
reader2.SetEdgePedigreeIdArrayName("graph edge")

reader1.Update()
reader2.Update()

dummy = vtkHierarchicalTreeRingView()
view = vtkHierarchicalTreeRingView()
view.SetLayoutStrategy()
view.SetHierarchyFromInputConnection(reader2.GetOutputPort())
view.SetGraphFromInputConnection(reader1.GetOutputPort())
view.SetVertexColorArrayName("VertexDegree")
view.SetEdgeColorArrayName("tree edge")
view.SetHoverArrayName("id")
view.SetColorEdges(True)
view.SetVertexLabelArrayName("id")
view.SetVertexLabelVisibility(True)
view.SetSectorShrinkFactor(0.02)
view.SetBundlingStrength(.5)

view2 = vtkHierarchicalTreeRingView()
view2.SetLayoutStrategy()
view2.SetHierarchyFromInputConnection(reader1.GetOutputPort())
view2.SetGraphFromInputConnection(reader2.GetOutputPort())
view2.SetRootAngles(180.,360.)
view2.SetVertexColorArrayName("vertex id")
view2.SetEdgeColorArrayName("graph edge")
view2.SetColorEdges(True)
view2.SetHoverArrayName("id")
view2.SetVertexLabelArrayName("id")
view2.SetSectorShrinkFactor(0.01)
view2.SetVertexLabelVisibility(True)

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
view2.ApplyViewTheme(theme)
theme.FastDelete()

win = vtkRenderWindow()
dummy.SetupRenderWindow(win)
view.SetupRenderWindow(win)
view.GetRenderer().ResetCamera()

win2 = vtkRenderWindow()
view2.SetupRenderWindow(win2)

win.GetInteractor().Initialize()
win2.GetInteractor().Initialize()

win.GetInteractor().Start()


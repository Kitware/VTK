
from vtk import *

reader1 = vtkXMLTreeReader()
reader1.SetFileName("vtkclasses.xml")
reader1.SetEdgePedigreeIdArrayName("tree edge")
reader1.GenerateVertexPedigreeIdsOff();
reader1.SetVertexPedigreeIdArrayName("id");

reader2 = vtkXMLTreeReader()
reader2.SetFileName("vtklibrary.xml")
reader2.SetEdgePedigreeIdArrayName("graph edge")
reader2.GenerateVertexPedigreeIdsOff();
reader2.SetVertexPedigreeIdArrayName("id");

reader1.Update()
reader2.Update()

view = vtkTreeRingView()
view.SetTreeFromInputConnection(reader2.GetOutputPort())
view.SetGraphFromInputConnection(reader1.GetOutputPort())
view.SetAreaColorArrayName("VertexDegree")
view.SetEdgeColorArrayName("tree edge")
view.SetAreaHoverArrayName("id")
view.SetColorEdges(True)
view.SetAreaLabelArrayName("id")
view.SetAreaLabelVisibility(True)
view.SetShrinkPercentage(0.02)
view.SetBundlingStrength(.5)

view2 = vtkTreeRingView()
view2.SetTreeFromInputConnection(reader1.GetOutputPort())
view2.SetGraphFromInputConnection(reader2.GetOutputPort())
view2.SetRootAngles(180.,360.)
view2.SetAreaColorArrayName("VertexDegree")
view2.SetEdgeColorArrayName("graph edge")
view2.SetColorEdges(True)
view2.SetAreaHoverArrayName("id")
view2.SetAreaLabelArrayName("id")
view2.SetAreaLabelVisibility(True)
view2.SetShrinkPercentage(0.01)

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
view2.ApplyViewTheme(theme)
theme.FastDelete()

win = vtkRenderWindow()
view.SetupRenderWindow(win)
view.Update()
view.GetRenderer().ResetCamera()

win2 = vtkRenderWindow()
view2.SetupRenderWindow(win2)
view2.Update()
view2.GetRenderer().ResetCamera()

win.GetInteractor().Initialize()
win2.GetInteractor().Initialize()

win.GetInteractor().Start()


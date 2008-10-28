
from vtk import *

reader1 = vtkXMLTreeReader()
reader1.SetFileName("vtkclasses.xml")
reader2 = vtkXMLTreeReader()
reader2.SetFileName("vtklibrary.xml")

reader1.Update()
reader2.Update()

dummy = vtkHierarchicalGraphView()
view = vtkHierarchicalGraphView()
view.SetAddRingsToView(True)
view.SetHierarchyFromInputConnection(reader2.GetOutputPort())
view.SetGraphFromInputConnection(reader1.GetOutputPort())
view.SetVertexColorArrayName("VertexDegree")
view.SetColorVertices(True)
#view.SetEdgeColorToSplineFraction()
view.SetVertexLabelArrayName("id")
view.SetVertexLabelVisibility(True)
#view.SetLayoutStrategyToCosmicTree()
view.SetRadialLayout(True)
view.SetRadialAngle(360)
view.SetBundlingStrength(.5)

view2 = vtkHierarchicalGraphView()
view2.SetHierarchyFromInputConnection(reader2.GetOutputPort())
view2.SetGraphFromInputConnection(reader1.GetOutputPort())
view2.SetVertexColorArrayName("VertexDegree")
view2.SetColorVertices(True)
#view2.SetEdgeColorArrayName("weight")
view2.SetColorEdges(True)
view2.SetVertexLabelArrayName("id")
view2.SetVertexLabelVisibility(True)
view2.TreeEdgeVisibilityOn()

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


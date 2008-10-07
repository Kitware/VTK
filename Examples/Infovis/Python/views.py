
from vtk import *

xmlRootDir = "../../../../VTKData/Data/Infovis/XML/"


treeReader = vtkXMLTreeReader()
treeReader.SetFileName(xmlRootDir+"vtklibrary.xml")
graphReader = vtkXMLTreeReader()
graphReader.SetFileName(xmlRootDir+"vtkclasses.xml")

# Create a tree layout strategy
treeStrat = vtkTreeLayoutStrategy();
treeStrat.RadialOn()
treeStrat.SetAngle(360)
treeStrat.SetLogSpacingValue(1)

# Display the tree in the tree map viewer
view0 = vtkTreeMapView()
view0.AddRepresentationFromInputConnection(treeReader.GetOutputPort());
view0.SetLabelArrayName("id")
#view0.SetLayoutStrategyToBox()
#view0.SetLayoutStrategyToSliceAndDice()
view0.SetLayoutStrategyToSquarify()
view0.SetFontSizeRange(16,8,4)
view0.SetBorderPercentage(.02)

# Create a graph layout view
view1 = vtkGraphLayoutView()
view1.AddRepresentationFromInputConnection(treeReader.GetOutputPort())
view1.SetVertexLabelArrayName("id")
view1.SetVertexLabelVisibility(True)
view1.SetVertexColorArrayName("VertexDegree")
view1.SetColorVertices(True)
view1.SetEdgeColorArrayName("edge_id")
view1.SetColorEdges(True)
#view1.SetLayoutStrategyToSimple2D()
view1.SetLayoutStrategy(treeStrat)

view2 = vtkHierarchicalGraphView()
view2.SetHierarchyFromInputConnection(treeReader.GetOutputPort())
view2.SetGraphFromInputConnection(graphReader.GetOutputPort())
view2.SetVertexLabelArrayName("id")
view2.SetVertexLabelVisibility(True)
view2.SetVertexColorArrayName("VertexDegree")
view2.SetColorVertices(True)
view2.SetEdgeColorArrayName("edge id")
view2.SetColorEdges(True)
view2.TreeEdgeVisibilityOn()
view2.SetLayoutStrategy(treeStrat)
view2.SetBundlingStrength(.7)

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
view0.ApplyViewTheme(theme)
view1.ApplyViewTheme(theme)
view2.ApplyViewTheme(theme)
theme.FastDelete()

win0 = vtkRenderWindow()
win0.SetSize(600,600)
view0.SetupRenderWindow(win0)
win0.GetInteractor().Initialize()

win1 = vtkRenderWindow()
win1.SetSize(600,600)
view1.SetupRenderWindow(win1)
view1.GetRenderer().ResetCamera()
win1.GetInteractor().Initialize()

win2 = vtkRenderWindow()
win2.SetSize(600,600)
view2.SetupRenderWindow(win2)
view2.GetRenderer().ResetCamera()
win2.GetInteractor().Initialize()

win0.GetInteractor().Start()


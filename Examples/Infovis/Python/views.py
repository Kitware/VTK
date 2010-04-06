from vtk import *
import os.path

xmlRootDir = "../../../../VTKData/Data/Infovis/XML/"
if not os.path.exists(xmlRootDir):
  xmlRootDir = "../../../../../VTKData/Data/Infovis/XML/"

treeReader = vtkXMLTreeReader()
treeReader.SetFileName(xmlRootDir+"vtklibrary.xml")
treeReader.SetEdgePedigreeIdArrayName("tree edge")
treeReader.GenerateVertexPedigreeIdsOff();
treeReader.SetVertexPedigreeIdArrayName("id");
graphReader = vtkXMLTreeReader()
graphReader.SetFileName(xmlRootDir+"vtkclasses.xml")
graphReader.SetEdgePedigreeIdArrayName("graph edge")
graphReader.GenerateVertexPedigreeIdsOff();
graphReader.SetVertexPedigreeIdArrayName("id");
graphReader.Update()
print graphReader.GetOutput()

# Create a tree layout strategy
treeStrat = vtkTreeLayoutStrategy();
treeStrat.RadialOn()
treeStrat.SetAngle(360)
treeStrat.SetLogSpacingValue(1)

view0 = vtkTreeRingView()
view0.SetTreeFromInputConnection(treeReader.GetOutputPort())
view0.SetGraphFromInputConnection(graphReader.GetOutputPort())
view0.SetAreaLabelArrayName("id")
view0.SetAreaColorArrayName("VertexDegree")
view0.SetAreaHoverArrayName("id")
view0.SetAreaLabelVisibility(True)
view0.SetShrinkPercentage(0.02)
view0.SetBundlingStrength(.75)
view0.Update()
view0.SetEdgeColorArrayName("graph edge")
view0.SetColorEdges(True)



# Create a graph layout view
view1 = vtkGraphLayoutView()
view1.AddRepresentationFromInputConnection(treeReader.GetOutputPort())
view1.SetVertexLabelArrayName("id")
view1.SetVertexLabelVisibility(True)
view1.SetVertexColorArrayName("VertexDegree")
view1.SetColorVertices(True)
view1.SetEdgeColorArrayName("edge_id")
view1.SetColorEdges(True)
view1.SetLayoutStrategyToTree()


view2 = vtkHierarchicalGraphView()
view2.SetHierarchyFromInputConnection(treeReader.GetOutputPort())
view2.SetGraphFromInputConnection(graphReader.GetOutputPort())
view2.SetVertexLabelArrayName("id")
view2.SetVertexLabelVisibility(True)
view2.SetVertexColorArrayName("VertexDegree")
view2.SetColorVertices(True)
view2.SetEdgeColorArrayName("edge id")
view2.SetColorEdges(True)
view2.SetLayoutStrategy(treeStrat)
view2.SetBundlingStrength(.7)

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(2)
theme.SetPointSize(10)
theme.SetSelectedCellColor(1,1,1)
theme.SetSelectedPointColor(1,1,1)
view0.ApplyViewTheme(theme)
view1.ApplyViewTheme(theme)
view2.ApplyViewTheme(theme)
theme.FastDelete()

view0.GetRenderWindow().SetSize(600,600)
view0.ResetCamera()
view0.Render()

view1.GetRenderWindow().SetSize(600,600)
view1.ResetCamera()
view1.Render()

view2.GetRenderWindow().SetSize(600,600)
view2.ResetCamera()
view2.Render()

view0.GetInteractor().Start()


#!/usr/bin/env python

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

view = vtkTreeRingView()
view.SetTreeFromInputConnection(reader2.GetOutputPort())
view.SetGraphFromInputConnection(reader1.GetOutputPort())
view.SetAreaColorArrayName("VertexDegree")
view.SetAreaHoverArrayName("id")
view.SetAreaLabelArrayName("id")
view.SetAreaLabelVisibility(True)
view.SetShrinkPercentage(0.02)
view.SetBundlingStrength(.5)
view.Update()
view.SetEdgeColorArrayName("tree edge")
view.SetColorEdges(True)

view2 = vtkTreeRingView()
view2.SetTreeFromInputConnection(reader1.GetOutputPort())
view2.SetGraphFromInputConnection(reader2.GetOutputPort())
view2.SetRootAngles(180.,360.)
view2.SetAreaColorArrayName("VertexDegree")
view2.SetAreaHoverArrayName("id")
view2.SetAreaLabelArrayName("id")
view2.SetAreaLabelVisibility(True)
view2.SetShrinkPercentage(0.01)
view2.Update()
view2.SetEdgeColorArrayName("graph edge")
view2.SetColorEdges(True)

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
view2.ApplyViewTheme(theme)
theme.FastDelete()

view.ResetCamera()
view.Render()

view2.ResetCamera()
view2.Render()

view.GetInteractor().Start()


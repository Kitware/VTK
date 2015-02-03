#!/usr/bin/env python
from vtk import *

reader1 = vtkXMLTreeReader()
reader1.SetFileName("treetest.xml")
reader1.Update()

view = vtkTreeRingView()
view.SetRepresentationFromInput(reader1.GetOutput())
view.SetAreaSizeArrayName("size")
view.SetAreaColorArrayName("level")
view.SetAreaLabelArrayName("name")
view.SetAreaLabelVisibility(True)
view.SetAreaHoverArrayName("name")
view.SetShrinkPercentage(0.05)
view.Update()

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
theme.FastDelete()

view.ResetCamera()
view.Render()

view.GetInteractor().Start()

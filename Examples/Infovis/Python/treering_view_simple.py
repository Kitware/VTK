from vtk import *

reader1 = vtkXMLTreeReader()
reader1.SetFileName("treetest.xml")
reader1.Update()

dummy = vtkTreeRingView()
view = vtkTreeRingView()
view.SetRepresentationFromInput(reader1.GetOutput())
view.SetSizeArrayName("size")
view.SetColorArrayName("level")
view.SetLabelArrayName("name")
view.SetHoverArrayName("name")
view.SetSectorShrinkPercentage(0.05)
view.SetLayoutStrategyToDefault()
view.Update()

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
theme.FastDelete()

win = vtkRenderWindow()
dummy.SetupRenderWindow(win)
view.SetupRenderWindow(win)
view.GetRenderer().ResetCamera()

win.GetInteractor().Initialize()
win.GetInteractor().Start()

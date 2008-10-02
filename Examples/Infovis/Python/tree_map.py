from vtk import *

reader1 = vtkXMLTreeReader()
reader1.SetFileName("treetest.xml")
reader1.Update()

numeric = vtkStringToNumeric()
numeric.SetInputConnection(reader1.GetOutputPort())

view = vtkTreeMapView()
view.SetSizeArrayName("size");
view.SetColorArrayName("level");
view.SetLabelArrayName("name");
view.SetHoverArrayName("name");
view.SetLayoutStrategyToSquarify();
view.SetRepresentationFromInputConnection(numeric.GetOutputPort());

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
theme.FastDelete()

win = vtkRenderWindow()
view.SetupRenderWindow(win)
view.GetRenderer().ResetCamera()

win.GetInteractor().Initialize()
win.GetInteractor().Start()

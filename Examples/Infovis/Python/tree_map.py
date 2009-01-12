from vtk import *

reader1 = vtkXMLTreeReader()
reader1.SetFileName("treetest.xml")
reader1.Update()

numeric = vtkStringToNumeric()
numeric.SetInputConnection(reader1.GetOutputPort())

view = vtkTreeMapView()
view.SetAreaSizeArrayName("size");
view.SetAreaColorArrayName("level");
view.SetAreaLabelArrayName("name");
view.SetAreaLabelVisibility(True);
view.SetAreaHoverArrayName("name");
view.SetLayoutStrategyToSquarify();
view.SetRepresentationFromInputConnection(numeric.GetOutputPort());

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
theme.FastDelete()

win = vtkRenderWindow()
view.SetupRenderWindow(win)
view.Update()
view.GetRenderer().ResetCamera()

win.GetInteractor().Initialize()
win.GetInteractor().Start()

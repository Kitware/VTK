from vtk import *

source = vtkRandomGraphSource()

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(source.GetOutputPort())

theme = vtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
theme.FastDelete()

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)
view.SetVertexColorArrayName("VertexDegree");
view.SetColorVertices(True);
window.GetInteractor().Start()


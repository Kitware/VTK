from vtk import *

source = vtkRandomGraphSource()

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(source.GetOutputPort())

theme = vtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.SetVertexColorArrayName("VertexDegree");
view.SetColorVertices(True);
view.ResetCamera()
view.Render()
view.GetInteractor().Start()


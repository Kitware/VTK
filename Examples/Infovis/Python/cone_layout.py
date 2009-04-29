from vtk import *


reader = vtkXMLTreeReader()
reader.SetFileName("vtkclasses.xml")

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(reader.GetOutputPort())
view.SetVertexLabelArrayName("id")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("vertex id")
view.SetColorVertices(True)
view.SetLayoutStrategyToCone()
view.SetInteractionModeTo3D() # Left mouse button causes 3D rotate instead of zoom
view.SetLabelPlacementModeToLabelPlacer()

theme = vtkViewTheme.CreateMellowTheme()
theme.SetCellColor(.2,.2,.6)
theme.SetLineWidth(2)
theme.SetPointSize(10)
view.ApplyViewTheme(theme)
theme.FastDelete()

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)
view.GetRenderer().ResetCamera()
window.Render()
window.GetInteractor().Start()
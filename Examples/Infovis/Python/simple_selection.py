from vtk import *

from vtk import *

source = vtkRandomGraphSource()
source.SetNumberOfVertices(25)
source.SetStartWithTree(True)
source.SetIncludeEdgeWeights(True)

view1 = vtkGraphLayoutView()
view1.AddRepresentationFromInputConnection(source.GetOutputPort())
view1.SetColorVertices(True)
view1.SetEdgeColorArrayName("edge weight")
view1.SetColorEdges(True)
view1.SetLayoutStrategyToSimple2D()

view2 = vtkGraphLayoutView()
view2.AddRepresentationFromInputConnection(source.GetOutputPort())
view2.SetColorVertices(True)
view2.SetEdgeColorArrayName("edge weight")
view2.SetColorEdges(True)
view2.SetLayoutStrategyToTree()

# Create a annotation link and set both views to use it
annotationLink = vtkAnnotationLink()
view1.GetRepresentation(0).SetAnnotationLink(annotationLink)
view2.GetRepresentation(0).SetAnnotationLink(annotationLink)

updater = vtkViewUpdater()
updater.AddAnnotationLink(annotationLink)
updater.AddView(view1)
updater.AddView(view2)

theme = vtkViewTheme.CreateNeonTheme()
view1.ApplyViewTheme(theme)
view2.ApplyViewTheme(theme)
theme.FastDelete()


view1.GetRenderWindow().SetSize(600, 600)
view1.ResetCamera()
view1.Render()

view2.GetRenderWindow().SetSize(600, 600)
view2.ResetCamera()
view2.Render()

view1.GetInteractor().Start()

from vtk import *

reader2 = vtkXMLTreeReader()
reader2.SetFileName("vtkclasses.xml")
reader2.Update()

reader3 = vtkXMLTreeReader()
reader3.SetFileName("vtklibrary.xml")
reader3.Update()

view2 = vtkIcicleView()
view2.SetRepresentationFromInput(reader2.GetOutput())
view2.SetAreaSizeArrayName("size")
view2.SetAreaColorArrayName("vertex id")
view2.SetAreaLabelArrayName("id")
view2.SetAreaLabelVisibility(True)
view2.SetAreaHoverArrayName("id")
view2.SetRootWidth(40.)
view2.SetLayerThickness(2.)
#view2.UseGradientColoring(False)
view2.Update()

view3 = vtkIcicleView()
view3.SetRepresentationFromInput(reader3.GetOutput())
view3.SetAreaSizeArrayName("size")
view3.SetAreaColorArrayName("vertex id")
view3.SetAreaLabelArrayName("id")
view3.SetAreaLabelVisibility(True)
view3.SetAreaHoverArrayName("id")
view3.SetRootWidth(20.)
view3.Update()

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
view2.ApplyViewTheme(theme)
view3.ApplyViewTheme(theme)
theme.FastDelete()

view2.ResetCamera()
view3.ResetCamera()
view2.Render()
view3.Render()

view2.GetInteractor().Initialize()
view3.GetInteractor().Initialize()
view2.GetInteractor().Start()

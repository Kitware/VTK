from vtk import *

reader2 = vtkXMLTreeReader()
reader2.SetFileName("vtkclasses.xml")
reader2.Update()

reader3 = vtkXMLTreeReader()
reader3.SetFileName("vtklibrary.xml")
reader3.Update()

dummy = vtkTreeRingView()
view2 = vtkTreeRingView()
view2.SetRepresentationFromInput(reader2.GetOutput())
view2.SetRootAngles(160.,380.)
view2.SetSizeArrayName("size")
view2.SetColorArrayName("vertex id")
view2.SetLabelArrayName("id")
view2.SetHoverArrayName("id")
view2.SetSectorShrinkPercentage(0.02)
view2.SetLayoutStrategyToDefault()
view2.Update()

view3 = vtkTreeRingView()
view3.SetRepresentationFromInput(reader3.GetOutput())
view3.SetSizeArrayName("size")
view3.SetColorArrayName("vertex id")
view3.SetLabelArrayName("id")
view3.SetHoverArrayName("id")
view3.SetSectorShrinkPercentage(0.02)
view3.SetLayoutStrategyToReversed()
view3.Update()

# Apply a theme to the views
theme = vtkViewTheme.CreateMellowTheme()
view2.ApplyViewTheme(theme)
view3.ApplyViewTheme(theme)
theme.FastDelete()

win2 = vtkRenderWindow()
win3 = vtkRenderWindow()
dummy.SetupRenderWindow(win2)
view2.SetupRenderWindow(win2)
view3.SetupRenderWindow(win3)
view2.GetRenderer().ResetCamera()
view3.GetRenderer().ResetCamera()

win2.GetInteractor().Initialize()
win3.GetInteractor().Initialize()
win2.GetInteractor().Start()

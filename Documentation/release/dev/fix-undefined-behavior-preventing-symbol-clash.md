## Fix undefined behavior in widget due to vtkInteractionCallback symbol clash

Update the widget classes `vtkImplicitPlaneWidget2`, `vtkDisplaySizedImplicitPlaneWidget`
and  `vtkCoordinateFrameWidget` renaming `vtkInteractionCallback` generic class
to `vtk<WidgetName>InteractionCallback`.

Use of the same classname violated the one definition rule (ODR) and was causing
undefined behavior.

See https://discourse.slicer.org/t/transition-of-nightly-build-from-vtk-9-0-20201111-to-9-1-20220125/21669

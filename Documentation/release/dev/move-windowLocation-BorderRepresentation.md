# Move WindowLocation API in BorderRepresentation

The WindowLocation API has been moved from `vtkTextRepresentation` to `vtkBorderRepresentation`.
so it can be used by any class inheriting `vtkBorderRepresentation`. `vtkScalarBarWidget` is the only
subclass of `vtkBorderWidget` that uses `vtkAbstractWidget`'s GetProcessEvents() instead of `vtkBorderWidget`'s
to allow the move of the bar even when WindowLocation is not set to AnyLocation.

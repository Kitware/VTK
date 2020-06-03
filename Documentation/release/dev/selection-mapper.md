## Display selection from `vtkOpenGLPolyDataMapper`

It is possible now to set a `vtkSelection` to a mapper to display selected ids directly from the mapper.\
When a selection is present, a second pass is called automatically to display the selection.\
Selection is always displayed in wireframe and the color can be set using `vtkProperty::SetSelectionColor`.\
Currently, only selections by id or value are supported.

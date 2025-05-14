## vtkRandomAttributesFilter: Prevent deletion of actives attributes

1. When using `vtkRandomAttributesGenerator`, the filter used to replace the active attributes with the desired random sequence. This was due the usage of the method `SetAttribute()` that replaces internally the current active attribute. By calling `AddArray()` and then `SetActiveAttribute()`, the new data array is now appended as expected and still flagged as active.
2. All the random attributes that can be activated are now named correctly. Especially random normals and tcoords were not named before.

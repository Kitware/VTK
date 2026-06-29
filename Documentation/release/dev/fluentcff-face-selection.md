## vtkFluentCFFReader face selection

The `vtkFluentCFFReader` allows to read the faces information when using `ReadFacesOn()`.
You can now select which faces to read using the `GetFaceSelection()` `vtkDataArraySelection`
object to load only a subset of them. This is initialized during `UpdateInformation()`
with the available list of faces being enabled by default.

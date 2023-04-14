## Add vtkStaticFaceHashLinksTemplate

`vtkStaticFaceHashLinksTemplate` can help group faces of an unstructured grid with linear cells that have the same hash,
i.e. minimum point id. Once the faces are grouped by their hash, each group of faces in a hash can be evaluated to
eliminate the duplicates. This way, in the end you will only have external faces.

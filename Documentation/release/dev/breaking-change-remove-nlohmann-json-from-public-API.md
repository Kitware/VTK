# Remove usage of the nlohmannjson library from public facing API

The `nlohmann::json vtkAbstractArray::SerializeValues()` method is now removed
as the public exposure of VTK's vendored `nlohmannjson` library caused difficulty for downstream projects that link to a different nlohmannjson.

These methods were added in [vtk/vtk!11163](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11163) which was released in 9.4. That merge
request has now been reverted. These methods are removed (instead of deprecation) because deprecation would prevent fixing the problem.

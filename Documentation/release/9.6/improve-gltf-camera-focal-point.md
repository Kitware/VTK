## Improve glTF camera focal point

Introduced a `vtkGLTFImporter::GuessCamerasFocalPoints()` called at the end of the import to assign a less arbitrary focal point based on the imported actors.

This method moves the initial arbitrary focal point along the camera direction vector (to where the center of the scene bounds would project) so that we hopefully orbit around something if interacting with the camera later.

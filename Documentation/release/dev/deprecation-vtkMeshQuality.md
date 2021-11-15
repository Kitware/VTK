# Deprecation in vtkMeshQuality

In `vtkMeshQuality`, the mechanism to allow users to run the filter in a legacy mode is deprecated.
In particular, `CompatibilityMode`, which when turned on activates a legacy behavior of the filter,
is deprecated. `Volume`, which is linked to this mode, is deprecated as well.

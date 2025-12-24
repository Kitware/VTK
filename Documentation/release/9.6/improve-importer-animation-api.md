## Improve Importers animation API

vtkImporter::GetTemporalInformation has been reworked and now provides a simpler API
that does not require framerate.

A new API to configure interpolation between timesteps have been added.

The previous implementation has been deprecated.

The new API has been used in vtkGLTFImporter

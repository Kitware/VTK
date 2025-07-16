# Fixed export of textures in `vtkAlembicExporter`

Previously, textures were unintentionally being saved to the current working directory of the executable. Now, they are saved alongside the exported Alembic file.

set(classes
  vtkOggTheoraWriter)

vtk_module_add_module(VTK::IOOggTheora
  CLASSES ${classes})

# this is for legacy codes, should be dropped.
vtk_module_definitions(VTK::IOOggTheora
  INTERFACE VTK_HAS_OGGTHEORA_SUPPORT)

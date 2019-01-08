set(classes
  vtkOpenSlideReader)

vtk_module_add_module(VTK::DomainsMicroscopy
  CLASSES ${classes})

vtk_module_find_package(PACKAGE OpenSlide)
vtk_module_link(VTK::DomainsMicroscopy
  PUBLIC
    OpenSlide::OpenSlide)

set(classes
  vtkOMFReader)

set(private_headers_core
  core/OMFElement.h
  core/OMFFile.h
  core/OMFHelpers.h
  core/OMFProject.h)

set(private_classes
  core/OMFElement
  core/OMFFile
  core/OMFHelpers
  core/OMFProject)

vtk_module_add_module(VTK::IOOMF
  CLASSES           ${classes}
  PRIVATE_CLASSES   ${private_classes}
  PRIVATE_HEADERS   ${private_headers_core})
vtk_add_test_mangling(VTK::IOOMF)

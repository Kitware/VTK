set(classes
  vtkEnSight6BinaryReader
  vtkEnSight6Reader
  vtkEnSightGoldBinaryReader
  vtkEnSightGoldCombinedReader
  vtkEnSightGoldReader
  vtkEnSightMasterServerReader
  vtkEnSightReader
  vtkEnSightSOSGoldReader
  vtkGenericEnSightReader)

set(private_headers
    core/EnSightFile.h
    core/EnSightDataSet.h)

set(private_classes
    core/EnSightFile
    core/EnSightDataSet)

vtk_module_add_module(VTK::IOEnSight
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers})
vtk_add_test_mangling(VTK::IOEnSight)

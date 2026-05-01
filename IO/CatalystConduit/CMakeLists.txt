set(classes
  vtkConduitSource
  vtkConduitToDataObject
  vtkDataObjectToConduit
  vtkConduitArrayUtilities
)

set(private_classes)
if (TARGET VTK::AcceleratorsVTKmDataModel)
  set(private_classes_cuda vtkConduitArrayUtilitiesDevice)
  set(private_classes ${private_classes} ${private_classes_cuda})
  set(classes_cuda vtkConduitArrayUtilities ${private_classes_cuda})
endif()

vtk_module_add_module(VTK::IOCatalystConduit
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes})

_vtk_module_real_target(vtkm_accel_target VTK::IOCatalystConduit)
if (TARGET VTK::vtkviskores)
  vtk_add_viskores_device_target_information(${vtkm_accel_target})
endif()

vtk_add_test_mangling(VTK::IOCatalystConduit)
add_subdirectory(Catalyst)

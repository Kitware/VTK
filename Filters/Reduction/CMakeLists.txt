set(classes
)

if (VTK_MODULE_ENABLE_VTK_CommonImplicitArrays)
list(APPEND classes
  vtkToAffineArrayStrategy
  vtkToConstantArrayStrategy
  vtkToImplicitArrayFilter
  vtkToImplicitRamerDouglasPeuckerStrategy
  vtkToImplicitStrategy
  vtkToImplicitTypeErasureStrategy
)
endif()

vtk_module_add_module(VTK::FiltersReduction
  CLASSES ${classes}
)
vtk_add_test_mangling(VTK::FiltersReduction)

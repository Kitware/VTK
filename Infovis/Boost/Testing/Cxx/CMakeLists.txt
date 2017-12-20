find_package(Boost REQUIRED
  COMPONENTS serialization
  )
include_directories(${Boost_INCLUDE_DIRS})

vtk_add_test_cxx(vtkInfovisBoostCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestVariantSerialization.cxx
  )
vtk_test_cxx_executable(vtkInfovisBoostCxxTests tests)

vtk_module_link_libraries(vtkInfovisBoostCxxTests LINK_PRIVATE ${Boost_SERIALIZATION_LIBRARY})

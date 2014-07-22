find_package(Boost REQUIRED
  COMPONENTS serialization
  )
include_directories(${Boost_INCLUDE_DIRS})

vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestVariantSerialization.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)

vtk_module_link_libraries(${vtk-module}CxxTests LINK_PRIVATE ${Boost_SERIALIZATION_LIBRARY})

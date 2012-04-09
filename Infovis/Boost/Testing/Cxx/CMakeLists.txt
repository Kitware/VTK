find_package(Boost REQUIRED
  COMPONENTS serialization
  )
include_directories(${Boost_INCLUDE_DIRS})

vtk_module_test_executable(TestVariantSerialization TestVariantSerialization.cxx
  )

target_link_libraries(TestVariantSerialization ${Boost_SERIALIZATION_LIBRARY})

add_test(NAME ${vtk-module}-TestVariantSerialization COMMAND TestVariantSerialization)

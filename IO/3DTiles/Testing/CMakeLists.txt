vtk_module_test_data(
  Data/3DTiles/jacksonville-triangle.obj
  Data/3DTiles/jacksonville-tileset.json  
  Data/3DTiles/berlin-triangle.gml
  Data/3DTiles/berlin-tileset.json
  )

# We have to update proj to be able to run this test.
# Now it depends on proj 6 being installed on the host system.
find_package(PROJ4)
if (PROJ4_FOUND AND PROJ4_VERSION VERSION_GREATER 5)
  add_subdirectory(Cxx)
endif()

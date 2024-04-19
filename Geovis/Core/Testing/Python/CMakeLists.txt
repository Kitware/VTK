# Check that numpy is available
include(FindPythonModules)
find_python_module(numpy numpy_found)
if (numpy_found)
  vtk_add_test_python(
  TestGeoProjectionBounds.py,NO_DATA,NO_VALID,
  )
endif ()

vtk_add_test_python(
  TestGeoProjection.py,NO_DATA,NO_VALID,
  )

set_property(TEST VTK::GeovisCorePython-TestGeoProjection
  APPEND
  PROPERTY
    FAIL_REGULAR_EXPRESSION "Cannot find proj.db")

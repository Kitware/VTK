vtk_add_test_python(
  TestGeoProjection.py,NO_DATA,NO_VALID,
  )

get_test_property(TK::GeovisCorePython-TestGeoProjection
  FAIL_REGULAR_EXPRESSION fail_regex)
list(APPEND fail_regex "Cannot find proj.db")
set_tests_properties(VTK::GeovisCorePython-TestGeoProjection
  PROPERTIES FAIL_REGULAR_EXPRESSION "${fail_regex}")

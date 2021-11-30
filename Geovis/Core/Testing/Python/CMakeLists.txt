vtk_add_test_python(
  TestGeoProjection.py,NO_DATA,NO_VALID,
  )

set_property(TEST VTK::GeovisCorePython-TestGeoProjection
  APPEND
  PROPERTY
    FAIL_REGULAR_EXPRESSION "Cannot find proj.db")

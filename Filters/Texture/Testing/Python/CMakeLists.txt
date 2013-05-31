add_test_python(AutomaticPlaneGeneration.py Graphics)
add_test_python(cylMap.py Graphics)
add_test_python(socbal.py Graphics)
add_test_python(triangularTCoords.py Graphics)

if (VTK_DATA_ROOT)
  add_test_python(textureThreshold.py Graphics)
endif()

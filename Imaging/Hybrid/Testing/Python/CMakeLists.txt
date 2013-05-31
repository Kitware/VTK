add_test_python(iceCream.py Graphics)
add_test_python(shepards.py Graphics)
add_test_python(triangularTexture.py Graphics)

if (VTK_DATA_ROOT)
  add_test_python(genHead.py Graphics)
endif()

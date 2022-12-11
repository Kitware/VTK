vtk_add_test_python(
  TestMathTextActor.py
  TestMathTextActor3D.py
  TestRenderString.py
  )

if (TARGET VTK::PythonContext2D)
  vtk_add_test_python(
    TestStringToPath.py
  )
endif ()

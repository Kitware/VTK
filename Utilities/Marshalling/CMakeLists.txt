find_package(Python3 QUIET COMPONENTS Interpreter)
if (Python3_FOUND)
  set(_vtk_testing_python_exe "$<TARGET_FILE:Python3::Interpreter>")
  vtk_add_test_python(
    TestMarshalHintHeaderConsistency,marshal_macro_annotate_headers.py,NO_DATA,NO_OUTPUT,NO_VALID --test)
endif ()

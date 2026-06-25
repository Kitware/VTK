  vtk_module_test_data(
    Data/vtk-uns-grid-2.json
    Data/tris-blocks-time.bp
    Data/tris-blocks-time.bp.dir/,REGEX:.*
    Data/cartesian-attr.bp
    Data/cartesian-attr.bp.dir/,REGEX:.*
    Data/groups.bp/,REGEX:.*
    Data/gs.bp/,REGEX:.*)

vtk_add_test_python(
   TestFidesBasic.py,NO_VALID,NO_OUTPUT
   TestFidesWriterBasic.py,NO_DATA,NO_VALID
   TestFidesWriterTime.py,NO_DATA,NO_VALID
)

if (VTK_WRAP_PYTHON AND TARGET VTK::conduit_python)
  get_target_property(_conduit_python_path VTK::conduit_python VTK_CONDUIT_PYTHON_MODULE_DIR)

  if (_conduit_python_path)
    include(FindPythonModules)
    find_python_module(numpy numpy_found)
    if (numpy_found)
      vtk_add_test_python(TestFidesConduitNode.py NO_VALID)

      # Append to the test's env mods to update PYTHONPATH
      set_property(TEST "VTK::IOFidesPython-TestFidesConduitNode" APPEND
        PROPERTY
          ENVIRONMENT_MODIFICATION
            "PYTHONPATH=path_list_append:${_conduit_python_path}")

      # Print a warning for older cmake versions
      if (CMAKE_VERSION VERSION_LESS "3.22")
        message(WARNING
          "CMake 3.22+ is required to reliably set up the environment for Python "
          "tests needing Conduit. TestFidesConduitNode may fail to import conduit and be skipped.")
      endif ()
    endif()
  endif ()
elseif (VTK_WRAP_PYTHON)
  vtk_add_test_python(TestFidesReaderNoConduit.py NO_VALID)
endif()

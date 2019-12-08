if (PYTHON_EXECUTABLE AND NOT VTK_BUILD_SHARED_LIBS)
  # For now, this test is only added for static builds.
  # In theory, we can make it work in shared builds too, it just needs more work to figure out
  # what variables need to be set on each platform correctly.
  add_test(NAME vtkPythonInterpreterPythonTests-TestImportVanillaInterpreter
    COMMAND "${PYTHON_EXECUTABLE}" -m vtk)
  set_tests_properties("vtkPythonInterpreterPythonTests-TestImportVanillaInterpreter"
    PROPERTIES
      LABELS "${_vtk_build_test_labels}"
      FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
      # This must match VTK_SKIP_RETURN_CODE in vtkTestingObjectFactory.h
      SKIP_RETURN_CODE 125
      ENVIRONMENT
      "PYTHONPATH=${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/_vtk.zip:${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/"
    )
endif()

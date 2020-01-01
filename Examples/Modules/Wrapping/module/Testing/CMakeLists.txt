if (TARGET VTK::vtkpython)
  add_test(NAME "import_wrappable"
    COMMAND
      "$<TARGET_FILE:VTK::vtkpython>"
      "${CMAKE_CURRENT_LIST_DIR}/import_wrappable.py")
  set_property(TEST "import_wrappable" APPEND
    PROPERTY
      ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/${python_destination}")
  if (WIN32)
    set_property(TEST "import_wrappable" APPEND
      PROPERTY
        ENVIRONMENT "PATH=$ENV{PATH}\;${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
  endif ()
endif ()

vtk_module_export_info()
set(Module_HDRS
  vtkTestDriver.h
  vtkTestingColors.h
  vtkTestUtilities.h
  )
if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES ${Module_HDRS}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
    COMPONENT Development
    )
endif()

# -----------------------------------------------------------------------------
# Macro vtk_tests() takes a list of cxx files which will be driven by the modules
# test driver. This helps reduce a lot of boiler place code in each module
macro(vtk_tests)
  create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
    ${ARGV}
    EXTRA_INCLUDE vtkTestDriver.h)

  vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

  set(TestsToRun ${Tests})
  list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

  # Add all the executables
  foreach(test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    if(VTK_DATA_ROOT)
      add_test(NAME ${vtk-module}Cxx-${TName}
        COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/${vtk-module}/${TName}.png)
    else()
      add_test(NAME ${vtk-module}Cxx-${TName}
        COMMAND ${vtk-module}CxxTests ${TName}
        ${${TName}_ARGS})
    endif()
  endforeach()
endmacro(vtk_tests)

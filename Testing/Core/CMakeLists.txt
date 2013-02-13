vtk_module_export_info()
set(Module_HDRS
  vtkTestDriver.h
  vtkTestErrorObserver.h
  vtkTestingColors.h
  vtkTestUtilities.h
  )
if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES ${Module_HDRS}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
    COMPONENT Development
    )
endif()

include(vtkTestingMacros)

########################################################################
#gather and package information for a test (TestSystemInformation)
#so that we can inspect the dashboard and see what each submitter
#was testing exactly
set(_modules ${VTK_MODULES_ENABLED})
set(_modules_not_tests)
list(SORT _modules)
foreach(vtk-module ${_modules})
  if(NOT ${vtk-module}_IS_TEST)
    set(_modules_not_tests "${_modules_not_tests}\"${vtk-module}\",\n")
  endif()
endforeach()

set(_modules ${VTK_MODULES_ENABLED})
set(_modules_system)
set(_modules_system_libs)
list(SORT _modules)
foreach(vtk-module ${_modules})
  string(TOUPPER "${vtk-module}" _upper)
  if(${VTK_USE_SYSTEM_${_upper}})
     set(_modules_system "${_modules_system}VTK_USE_SYSTEM_${_upper},\n")
     set(_modules_system_libs "${_modules_system_libs}\"${_upper}_LIBRARY\",\n")
   endif()
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ConfigSummary.txt.in ${PROJECT_BINARY_DIR}/Testing/Temporary/ConfigSummary.txt)

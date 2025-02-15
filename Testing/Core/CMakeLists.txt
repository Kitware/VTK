set(headers
  vtkEmscriptenTestUtilities.h
  vtkPermuteOptions.h
  vtkTestDriver.h
  vtkTestErrorObserver.h
  vtkTestingColors.h
  vtkWindowsTestUtilities.h)

set(sources
  vtkEmscriptenTestUtilities.cxx)

set(templates
  vtkTestConditionals.txx)
set(classes
  vtkTestUtilities)

vtk_module_add_module(VTK::TestingCore
  CLASSES   ${classes}
  HEADERS   ${headers}
  SOURCES   ${sources}
  TEMPLATES ${templates})

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  vtk_module_link_options(VTK::TestingCore
    INTERFACE
      "$<BUILD_INTERFACE:--js-library=${CMAKE_CURRENT_SOURCE_DIR}/vtkEmscriptenTestUtilities.js>"
      "$<INSTALL_INTERFACE:--js-library=$<INSTALL_PREFIX>/${_vtk_build_LIBRARY_DESTINATION}/vtkEmscriptenTestUtilities.js>")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtkEmscriptenTestUtilities.js"
    DESTINATION ${_vtk_build_LIBRARY_DESTINATION})
endif ()

# Write out a summary of the configuration for use in the
# `TestSystemInformation` test.
set(system_info_file "${CMAKE_BINARY_DIR}/Testing/Temporary/ConfigSummary.txt")
file(WRITE "${system_info_file}")
set(built_modules "${vtk_modules}")
list(SORT built_modules)
foreach (module IN LISTS built_modules)
  get_property(is_third_party GLOBAL
    PROPERTY "_vtk_module_${module}_third_party")
  if (is_third_party)
    if (VTK_MODULE_USE_EXTERNAL_${module})
      set(module_text "${module} (external)")
    else ()
      set(module_text "${module} (internal)")
    endif ()
  else ()
    set(module_text "${module}")
  endif ()
  file(APPEND "${system_info_file}"
    "${module_text}\n")
endforeach ()
vtk_add_test_mangling(VTK::TestingCore)

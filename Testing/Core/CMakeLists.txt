set(headers
  vtkPermuteOptions.h
  vtkTestDriver.h
  vtkTestErrorObserver.h
  vtkTestingColors.h
  vtkTestUtilities.h
  vtkWindowsTestUtilities.h)

set(templates
  vtkTestConditionals.txx)

vtk_module_add_module(VTK::TestingCore
  HEADERS   ${headers}
  TEMPLATES ${templates}
  HEADER_ONLY)

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

# Load up the CMake variables we need.
vtk_module_impl()
vtk_module_config(${vtk-module} ${${vtk-module}_DEPENDS})

# Placeholders
set(extra_deps)
set(compile_defs)

# Add our test executables.
add_executable(TimingTests MACOSX_BUNDLE
  TimingTests.cxx
  vtkRenderTimings.cxx
  )
target_link_libraries(TimingTests ${${vtk-module}_LIBRARIES})
set_property(TARGET TimingTests APPEND PROPERTY
  COMPILE_DEFINITIONS "${${vtk-module}_DEFINITIONS}")
if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  list(APPEND compile_defs VTK_OPENGL2)
endif()

# Chemistry target
if(TARGET vtkDomainsChemistry)
  if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
    list(APPEND extra_deps vtkDomainsChemistryOpenGL2)
  else()
    list(APPEND extra_deps vtkDomainsChemistry)
  endif()
  list(APPEND compile_defs HAVE_CHEMISTRY)
  include_directories(${VTK_SOURCE_DIR}/Domains/Chemistry
    ${VTK_BINARY_DIR}/Domains/Chemistry)
endif()

if(compile_defs)
  set_source_files_properties(TimingTests.cxx APPEND PROPERTIES
    COMPILE_DEFINITIONS "${compile_defs}")
endif()

if (extra_deps)
  target_link_libraries(TimingTests ${extra_deps})
endif()

add_executable(GLBenchmarking MACOSX_BUNDLE
  GLBenchmarking.cxx
  )
target_link_libraries(GLBenchmarking ${${vtk-module}_LIBRARIES})
set_property(TARGET GLBenchmarking APPEND PROPERTY
  COMPILE_DEFINITIONS "${${vtk-module}_DEFINITIONS}")

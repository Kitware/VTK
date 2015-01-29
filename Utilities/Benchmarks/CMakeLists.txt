# Load up the CMake variables we need.
vtk_module_impl()
vtk_module_config(${vtk-module} ${${vtk-module}_DEPENDS})

# Add our test executables.
add_executable(TimingTests MACOSX_BUNDLE TimingTests.cxx vtkRenderTimings.cxx)
target_link_libraries(TimingTests ${${vtk-module}_LIBRARIES})
set_source_files_properties(TimingTests.cxx APPEND PROPERTIES
  COMPILE_DEFINITIONS "${${vtk-module}_DEFINITIONS}")
if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set(compile_defs VTK_OPENGL2)
endif()

# Chemistry target
if(TARGET vtkDomainsChemistry)
  if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
    set(extra_deps vtkDomainsChemistryOpenGL2)
  else()
    set(extra_deps vtkDomainsChemistry)
  endif()
  list(APPEND compile_defs HAVE_CHEMISTRY)
  include_directories(${VTK_SOURCE_DIR}/Domains/Chemistry
    ${VTK_BINARY_DIR}/Domains/Chemistry)
endif()

# Volume target
if(TARGET vtkRenderingVolumeOpenGL2)
  if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
    list(APPEND extra_deps vtkRenderingVolumeOpenGL2)
  else()
    list(APPEND extra_deps vtkRenderingVolumeOpenGL)
  endif()
  list(APPEND compile_defs HAVE_VOLUME)
  include_directories(
    ${VTK_SOURCE_DIR}/Rendering/Volume
    ${VTK_SOURCE_DIR}/Rendering/VolumeOpenGL2
    ${VTK_BINARY_DIR}/Rendering/VolumeOpenGL2
    ${VTK_BINARY_DIR}/Rendering/VolumeOpenGL2)
endif()

set_source_files_properties(TimingTests.cxx APPEND PROPERTIES
  COMPILE_DEFINITIONS "${compile_defs}")
target_link_libraries(TimingTests ${extra_deps})

add_executable(GLBenchmarking MACOSX_BUNDLE GLBenchmarking.cxx)
target_link_libraries(GLBenchmarking ${${vtk-module}_LIBRARIES})
set_source_files_properties(GLBenchmarking.cxx APPEND PROPERTIES
  COMPILE_DEFINITIONS "${${vtk-module}_DEFINITIONS}")

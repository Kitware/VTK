if (NOT (DEFINED vtk_cmake_dir AND
         DEFINED vtk_cmake_build_dir AND
         DEFINED vtk_cmake_destination AND
         DEFINED vtk_modules))
  message(FATAL_ERROR
    "vtkInstallCMakePackageCompileTools is missing input variables.")
endif ()

configure_file(
  "${vtk_cmake_dir}/vtkcompiletools-config.cmake.in"
  "${vtk_cmake_build_dir}/vtkcompiletools-config.cmake"
  @ONLY)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${vtk_cmake_build_dir}/vtkcompiletools-config-version.cmake"
  VERSION "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}"
  COMPATIBILITY AnyNewerVersion)

# For convenience, a package is written to the top of the build tree. At some
# point, this should probably be deprecated and warn when it is used.
file(GENERATE
  OUTPUT  "${CMAKE_BINARY_DIR}/vtkcompiletools-config.cmake"
  CONTENT "include(\"${vtk_cmake_build_dir}/vtkcompiletools-config.cmake\")\n")
configure_file(
  "${vtk_cmake_build_dir}/vtkcompiletools-config-version.cmake"
  "${CMAKE_BINARY_DIR}/vtkcompiletools-config-version.cmake"
  COPYONLY)

install(
  FILES       "${vtk_cmake_build_dir}/vtkcompiletools-config.cmake"
              "${vtk_cmake_build_dir}/vtkcompiletools-config-version.cmake"
  DESTINATION "${vtk_cmake_destination}"
  COMPONENT   "development")

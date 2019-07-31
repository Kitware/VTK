if (NOT (DEFINED vtk_cmake_dir AND
         DEFINED vtk_cmake_build_dir AND
         DEFINED vtk_cmake_destination AND
         DEFINED vtk_modules))
  message(FATAL_ERROR
    "vtkInstallCMakePackage is missing input variables.")
endif ()

set(vtk_all_components)
foreach (vtk_module IN LISTS vtk_modules)
  string(REPLACE "VTK::" "" vtk_component "${vtk_module}")
  list(APPEND vtk_all_components
    "${vtk_component}")
endforeach ()

if (TARGET "VTK::vtkm")
  set(vtk_has_vtkm ON)
else ()
  set(vtk_has_vtkm OFF)
endif ()

_vtk_module_write_import_prefix("${vtk_cmake_build_dir}/vtk-prefix.cmake" "${vtk_cmake_destination}")

set(vtk_prefix_paths)

set(vtk_python_version "")
if (VTK_WRAP_PYTHON)
  set(vtk_python_version "${VTK_PYTHON_VERSION}")
endif ()

configure_file(
  "${vtk_cmake_dir}/vtk-config.cmake.in"
  "${vtk_cmake_build_dir}/vtk-config.cmake"
  @ONLY)

if (NOT DEFINED VTK_RELOCATABLE_INSTALL)
  option(VTK_RELOCATABLE_INSTALL "Do not embed hard-coded paths into the install" ON)
  mark_as_advanced(VTK_RELOCATABLE_INSTALL)
endif ()
if (VTK_RELOCATABLE_INSTALL)
  set(vtk_prefix_paths)
endif ()

configure_file(
  "${vtk_cmake_dir}/vtk-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vtk-config.cmake"
  @ONLY)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${vtk_cmake_build_dir}/vtk-config-version.cmake"
  VERSION "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}"
  COMPATIBILITY AnyNewerVersion)

# For convenience, a package is written to the top of the build tree. At some
# point, this should probably be deprecated and warn when it is used.
file(GENERATE
  OUTPUT  "${CMAKE_BINARY_DIR}/vtk-config.cmake"
  CONTENT "include(\"${vtk_cmake_build_dir}/vtk-config.cmake\")\n")
configure_file(
  "${vtk_cmake_build_dir}/vtk-config-version.cmake"
  "${CMAKE_BINARY_DIR}/vtk-config-version.cmake"
  COPYONLY)

set(vtk_cmake_module_files
  FindADIOS1.cmake
  Finddouble-conversion.cmake
  FindEigen3.cmake
  FindEXPAT.cmake
  FindFFMPEG.cmake
  FindFontConfig.cmake
  FindFreetype.cmake
  FindGL2PS.cmake
  FindGLEW.cmake
  FindJsonCpp.cmake
  FindLibHaru.cmake
  FindLibPROJ.cmake
  FindLibXml2.cmake
  FindLZ4.cmake
  FindLZMA.cmake
  Findmpi4py.cmake
  FindMySQL.cmake
  FindNetCDF.cmake
  FindODBC.cmake
  FindOGG.cmake
  FindOpenMP.cmake
  FindOpenSlide.cmake
  FindOpenVR.cmake
  FindOSMesa.cmake
  FindPostgreSQL.cmake
  FindTBB.cmake
  FindTHEORA.cmake
  Findutf8cpp.cmake

  vtkCMakeBackports.cmake
  vtkDetectLibraryType.cmake
  vtkEncodeString.cmake
  vtkExternalData.cmake
  vtkHashSource.cmake
  vtkModule.cmake
  vtkModuleGraphviz.cmake
  vtkModuleJson.cmake
  vtkModuleTesting.cmake
  vtkModuleWrapJava.cmake
  vtkModuleWrapPython.cmake
  vtkObjectFactory.cmake
  vtkObjectFactory.cxx.in
  vtkObjectFactory.h.in
  vtkTestingDriver.cmake
  vtkTestingRenderingDriver.cmake
  vtkTopologicalSort.cmake)
set(vtk_cmake_patch_files
  patches/3.7/FindPNG.cmake
  patches/3.7/FindTIFF.cmake
  patches/3.7/exportheader.cmake.in
  patches/3.7/GenerateExportHeader.cmake
  patches/3.10/FindMPI/fortranparam_mpi.f90.in
  patches/3.10/FindMPI/libver_mpi.c
  patches/3.10/FindMPI/libver_mpi.f90.in
  patches/3.10/FindMPI/mpiver.f90.in
  patches/3.10/FindMPI/test_mpi.c
  patches/3.10/FindMPI/test_mpi.f90.in
  patches/3.10/FindMPI.cmake
  patches/3.13/FindZLIB.cmake
  patches/99/FindGDAL.cmake
  patches/99/FindHDF5.cmake
  patches/99/FindJPEG.cmake
  patches/99/FindOpenGL.cmake
  patches/99/FindPython/Support.cmake
  patches/99/FindPython2.cmake
  patches/99/FindPython3.cmake
  patches/99/FindSQLite3.cmake
  patches/99/FindX11.cmake)

set(vtk_cmake_files_to_install)
foreach (vtk_cmake_module_file IN LISTS vtk_cmake_module_files vtk_cmake_patch_files)
  configure_file(
    "${vtk_cmake_dir}/${vtk_cmake_module_file}"
    "${vtk_cmake_build_dir}/${vtk_cmake_module_file}"
    COPYONLY)
  list(APPEND vtk_cmake_files_to_install
    "${vtk_cmake_module_file}")
endforeach ()

include(vtkInstallCMakePackageHelpers)
if (NOT VTK_RELOCATABLE_INSTALL)
  list(APPEND vtk_cmake_files_to_install
    "${vtk_cmake_build_dir}/vtk-find-package-helpers.cmake")
endif ()

foreach (vtk_cmake_file IN LISTS vtk_cmake_files_to_install)
  if (IS_ABSOLUTE "${vtk_cmake_file}")
    file(RELATIVE_PATH vtk_cmake_subdir_root "${vtk_cmake_build_dir}" "${vtk_cmake_file}")
    get_filename_component(vtk_cmake_subdir "${vtk_cmake_subdir_root}" DIRECTORY)
    set(vtk_cmake_original_file "${vtk_cmake_file}")
  else ()
    get_filename_component(vtk_cmake_subdir "${vtk_cmake_file}" DIRECTORY)
    set(vtk_cmake_original_file "${vtk_cmake_dir}/${vtk_cmake_file}")
  endif ()
  install(
    FILES       "${vtk_cmake_original_file}"
    DESTINATION "${vtk_cmake_destination}/${vtk_cmake_subdir}"
    COMPONENT   "development")
endforeach ()

install(
  FILES       "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vtk-config.cmake"
              "${vtk_cmake_build_dir}/vtk-config-version.cmake"
              "${vtk_cmake_build_dir}/vtk-prefix.cmake"
  DESTINATION "${vtk_cmake_destination}"
  COMPONENT   "development")

install(
  FILES       "${CMAKE_CURRENT_LIST_DIR}/../Copyright.txt"
  DESTINATION "${CMAKE_INSTALL_DOCDIR}"
  COMPONENT   "license")

vtk_module_export_find_packages(
  CMAKE_DESTINATION "${vtk_cmake_destination}"
  FILE_NAME         "VTK-vtk-module-find-packages.cmake"
  MODULES           ${vtk_modules})

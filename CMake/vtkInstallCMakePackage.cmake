if (NOT (DEFINED vtk_cmake_dir AND
         DEFINED vtk_cmake_build_dir AND
         DEFINED vtk_cmake_destination AND
         DEFINED vtk_modules))
  message(FATAL_ERROR
    "vtkInstallCMakePackage is missing input variables.")
endif ()

set(vtk_has_catalyst 0)
set(vtk_catalyst_directory "")
if (TARGET VTK::catalyst-vtk)
  set(vtk_has_catalyst 1)
  get_property(vtk_catalyst_directory GLOBAL
    PROPERTY vtk_catalyst_directory)
endif ()

string(REPLACE "VTK::" "" vtk_all_components "${vtk_modules}")
# Components that are not modules.
set(_vtk_non_module_components
  WrapHierarchy

  vtkbuild

  vtkpython
  pvtkpython
  WrapPython
  WrapPythonInit

  vtkjava
  ParseJava
  WrapJava

  vtkWebAssemblyTestLinkOptions)
foreach (_vtk_non_module_component IN LISTS _vtk_non_module_components)
  if (TARGET "VTK::${_vtk_non_module_component}")
    list(APPEND vtk_all_components
      "${_vtk_non_module_component}")
  endif ()
endforeach ()

if (TARGET "VTK::vtkviskores")
  set(vtk_has_viskores ON)
  set(vtk_has_vtkm ON)
else ()
  set(vtk_has_viskores OFF)
  set(vtk_has_vtkm OFF)
endif ()

get_property(vtk_smp_backends GLOBAL
  PROPERTY _vtk_smp_backends)

_vtk_module_write_import_prefix("${vtk_cmake_build_dir}/vtk-prefix.cmake" "${vtk_cmake_destination}")

set(vtk_python_version "")
if (VTK_WRAP_PYTHON)
  set(vtk_python_version "3")
endif ()

set(vtk_has_qml 0)
if (TARGET VTK::GUISupportQtQuick)
  set(vtk_has_qml 1)
endif ()

get_property(vtk_opengl_preference_set GLOBAL
  PROPERTY _vtk_opengl_preference
  SET)
if (vtk_opengl_preference_set)
  get_property(vtk_opengl_preference GLOBAL
    PROPERTY _vtk_opengl_preference)
else ()
  set(vtk_opengl_preference "")
endif ()

configure_file(
  "${vtk_cmake_dir}/vtk-config.cmake.in"
  "${vtk_cmake_build_dir}/vtk-config.cmake"
  @ONLY)

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
  Finddouble-conversion.cmake
  FindDirectX.cmake
  FindEigen3.cmake
  FindEXPAT.cmake
  FindExprTk.cmake
  FindFFMPEG.cmake
  FindFontConfig.cmake
  FindFreetype.cmake
  FindGL2PS.cmake
  FindJOGL.cmake
  FindJsonCpp.cmake
  FindLibHaru.cmake
  FindLibPROJ.cmake
  FindLibXml2.cmake
  FindLZ4.cmake
  FindLZMA.cmake
  FindMEMKIND.cmake
  Findmpi4py.cmake
  FindMySQL.cmake
  FindNetCDF.cmake
  FindODBC.cmake
  FindOGG.cmake
  FindOpenSlide.cmake
  FindOpenVR.cmake
  FindOpenXRRemoting.cmake
  FindOSMesa.cmake
  FindPEGTL.cmake
  FindTBB.cmake
  FindTHEORA.cmake
  Findutf8cpp.cmake
  FindCGNS.cmake
  FindzSpace.cmake

  vtkCMakeBackports.cmake
  vtkDetectLibraryType.cmake
  vtkEncodeString.cmake
  vtkHashSource.cmake
  vtkMobileDevices.cmake
  vtkModule.cmake
  vtkModuleGraphviz.cmake
  vtkModuleJson.cmake
  vtkModuleSerialization.cmake
  vtkModuleTesting.cmake
  vtkModuleWrapJava.cmake
  vtkModuleWrapPython.cmake
  vtkObjectFactory.cmake
  vtkObjectFactory.cxx.in
  vtkObjectFactory.h.in
  vtkSerializationLibrariesRegistrar.cxx.in
  vtkSerializationLibraryRegistrar.cxx.in
  vtkSerializationLibraryRegistrar.h.in
  vtkTestingDriver.cmake
  vtkTestingRenderingDriver.cmake
  vtkTopologicalSort.cmake
  vtk-use-file-compat.cmake
  vtk-use-file-deprecated.cmake
  vtk-use-file-error.cmake)
set(vtk_cmake_patch_files
  patches/3.13/FindZLIB.cmake
  patches/3.16/FindPostgreSQL.cmake
  patches/3.19/FindJPEG.cmake
  patches/3.19/FindLibArchive.cmake
  patches/3.19/FindSQLite3.cmake
  patches/3.20/FindGDAL.cmake
  patches/3.22/FindMPI/fortranparam_mpi.f90.in
  patches/3.22/FindMPI/libver_mpi.c
  patches/3.22/FindMPI/libver_mpi.f90.in
  patches/3.22/FindMPI/mpiver.f90.in
  patches/3.22/FindMPI/test_mpi.c
  patches/3.22/FindMPI/test_mpi.f90.in
  patches/3.22/FindMPI.cmake
  patches/3.23/FindPython/Support.cmake
  patches/3.23/FindPython3.cmake
  patches/99/FindHDF5.cmake
  patches/99/FindOpenGL.cmake
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

vtk_module_export_find_packages(
  CMAKE_DESTINATION "${vtk_cmake_destination}"
  FILE_NAME         "VTK-vtk-module-find-packages.cmake"
  MODULES           ${vtk_modules})

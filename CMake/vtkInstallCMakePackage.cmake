if (NOT (DEFINED vtk_cmake_dir AND
         DEFINED vtk_cmake_build_dir AND
         DEFINED vtk_cmake_destination AND
         DEFINED vtk_modules))
  message(FATAL_ERROR
    "vtkInstallCMakePackage is missing input variables.")
endif ()

if (TARGET "VTK::vtkm")
  set(vtk_has_vtkm ON)
else ()
  set(vtk_has_vtkm OFF)
endif ()

configure_file(
  "${vtk_cmake_dir}/vtk-config.cmake.in"
  "${vtk_cmake_build_dir}/vtk-config.cmake"
  @ONLY)
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${vtk_cmake_build_dir}/vtk-config-version.cmake"
  VERSION "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}"
  COMPATIBILITY SameMinorVersion)

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
  FindOptiX.cmake
  FindOSMesa.cmake
  FindPostgreSQL.cmake
  FindPythonModules.cmake
  FindTBB.cmake
  FindTHEORA.cmake

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
  patches/99/FindJPEG.cmake
  patches/99/FindOpenGL.cmake
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

foreach (vtk_cmake_file IN LISTS vtk_cmake_files_to_install)
  get_filename_component(subdir "${vtk_cmake_file}" DIRECTORY)
  install(
    FILES       "${vtk_cmake_dir}/${vtk_cmake_file}"
    DESTINATION "${vtk_cmake_destination}/${subdir}"
    COMPONENT   "development")
endforeach ()

install(
  FILES       "${vtk_cmake_build_dir}/vtk-config.cmake"
              "${vtk_cmake_build_dir}/vtk-config-version.cmake"
  DESTINATION "${vtk_cmake_destination}"
  COMPONENT   "development")

install(
  FILES       "${VTK_SOURCE_DIR}/Copyright.txt"
  DESTINATION "${CMAKE_INSTALL_DOCDIR}"
  COMPONENT   "license")

vtk_module_export_find_packages(
  CMAKE_DESTINATION "${vtk_cmake_destination}"
  FILE_NAME         "VTK-vtk-module-find-packages.cmake"
  MODULES           ${vtk_modules})

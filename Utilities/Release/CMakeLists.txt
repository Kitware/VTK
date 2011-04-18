
#------------------------------------------------------------------------------
# Add install rules for required system runtimes such as MSVCRxx.dll
set (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP ON)
include(InstallRequiredSystemLibraries)
if (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
  install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
    DESTINATION ${VTK_INSTALL_BIN_DIR_CM24}
    PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
    COMPONENT RuntimeLibraries)

  if(VTK_WRAP_PYTHON)
    # Install the runtimes to the lib dir as well since python modules are
    # installed in that directory and the manifest files need to present there as
    # well.
    install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
      DESTINATION ${VTK_INSTALL_LIB_DIR_CM24}/site-packages/vtk
      PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
      COMPONENT RuntimeLibraries)
  endif(VTK_WRAP_PYTHON)
endif (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)


install(FILES ${VTK_SOURCE_DIR}/vtkLogo.ico
  DESTINATION ${VTK_INSTALL_BIN_DIR_CM24}
  PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
  COMPONENT RuntimeExecutables)

if(WIN32)
  set(vtk_runtime_dir ${VTK_INSTALL_BIN_DIR_CM24})
else()
  set(vtk_runtime_dir ${VTK_INSTALL_LIB_DIR_CM24})
endif()

# install core python distribution
if(NOT APPLE AND VTK_WRAP_PYTHON)
  get_filename_component(PYTHON_BIN_DIR "${PYTHON_EXECUTABLE}" PATH)
  get_filename_component(PYTHON_LIB_DIR "${PYTHON_LIBRARY}" PATH)
  if(WIN32)
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(standard_lib=1)"
                  OUTPUT_VARIABLE PYTHON_LIBS_DIR
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

    file(GLOB python_core_modules "${PYTHON_BIN_DIR}/DLLs/*.pyd")

    install(FILES ${python_core_modules}
        DESTINATION ${VTK_INSTALL_BIN_DIR_CM24}
        COMPONENT RuntimeLibraries)
  else(WIN32)
    get_filename_component(PYTHON_LIBS_DIR "${PYTHON_LIBRARY}" PATH)
    file(TO_CMAKE_PATH ${PYTHON_LIBS_DIR} PYTHON_LIBS_DIR)

    # install pyconfig.h
    string(REGEX MATCH "python[0-9].[0-9]" py_inc_dir ${PYTHON_INCLUDE_DIR})
    install(FILES ${PYTHON_INCLUDE_DIR}/pyconfig.h
            DESTINATION ${vtk_runtime_dir}/include/${py_inc_dir}
            COMPONENT RuntimeLibraries)
  endif(WIN32)

  file(TO_CMAKE_PATH ${PYTHON_LIBS_DIR} PYTHON_LIBS_DIR)
  install(DIRECTORY ${PYTHON_LIBS_DIR}
          DESTINATION ${vtk_runtime_dir}
          USE_SOURCE_PERMISSIONS
          COMPONENT RuntimeLibraries)
endif()

set(_dir "${VTK_INSTALL_LIB_DIR_CM24}")
if(WIN32)
  set(_dir "bin")
endif(WIN32)


list(APPEND lib_search_dir "\${CMAKE_INSTALL_PREFIX}/bin" "\${CMAKE_INSTALL_PREFIX}/${VTK_INSTALL_LIB_DIR_CM24}")
list(APPEND lib_search_dir "${VTK_BINARY_DIR}/bin")

if(WIN32)
  list(APPEND lib_search_dir "${VTK_BINARY_DIR}/bin/Release")
endif()

if(VTK_WRAP_TCL)
  get_filename_component(TCL_BIN_DIR "${TCL_TCLSH}" PATH)
  get_filename_component(TCL_LIB_DIR "${TCL_LIBRARY}" PATH)
  get_filename_component(TK_LIB_DIR "${TK_LIBRARY}" PATH)
  list(APPEND lib_search_dir ${TCL_BIN_DIR} ${TCL_BIN_DIR} ${TK_LIB_DIR})
endif()

if(VTK_WRAP_PYTHON)
  list(APPEND lib_search_dir ${PYTHON_BIN_DIR} ${PYTHON_LIB_DIR})
  if(WIN32)
    list(APPEND lib_search_dir ${PYTHON_BIN_DIR}/DLLs)
  endif()
endif()

set(APPS)

if(VTK_WRAP_PYTHON)
  list(APPEND APPS "\${CMAKE_INSTALL_PREFIX}/${_dir}/vtkpython${CMAKE_EXECUTABLE_SUFFIX}")  # paths to executables
endif()

if(VTK_WRAP_TCL)
  #list(APPEND APPS "\${CMAKE_INSTALL_PREFIX}/${_dir}/vtk${CMAKE_EXECUTABLE_SUFFIX}")
endif()

install(CODE "
  include(BundleUtilities)
  fixup_bundle(\"${APPS}\"   \"${lib_names}\"   \"${lib_search_dir}\")"
  COMPONENT RuntimeLibraries)

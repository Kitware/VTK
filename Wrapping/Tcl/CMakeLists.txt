# Defined local options to add to VTK Tcl interpreter
#
# Example of such options are:
#   VTK_EXTRA_TCL_WRAPPING_CODE which should define extra code that will be
#   added to vtkTkAppInitConfigure.h. This code may include something like
#
#
#   extern "C" int Vtkmyspecialtcl_Init(Tcl_Interp *interp);
#   #define VTK_EXTRA_TCL_INIT \
#     if (Vtkmyspecialtcl_Init(interp) == TCL_ERROR) \
#       {
#       return TCL_ERROR;
#       }
#
#   and
#
#   SET(VTK_EXTRA_TCL_LIBRARIES ${VTK_EXTRA_TCL_LIBRARIES} vtkMySpecialTCL)

CONFIGURE_FILE(
  ${VTK_SOURCE_DIR}/Wrapping/Tcl/vtkTkAppInitConfigure.h.in
  ${VTK_BINARY_DIR}/Wrapping/Tcl/vtkTkAppInitConfigure.h
  @ONLY IMMEDIATE
)

get_property(VTK_TCL_WRAPPED GLOBAL PROPERTY VTK_TCL_WRAPPED)
# Format the list in a way Tcl expects it
unset(VTK_TCL_WRAPPED_MODULES)
foreach(module ${VTK_TCL_WRAPPED})
  if("${${module}_TCL_NAME}" STREQUAL "")
    set(${module}_TCL_NAME ${module})
  endif()
  string(REGEX REPLACE "^vtk" "" tcl_module ${${module}_TCL_NAME})
  set(VTK_TCL_WRAPPED_MODULES "${VTK_TCL_WRAPPED_MODULES} ${tcl_module}")
endforeach()

IF (NOT TCL_LIBRARY)
  MESSAGE("VTK_WRAP_TCL is ON, but the Tcl library was not found.  Please set TCL_LIBRARY." "Error")
ENDIF (NOT TCL_LIBRARY)

include(${VTK_SOURCE_DIR}/Wrapping/Tcl/vtkDetermineTkResources.cmake)

IF (WIN32)
  IF (NOT BORLAND)
    IF(NOT CYGWIN)
      CONFIGURE_FILE(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/vtk.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/resources/vtk.rc)
      INCLUDE_DIRECTORIES(
        "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        "${CMAKE_CURRENT_BINARY_DIR}/resources"
        )
      SET(VTK_EXE_RESOURCE_FILES
        "${CMAKE_CURRENT_BINARY_DIR}/resources/vtk.rc")
    ENDIF(NOT CYGWIN)
  ENDIF (NOT BORLAND)
ENDIF (WIN32)

find_package(TCL)

# export the module information
set(${vtk-module}_NO_HeaderTest 1)
set(${vtk-module}_SYSTEM_INCLUDE_DIRS ${TCL_INCLUDE_PATH})
vtk_module_export_info()

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${VTK_BINARY_DIR}/Common/Core
  ${VTK_SOURCE_DIR}/Common/Core
  ${VTK_BINARY_DIR}/Utilities/KWSys
  ${TCL_INCLUDE_PATH}
  )

if(VTK_USE_TK)
  include_directories(
    ${TK_INCLUDE_PATH}
    ${TK_XLIB_PATH}
  )
endif(VTK_USE_TK)

# Create the vtk Tcl wrapper executable.
VTK_ADD_EXECUTABLE(vtk vtkTkAppInit.cxx ${VTK_EXE_RESOURCE_FILES})

IF(VTK_USE_CARBON)
  FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
  IF(VTK_APPLE_RESOURCE)
    ADD_CUSTOM_COMMAND(
      SOURCE vtk
      COMMAND ${VTK_APPLE_RESOURCE}
      ARGS Carbon.r -o ${VTK_EXECUTABLE_DIR}/vtk
      TARGET vtk
      )
  ENDIF(VTK_APPLE_RESOURCE)
ENDIF(VTK_USE_CARBON)

# Link to TCL static libraries
if(BUILD_SHARED_LIBS)
  # vtkCommonCoreTCL is required even for shared builds
  # to link the vtkTclUtil object - FIXME ??
  target_link_libraries(vtk vtkCommonCoreTCL)
else()
  foreach(module ${VTK_TCL_WRAPPED})
    target_link_libraries(vtk ${${module}_TCL_NAME}TCL)
  endforeach()
endif()

if(TK_FOUND AND VTK_USE_TK)
  target_link_libraries(vtk ${VTK_TK_LIBRARIES})
endif()

if(NOT VTK_INSTALL_NO_RUNTIME AND VTK_BUILD_FORWARDING_EXECUTABLES)
  install(TARGETS vtk
    EXPORT ${VTK_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeExecutables # .exe, .dll
    LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries # .so, mod.dll
    ARCHIVE DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT Development      # .a, .lib
    )
  install(TARGETS vtk${VTK_EXE_SUFFIX}
    EXPORT ${VTK_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeExecutables # .exe, .dll
    LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries # .so, mod.dll
    ARCHIVE DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT Development      # .a, .lib
    )
elseif(NOT VTK_INSTALL_NO_RUNTIME)
  install(TARGETS vtk
    EXPORT ${VTK_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeExecutables # .exe, .dll
    LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries # .so, mod.dll
    ARCHIVE DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT Development      # .a, .lib
    )
endif()

# Create the pvtk Tcl wrapper executable with MPI support.
if(vtkParallelMPI_ENABLED)
  vtk_module_config(PVTK vtkParallelMPI)
  include_directories(${PVTK_INCLUDE_DIRS})

  VTK_ADD_EXECUTABLE(pvtk vtkParaTkAppInit.cxx ${VTK_EXE_RESOURCE_FILES})
  IF(VTK_USE_CARBON)
    FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
    IF(VTK_APPLE_RESOURCE)
      ADD_CUSTOM_COMMAND(
        SOURCE pvtk
        COMMAND ${VTK_APPLE_RESOURCE}
        ARGS Carbon.r -o ${VTK_EXECUTABLE_DIR}/pvtk
        TARGET pvtk
        )
    ENDIF(VTK_APPLE_RESOURCE)
  ENDIF(VTK_USE_CARBON)

  target_link_libraries(pvtk ${PVTK_LIBRARIES})
  if(VTK_USE_TK)
    target_link_libraries(pvtk ${VTK_TK_LIBRARIES})
  endif(VTK_USE_TK)

  # Link to TCL static libraries
  if(BUILD_SHARED_LIBS)
    # vtkCommonCoreTCL is required even for shared builds
    # to link the vtkTclUtil object - FIXME ??
    target_link_libraries(pvtk vtkCommonCoreTCL)
  else()
    foreach(module ${VTK_TCL_WRAPPED})
      target_link_libraries(pvtk ${${module}_TCL_NAME}TCL)
    endforeach()
  endif()
endif()

if(NOT VTK_INSTALL_NO_DOCUMENTATION)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README"
    DESTINATION ${VTK_INSTALL_TCL_DIR}
    COMPONENT RuntimeLibraries)
endif()
if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtktcl.c"
    DESTINATION ${VTK_INSTALL_TCL_DIR}
    COMPONENT Development)
  install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/vtkTcl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vtkTk.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vtkTclUtil.h"
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
    COMPONENT Development)
endif()

# Configure the Tcl package index files for the build tree.
set(VTK_TCL_SCRIPT_DIR "${VTK_BINARY_DIR}/Wrapping/Tcl")

set(VTK_TCL_LIBNAME_PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")
set(VTK_TCL_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})

if(VTK_TCL_CONFIGURATION_TYPES)
  foreach(config ${VTK_TCL_CONFIGURATION_TYPES})
    set(VTK_TCL_LIBRARY_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${config}")
    configure_file(${VTK_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
                   ${VTK_BINARY_DIR}/Wrapping/Tcl/${config}/pkgIndex.tcl
                   @ONLY IMMEDIATE)
  endforeach()
else()
  set(VTK_TCL_LIBRARY_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
  configure_file(${VTK_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
                 ${VTK_BINARY_DIR}/Wrapping/Tcl/pkgIndex.tcl
                 @ONLY IMMEDIATE)
endif()

# A few hand coded bits of Tcl to glue things together.
macro(configure_tcl_files)
  foreach(file ${ARGN})
    configure_file(${file}.tcl.in "${VTK_TCL_HOME}/${file}.tcl" @ONLY IMMEDIATE)
  endforeach()
endmacro()

macro(copy_tcl_files)
  foreach(file ${ARGN})
    configure_file(${file}.tcl "${VTK_TCL_HOME}/${file}.tcl" COPYONLY)
  endforeach()
endmacro()

configure_tcl_files(
  vtkbase/vtkbase
  vtk/vtk
  vtktesting/vtktesting
  vtktesting/grab
  vtkinteraction/vtkinteraction
  )
copy_tcl_files(
  vtktesting/backdrop
  vtktesting/colors
  vtktesting/mccases
  vtkinteraction/bindings
  vtkinteraction/bindings-iw
  vtkinteraction/bindings-rw
  vtkinteraction/Interactor
  vtkinteraction/setget
  )

if(NOT VTK_INSTALL_NO_RUNTIME)
  install(DIRECTORY ${VTK_TCL_HOME}/vtk ${VTK_TCL_HOME}/vtkbase
    ${VTK_TCL_HOME}/vtktesting ${VTK_TCL_HOME}/vtkinteraction
    DESTINATION ${VTK_INSTALL_TCL_DIR}
    COMPONENT RuntimeLibraries)
endif()

# We must write out a file for each wrapped module to load the SO.
foreach(mod ${VTK_TCL_WRAPPED})
  set(module "${${mod}_TCL_NAME}")
  string(TOLOWER ${module} module_lc)
  configure_file(vtkmodule.tcl.in
    "${VTK_TCL_HOME}/${module_lc}/${module_lc}.tcl" @ONLY IMMEDIATE)
  if(NOT VTK_INSTALL_NO_RUNTIME)
    install(FILES
      "${VTK_TCL_HOME}/${module_lc}/${module_lc}.tcl"
      DESTINATION ${VTK_INSTALL_TCL_DIR}/${module_lc}
      COMPONENT RuntimeLibraries)
  endif()
endforeach()

# Configure the Tcl package index file for the install tree.
SET(VTK_TCL_SCRIPT_DIR "[file dirname [info script]]")
IF(UNIX)
  SET(VTK_TCL_LIBRARY_DIR "[file dirname [file dirname [file dirname [info script]]]]")
ELSE(UNIX)
  SET(VTK_TCL_LIBRARY_DIR
      "[file join [file dirname [file dirname [file dirname [file dirname [info script]]]]] bin]")
ENDIF(UNIX)
configure_file(${VTK_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
               ${VTK_BINARY_DIR}/Wrapping/Tcl/Install/Hide/pkgIndex.tcl
               @ONLY IMMEDIATE)
IF(NOT VTK_INSTALL_NO_RUNTIME)
  INSTALL(FILES
    ${VTK_BINARY_DIR}/Wrapping/Tcl/Install/Hide/pkgIndex.tcl
    DESTINATION ${VTK_INSTALL_TCL_DIR}
    COMPONENT RuntimeLibraries)
ENDIF()

# If the wrapped tcl libs are static we have to initialized them.
if(NOT BUILD_SHARED_LIBS)
  file(REMOVE ${VTK_BINARY_DIR}/Wrapping/Tcl/vtktcl_static_prototypes.h ${VTK_BINARY_DIR}/Wrapping/Tcl/vtktcl_static_packages.h)
  foreach(module ${VTK_TCL_WRAPPED})
    string(TOLOWER ${${module}_TCL_NAME} module_lc)
    string(REGEX REPLACE "^vtk" "Vtk" module_lc ${module_lc})
    file(APPEND ${VTK_BINARY_DIR}/Wrapping/Tcl/vtktcl_static_prototypes.h "extern \"C\" int ${module_lc}tcl_Init(Tcl_Interp *interp);\n")
    file(APPEND ${VTK_BINARY_DIR}/Wrapping/Tcl/vtktcl_static_packages.h "Tcl_StaticPackage(0, \"${${module}_TCL_NAME}Tcl\", ${module_lc}tcl_Init, NULL);\n")
  endforeach()
endif()

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)

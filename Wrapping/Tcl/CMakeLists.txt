# this file is used directly from VTK/CMakeLists.txt, not from VTK/Wrapping/CMakeLists.txt

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
INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)

CONFIGURE_FILE(
  ${VTK_SOURCE_DIR}/Wrapping/Tcl/vtkTkAppInitConfigure.h.in
  ${VTK_BINARY_DIR}/Wrapping/Tcl/vtkTkAppInitConfigure.h
  @ONLY IMMEDIATE
)

get_property(VTK_TCL_WRAPPED GLOBAL PROPERTY VTK_TCL_WRAPPED)
# Format the list in a way Tcl expects it
unset(VTK_TCL_WRAPPED_MODULES)
foreach(module ${VTK_TCL_WRAPPED})
  string(REGEX REPLACE "^vtk" "" tcl_module ${module})
  if("${tcl_module}" STREQUAL "RenderingContext2D")
    set(tcl_module "RenderingContextIID")
  elseif("${tcl_module}" STREQUAL "ViewsContext2D")
    set(tcl_module "ViewsContextIID")
  endif()
  set(VTK_TCL_WRAPPED_MODULES "${VTK_TCL_WRAPPED_MODULES} ${tcl_module}")
endforeach()

IF (NOT TCL_LIBRARY)
  MESSAGE("VTK_WRAP_TCL is ON, but the Tcl library was not found.  Please set TCL_LIBRARY." "Error")
ENDIF (NOT TCL_LIBRARY)

IF (WIN32)
  IF (NOT BORLAND)
    IF(NOT CYGWIN)
      IF(VTK_USE_TK)
        INCLUDE_DIRECTORIES("${VTK_TK_RESOURCES_DIR}")
      ENDIF(VTK_USE_TK)

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

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${TCL_INCLUDE_PATH}
  )

# Create the vtk Tcl wrapper executable.
VTK_ADD_EXECUTABLE(vtk vtkTkAppInit.cxx ${VTK_EXE_RESOURCE_FILES})
target_link_libraries(vtk vtkCommonCoreTCL)

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

# Link to extra TCL libraries
foreach(lib ${_tcl_modules})
  target_link_libraries(vtk ${lib})
endforeach(lib)

if(VTK_USE_TK)
  target_link_libraries(vtk ${VTK_TK_LIBRARIES})
endif(VTK_USE_TK)

if(VTK_INSTALL_RUNTIME AND VTK_BUILD_FORWARDING_EXECUTABLES)
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
elseif(VTK_INSTALL_RUNTIME)
  install(TARGETS vtk
    EXPORT ${VTK_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeExecutables # .exe, .dll
    LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries # .so, mod.dll
    ARCHIVE DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT Development      # .a, .lib
    )
endif()

# Create the pvtk Tcl wrapper executable with MPI support.
IF (VTK_USE_PARALLEL)
  IF (VTK_USE_MPI)
    INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
    IF (MPI_LIBRARY) 
      SET(MPI_LIBRARIES ${MPI_LIBRARY})   
    ENDIF (MPI_LIBRARY)   
    IF (MPI_EXTRA_LIBRARY)   
      SET(MPI_LIBRARIES ${MPI_LIBRARIES} "${MPI_EXTRA_LIBRARY}")   
    ENDIF (MPI_EXTRA_LIBRARY) 
    
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

    TARGET_LINK_LIBRARIES (pvtk
      ${MPI_LIBRARIES}
    )

    IF(VTK_INSTALL_RUNTIME)
      INSTALL(TARGETS pvtk
        EXPORT ${VTK_INSTALL_EXPORT_NAME}
        RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeExecutables # .exe, .dll
        LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries # .so, mod.dll
        ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development      # .a, .lib
        )
    ENDIF()
  ENDIF (VTK_USE_MPI)
ENDIF (VTK_USE_PARALLEL)

if(NOT VTK_INSTALL_NO_DOCUMENTATION)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README"
    DESTINATION ${VTK_INSTALL_TCL_DIR}
    COMPONENT RuntimeLibraries)
endif()
if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtktcl.c"
    DESTINATION ${VTK_INSTALL_TCL_DIR}
    COMPONENT Development)
endif()

# Configure the Tcl package index files for the build tree.
set(VTK_TCL_SCRIPT_DIR "${VTK_BINARY_DIR}/Wrapping/Tcl")

set(VTK_TCL_LIBNAME_PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")
set(VTK_TCL_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})

if(VTK_TCL_CONFIGURATION_TYPES)
  foreach(config ${VTK_TCL_CONFIGURATION_TYPES})
    set(VTK_TCL_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}/${config}")
    configure_file(${VTK_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
                   ${VTK_BINARY_DIR}/Wrapping/Tcl/${config}/pkgIndex.tcl
                   @ONLY IMMEDIATE)
  endforeach()
else()
  set(VTK_TCL_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}")
  configure_file(${VTK_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
                 ${VTK_BINARY_DIR}/Wrapping/Tcl/pkgIndex.tcl
                 @ONLY IMMEDIATE)
endif()

# A few hand coded bits of Tcl to glue things together.
configure_file(vtkbase/vtkbase.tcl.in
  "${VTK_TCL_HOME}/vtkbase/vtkbase.tcl" @ONLY IMMEDIATE)
configure_file(vtk/vtk.tcl.in
  "${VTK_TCL_HOME}/vtk/vtk.tcl" @ONLY IMMEDIATE)
if(NOT VTK_INSTALL_NO_RUNTIME)
  install(FILES ${VTK_TCL_HOME}/vtk/vtk.tcl
    DESTINATION ${VTK_INSTALL_TCL_DIR}/vtk
    COMPONENT RuntimeLibraries)
  install(FILES ${VTK_TCL_HOME}/vtkbase/vtkbase.tcl
    DESTINATION ${VTK_INSTALL_TCL_DIR}/vtkbase
    COMPONENT RuntimeLibraries)
endif()

# We must write out a file for each wrapped module to load the SO.
foreach(module ${VTK_TCL_WRAPPED})
  string(TOLOWER ${module} module_lc)
  if(module_lc STREQUAL "vtkrenderingcontext2d")
    set(module_lc "vtkrenderingcontextiid")
    set(module "vtkRenderingContextIID")
  elseif(module_lc STREQUAL "vtkviewscontext2d")
    set(module_lc "vtkviewscontextiid")
    set(module "vtkViewsContextIID")
  endif()
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

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)

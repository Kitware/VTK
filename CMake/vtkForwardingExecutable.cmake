
#----------------------------------------------------------------------------
# Function for adding an executable with support for shared forwarding.
# Typically, one just uses ADD_EXECUTABLE to add an executable target. However
# on linuxes when rpath is off, and shared libararies are on, to over come the
# need for setting the LD_LIBRARY_PATH, we use shared-forwarding. This macro
# makes it easier to employ shared forwarding if needed. 
# ARGUMENTS:
# out_real_exe_suffix -- (out) suffix to be added to the exe-target to locate the
#                     real executable target when shared forwarding is employed.
#                     This is empty when shared forwarding is not needed.
# exe_name        -- (in)  exe target name i.e. the first argument to
#                    ADD_EXECUTABLE.
# Any remaining arguments are simply passed on to the ADD_EXECUTABLE call.
# While writing install rules for this executable. One typically does the
# following.
#   INSTALL(TARGETS exe_name
#           DESTINATION "bin"
#           COMPONENT Runtime)
#   IF (vtk_exe_suffix)
#     # Shared forwarding enabled.
#     INSTALL(TARGETS exe_name${out_real_exe_suffix}
#             DESTINATION "lib"
#             COMPONENT Runtime)
#   ENDIF (vtk_exe_suffix)
#----------------------------------------------------------------------------
FUNCTION (vtk_add_executable_with_forwarding
            out_real_exe_suffix
            exe_name
            )
  if (NOT DEFINED VTK_INSTALL_LIB_DIR_CM24)
    MESSAGE(FATAL_ERROR
      "VTK_INSTALL_LIB_DIR_CM24 variable must be set before calling add_executable_with_forwarding"
    )
  endif (NOT DEFINED VTK_INSTALL_LIB_DIR_CM24)

  vtk_add_executable_with_forwarding2(out_var "" "" 
    ${VTK_INSTALL_LIB_DIR_CM24}
    ${exe_name} ${ARGN})
  set (${out_real_exe_suffix} "${out_var}" PARENT_SCOPE)
ENDFUNCTION(vtk_add_executable_with_forwarding)

#----------------------------------------------------------------------------
FUNCTION (vtk_add_executable_with_forwarding2
            out_real_exe_suffix
            extra_build_dirs
            extra_install_dirs
            install_lib_dir
            exe_name
            )

  SET(mac_bundle)
  IF (APPLE)
    set (largs ${ARGN})
    LIST (FIND largs "MACOSX_BUNDLE" mac_bundle_index)
    IF (mac_bundle_index GREATER -1)
      SET (mac_bundle TRUE)
    ENDIF (mac_bundle_index GREATER -1)
  ENDIF (APPLE)

  SET(VTK_EXE_SUFFIX)
  IF (BUILD_SHARED_LIBS AND NOT mac_bundle)
    IF(NOT WIN32)
      SET(exe_output_path ${EXECUTABLE_OUTPUT_PATH})
      IF (NOT EXECUTABLE_OUTPUT_PATH)
        SET (exe_output_path ${CMAKE_BINARY_DIR})
      ENDIF (NOT EXECUTABLE_OUTPUT_PATH)
      SET(VTK_EXE_SUFFIX -launcher)
      SET(VTK_FORWARD_DIR_BUILD "${exe_output_path}")
      SET(VTK_FORWARD_DIR_INSTALL "../${install_lib_dir}")
      SET(VTK_FORWARD_PATH_BUILD "\"${VTK_FORWARD_DIR_BUILD}\"")
      SET(VTK_FORWARD_PATH_INSTALL "\"${VTK_FORWARD_DIR_INSTALL}\"")
      FOREACH(dir ${extra_build_dirs})
        SET (VTK_FORWARD_PATH_BUILD "${VTK_FORWARD_PATH_BUILD},\"${dir}\"")
      ENDFOREACH(dir)
      FOREACH(dir ${extra_install_dirs})
        SET (VTK_FORWARD_PATH_INSTALL "${VTK_FORWARD_PATH_INSTALL},\"${dir}\"")
      ENDFOREACH(dir)

      SET(VTK_FORWARD_EXE ${exe_name})
      CONFIGURE_FILE(
        ${VTK_CMAKE_DIR}/vtk-forward.c.in
        ${CMAKE_CURRENT_BINARY_DIR}/${exe_name}-forward.c
        @ONLY IMMEDIATE)
      add_executable(${exe_name}${VTK_EXE_SUFFIX}
        ${CMAKE_CURRENT_BINARY_DIR}/${exe_name}-forward.c)
      set_target_properties(${exe_name}${VTK_EXE_SUFFIX} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/launcher)
      set_target_properties(${exe_name}${VTK_EXE_SUFFIX} PROPERTIES
        OUTPUT_NAME ${exe_name})
      ADD_DEPENDENCIES(${exe_name}${VTK_EXE_SUFFIX} ${exe_name})
    ENDIF(NOT WIN32)
  ENDIF (BUILD_SHARED_LIBS AND NOT mac_bundle)

  add_executable(${exe_name} ${ARGN})

  set (${out_real_exe_suffix} "${VTK_EXE_SUFFIX}" PARENT_SCOPE)
ENDFUNCTION (vtk_add_executable_with_forwarding2)

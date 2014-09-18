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
#   install(TARGETS exe_name
#           DESTINATION "bin"
#           COMPONENT Runtime)
#   if(vtk_exe_suffix)
#     # Shared forwarding enabled.
#     install(TARGETS exe_name${out_real_exe_suffix}
#             DESTINATION "lib"
#             COMPONENT Runtime)
#   endif()
#----------------------------------------------------------------------------
function(vtk_add_executable_with_forwarding
         out_real_exe_suffix
         exe_name)
  if(NOT DEFINED VTK_INSTALL_LIBRARY_DIR)
    message(FATAL_ERROR
      "VTK_INSTALL_LIBRARY_DIR variable must be set before calling add_executable_with_forwarding")
  endif()

  vtk_add_executable_with_forwarding2(out_var "" ""
    ${VTK_INSTALL_LIBRARY_DIR}
    ${exe_name} ${ARGN})
  set(${out_real_exe_suffix} "${out_var}" PARENT_SCOPE)
endfunction()

#----------------------------------------------------------------------------
function(vtk_add_executable_with_forwarding2
         out_real_exe_suffix
         extra_build_dirs
         extra_install_dirs
         install_lib_dir
         exe_name)

  set(mac_bundle)
  if(APPLE)
    set(largs ${ARGN})
    list(FIND largs "MACOSX_BUNDLE" mac_bundle_index)
    if(mac_bundle_index GREATER -1)
      set(mac_bundle TRUE)
    endif()
  endif()

  set(VTK_EXE_SUFFIX)
  if(BUILD_SHARED_LIBS AND NOT mac_bundle)
    if(NOT WIN32)
      set(exe_output_path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
      if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(exe_output_path ${CMAKE_BINARY_DIR})
      endif()
      set(VTK_EXE_SUFFIX -launcher)
      set(VTK_FORWARD_DIR_BUILD "${exe_output_path}")
      set(VTK_FORWARD_DIR_INSTALL "../${install_lib_dir}")
      set(VTK_FORWARD_PATH_BUILD "\"${VTK_FORWARD_DIR_BUILD}\"")
      set(VTK_FORWARD_PATH_INSTALL "\"${VTK_FORWARD_DIR_INSTALL}\"")
      foreach(dir ${extra_build_dirs})
        set(VTK_FORWARD_PATH_BUILD "${VTK_FORWARD_PATH_BUILD},\"${dir}\"")
      endforeach()
      foreach(dir ${extra_install_dirs})
        set(VTK_FORWARD_PATH_INSTALL "${VTK_FORWARD_PATH_INSTALL},\"${dir}\"")
      endforeach()

      set(VTK_FORWARD_EXE ${exe_name})
      configure_file(
        ${VTK_CMAKE_DIR}/vtk-forward.c.in
        ${CMAKE_CURRENT_BINARY_DIR}/${exe_name}-forward.c
        @ONLY)
      add_executable(${exe_name}${VTK_EXE_SUFFIX}
        ${CMAKE_CURRENT_BINARY_DIR}/${exe_name}-forward.c)
      set_target_properties(${exe_name}${VTK_EXE_SUFFIX} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/launcher)
      set_target_properties(${exe_name}${VTK_EXE_SUFFIX} PROPERTIES
        OUTPUT_NAME ${exe_name})
      add_dependencies(${exe_name}${VTK_EXE_SUFFIX} ${exe_name})
    endif()
  endif()

  add_executable(${exe_name} ${ARGN})

  set(${out_real_exe_suffix} "${VTK_EXE_SUFFIX}" PARENT_SCOPE)
endfunction()

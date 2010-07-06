# Emit a fatal error and inform the user if they have not enabled hooks.
option(VTK_IGNORE_HOOKS "Should the VTK hooks check be ignored?" OFF)
if(DEFINED ENV{DASHBOARD_TEST_FROM_CTEST})
  set(VTK_FROM_CTEST TRUE)
endif()
if(NOT VTK_IGNORE_HOOKS AND NOT VTK_FROM_CTEST AND
    EXISTS "${VTK_SOURCE_DIR}/.git/config")
  if(NOT EXISTS "${VTK_SOURCE_DIR}/.git/hooks/.git/config")
    message(FATAL_ERROR
"Please initialize your local Git hooks, paste the following into a shell:
 cd \"${VTK_SOURCE_DIR}/.git/hooks\"
 git init
 git pull .. remotes/origin/hooks
 cd ../..
See http://www.vtk.org/Wiki/VTK/Git#Hooks for more details.
If you wish to ignore this check for a build set the CMake cache variable VTK_IGNORE_HOOKS to ON. To ignore this check in all builds either archive your clone, or create the file\n ${VTK_SOURCE_DIR}/.git/hooks/.git/config\nin your source tree.")
  endif()
endif()

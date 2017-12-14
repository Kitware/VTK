# Full functional CMAKE_CROSSCOMPILING_EMULATOR support for custom_command and
# custom_target is available in CMake 3.8.0
# It was first added in CMake 3.6.0 and later fixed in CMake 3.8.0 (commit e7480d67, CMake issue #16288)
set(_vtk_crosscompiling_emulator_support_custom_target 1)
if(CMAKE_VERSION VERSION_LESS "3.8.0")
  set(_vtk_crosscompiling_emulator_support_custom_target 0)
endif()
# Maintain backward compatibility with user setting COMPILE_TOOLS_IMPORTED
if(DEFINED COMPILE_TOOLS_IMPORTED AND NOT DEFINED VTK_COMPILE_TOOLS_IMPORTED)
  set(VTK_COMPILE_TOOLS_IMPORTED ${COMPILE_TOOLS_IMPORTED})
  unset(COMPILE_TOOLS_IMPORTED)
endif()
# Variable VTK_COMPILE_TOOLS_IMPORTED is preferred
if(NOT DEFINED VTK_COMPILE_TOOLS_IMPORTED)
  set(VTK_COMPILE_TOOLS_IMPORTED FALSE)
endif()
if(CMAKE_CROSSCOMPILING
    AND NOT VTK_COMPILE_TOOLS_IMPORTED
    AND (NOT DEFINED CMAKE_CROSSCOMPILING_EMULATOR
         OR NOT _vtk_crosscompiling_emulator_support_custom_target)
  )
  # if CMAKE_CROSSCOMPILING is true and crosscompiling emulator is not available, we need
  # to import build-tools targets.
  find_package(VTKCompileTools REQUIRED)
  set(VTK_COMPILE_TOOLS_IMPORTED TRUE)
endif()

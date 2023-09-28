if (NOT (DEFINED token_cmake_dir AND
         DEFINED token_cmake_build_dir AND
         DEFINED token_cmake_destination))
  message(FATAL_ERROR
    "tokenInstallCMakePackage is missing input variables.")
endif ()

set(prefix_file "${token_cmake_build_dir}/token-prefix.cmake")
file(WRITE "${prefix_file}"
  "set(_token_import_prefix \"\${CMAKE_CURRENT_LIST_DIR}\")\n")
set(destination "${token_cmake_destination}")
while (destination)
  get_filename_component(destination "${destination}" DIRECTORY)
  file(APPEND "${prefix_file}"
    "get_filename_component(_token_import_prefix \"\${_token_import_prefix}\" DIRECTORY)\n")
endwhile ()

configure_file(
  "${token_cmake_dir}/tokenConfig.cmake.in"
  "${token_cmake_build_dir}/tokenConfig.cmake"
  @ONLY)
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${token_cmake_build_dir}/tokenConfigVersion.cmake"
  VERSION "${token_VERSION}"
  COMPATIBILITY AnyNewerVersion)

# For convenience, a package is written to the top of the build tree. At some
# point, this should probably be deprecated and warn when it is used.
file(GENERATE
  OUTPUT  "${CMAKE_BINARY_DIR}/tokenConfig.cmake"
  CONTENT "include(\"${token_cmake_build_dir}/tokenConfig.cmake\")\n")
configure_file(
  "${token_cmake_build_dir}/tokenConfigVersion.cmake"
  "${CMAKE_BINARY_DIR}/tokenConfigVersion.cmake"
  COPYONLY)

set(token_cmake_module_files
  tokenMacros.cmake
)

set(token_cmake_files_to_install
  "${prefix_file}"
)

foreach (token_cmake_module_file IN LISTS token_cmake_module_files)
  configure_file(
    "${token_cmake_dir}/${token_cmake_module_file}"
    "${token_cmake_build_dir}/${token_cmake_module_file}"
    COPYONLY)
  list(APPEND token_cmake_files_to_install
    "${token_cmake_module_file}")
endforeach ()

include(tokenInstallCMakePackageHelpers)
if (NOT token_RELOCATABLE_INSTALL)
  list(APPEND token_cmake_files_to_install
    "${token_cmake_build_dir}/token-find-package-helpers.cmake")
endif ()

foreach (token_cmake_file IN LISTS token_cmake_files_to_install)
  if (IS_ABSOLUTE "${token_cmake_file}")
    file(RELATIVE_PATH token_cmake_subdir_root "${token_cmake_build_dir}" "${token_cmake_file}")
    get_filename_component(token_cmake_subdir "${token_cmake_subdir_root}" DIRECTORY)
    set(token_cmake_original_file "${token_cmake_file}")
  else ()
    get_filename_component(token_cmake_subdir "${token_cmake_file}" DIRECTORY)
    set(token_cmake_original_file "${token_cmake_dir}/${token_cmake_file}")
  endif ()
  install(
    FILES       "${token_cmake_original_file}"
    DESTINATION "${token_cmake_destination}/${token_cmake_subdir}"
    COMPONENT   "development")
endforeach ()

install(
  FILES       "${token_cmake_build_dir}/tokenConfig.cmake"
              "${token_cmake_build_dir}/tokenConfigVersion.cmake"
  DESTINATION "${token_cmake_destination}"
  COMPONENT   "development")

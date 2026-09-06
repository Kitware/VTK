# Settings shared by every webgpu configuration.
#
# Each configuration needs a file named for it - ctest_configure.cmake loads
# `configure_$ENV{CMAKE_CONFIGURATION}.cmake` - so the per-configuration files
# stay, and hold only what is specific to their platform.

# Where download_dawn.cmake puts the implementation.
get_filename_component(dawn_dir "${CMAKE_CURRENT_LIST_DIR}/../dawn" ABSOLUTE)
set(CMAKE_PREFIX_PATH "${dawn_dir}")

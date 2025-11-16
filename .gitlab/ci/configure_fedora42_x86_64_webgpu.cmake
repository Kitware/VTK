get_filename_component(dawn_dir "${CMAKE_CURRENT_LIST_DIR}/../dawn" ABSOLUTE)
set(CMAKE_PREFIX_PATH "${dawn_dir}")
include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora42.cmake")

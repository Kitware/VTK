get_filename_component(this_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(top_dir "${this_dir}/../.." ABSOLUTE)
include("${top_dir}/CTestConfig.cmake")

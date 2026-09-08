include("${CMAKE_CURRENT_LIST_DIR}/configure_webgpu.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora44.cmake")

# The Qt widget is the only consumer of the WebGPU module's API in the tree. Build it here so a
# change to that API cannot break it unnoticed: no other configuration enables both Qt and WebGPU.
# Naming the two modules rather than the Qt group keeps the rest of the group out of this build;
# a module setting other than DEFAULT wins over the group's.
set(VTK_MODULE_ENABLE_VTK_GUISupportQt YES CACHE STRING "")
set(VTK_MODULE_ENABLE_VTK_GUISupportQtWebGPU YES CACHE STRING "")

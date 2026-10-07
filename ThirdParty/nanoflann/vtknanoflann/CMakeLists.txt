set(headers
  include/nanoflann.hpp)

# nanoflann's parallel tree build uses std::thread/std::future.
vtk_module_find_package(PACKAGE Threads)

vtk_module_install_headers(
  USE_RELATIVE_PATHS
  SUBDIR  "vtknanoflann"
  FILES   ${headers})

vtk_module_link(VTK::nanoflann
  INTERFACE
    Threads::Threads)

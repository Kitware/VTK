vtk_module_impl()
vtk_module_export("")

# Other web modules write output files into
# the www directory. Here we simply install the
# entire directory in one place.
if(NOT IS_DIRECTORY "${VTK_WWW_DIR}")
  file(MAKE_DIRECTORY "${VTK_WWW_DIR}")
endif()

install(DIRECTORY "${VTK_WWW_DIR}"
  DESTINATION ${VTK_INSTALL_LIBRARY_DIR}
  COMPONENT Runtime)

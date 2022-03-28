## Fix Installation destination for Python modules on MinGW

The installation destination for Python modules on MinGW has been changed from
`"${CMAKE_INSTALL_BINDIR}/Lib/site-packages"` to
`"${CMAKE_INSTALL_LIBDIR}/python${_vtk_python_version_suffix}/site-packages"`.

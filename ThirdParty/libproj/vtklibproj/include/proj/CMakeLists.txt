set(installdest ${INCLUDEDIR}/proj)

install(
  FILES util.hpp metadata.hpp common.hpp crs.hpp datum.hpp
    coordinatesystem.hpp coordinateoperation.hpp io.hpp nn.hpp
  #DESTINATION ${installdest}
  DESTINATION "${_vtk_build_HEADERS_DESTINATION}/vtklibproj/include/proj"
  COMPONENT "${_vtk_build_HEADERS_COMPONENT}"
)

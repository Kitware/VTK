#netcdf cxx

project(netcdf_cxx)

SET(SOURCES
  netcdf.cpp
  ncvalues.cpp
  )

foreach(file ${SOURCES})
  set(path_SOURCES ${path_SOURCES} ${PROJECT_SOURCE_DIR}/${file})
endforeach(file ${SOURCES})

set(netcdf_cxx_SOURCES ${path_SOURCES} CACHE INTERNAL "" FORCE)

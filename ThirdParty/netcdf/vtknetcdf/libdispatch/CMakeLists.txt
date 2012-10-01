#netcdf libdispatch

project(netcdf_libdispatch)

set(SOURCES
  parallel.c
  copy.c
  file.c
  dim.c
  att.c
  error.c
  var.c
  dispatch.c
  nc_url.c
  nclist.c
  ncbytes.c
  ncbytes.h
  )

if(USE_NETCDF4)
  SET(SOURCES ${SOURCES} nc4.c)
endif()

foreach(file ${SOURCES})
  set(path_SOURCES ${path_SOURCES} ${PROJECT_SOURCE_DIR}/${file})
endforeach(file ${SOURCES})

set(netcdf_dispatch_SOURCES ${path_SOURCES} CACHE INTERNAL "" FORCE)

#netcdf libsrc

project(netcdf_libsrc)

SET(SOURCES
  string.c
  v1hpg.c
  onstack.h
  rnd.h
  utf8proc.c
  utf8proc_data.h
  nclistmgr.c
  #putget.m4
  putget.c
  #attr.m4
  attr.c
  nc3dispatch.c
  nc.c
  var.c
  dim.c
  #ncx.m4
  ncx.c
  ncx.h
  lookup3.c
  pstdint.h
  )
  
if(USE_FFIO)
  SET(SOURCES ${SOURCES} ffio.c)
else()
  SET(SOURCES ${SOURCES} posixio.c)
endif()

foreach(file ${SOURCES})
  set(path_SOURCES ${path_SOURCES} ${PROJECT_SOURCE_DIR}/${file})
endforeach()

set(netcdf_src_SOURCES ${path_SOURCES} PARENT_SCOPE)

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
else(USE_FFIO)
  SET(SOURCES ${SOURCES} posixio.c)
endif(USE_FFIO)

foreach(file ${SOURCES})
  set(path_SOURCES ${path_SOURCES} ${PROJECT_SOURCE_DIR}/${file})
endforeach(file ${SOURCES})

set(netcdf_src_SOURCES ${path_SOURCES} CACHE INTERNAL "" FORCE)

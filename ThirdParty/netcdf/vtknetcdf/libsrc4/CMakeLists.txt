#netcdf libsrc4

project(netcdf_libsrc4)

set(SOURCES
  nc4internal.h
  error4.c
  nc4dispatch.c
  nc4dispatch.h
  nc4attr.c
  nc4dim.c
  nc4file.c
  nc4grp.c
  nc4type.c
  nc4var.c
  ncfunc.c
  nc4internal.c
  nc4hdf.c
  )
  
#
## HDF4 used?
#if USE_HDF4
#libnetcdf4_la_LIBADD += -lmfhdf -ldf
#endif # USE_HDF4
#
## How about pnetcdf?
#if USE_PNETCDF
#libnetcdf4_la_LIBADD += -lpnetcdf
#endif # USE_PNETCDF
#
## Always link to hdf5_hl, hdf5, and z.
#libnetcdf4_la_LIBADD += -lhdf5_hl -lhdf5 -lz
#
## This turns on declspec magic in netcdf.h for windows DLLs.
#if BUILD_DLL
#libnetcdf4_la_CPPFLAGS += -DDLL_EXPORT
#endif
#

foreach(file ${SOURCES})
  set(path_SOURCES ${path_SOURCES} ${PROJECT_SOURCE_DIR}/${file})
endforeach()

set(netcdf_src4_SOURCES ${path_SOURCES} PARENT_SCOPE)

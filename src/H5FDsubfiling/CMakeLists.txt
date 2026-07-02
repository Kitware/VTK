if (FALSE) # XXX(kitware): cmake versions are handled by VTK.
cmake_minimum_required (VERSION 3.26)
endif ()
project (HDF5_H5FD_SUBFILING C)

# Sanity checking
if (NOT H5FD_PUBLIC_HDRS)
  message (FATAL_ERROR "internal configure error - H5FD_PUBLIC_HDRS not set")
endif ()
if (NOT H5FD_SUBFILING_DIR)
  message (FATAL_ERROR "internal configure error - H5FD_SUBFILING_DIR not set")
endif ()

if (HDF5_ENABLE_SUBFILING_VFD)
  # Sanity checking
  if (NOT H5FD_SOURCES)
    message (FATAL_ERROR "internal configure error - H5FD_SOURCES not set")
  endif ()

  set (HDF5_H5FD_SUBFILING_SOURCES
    ${H5FD_SUBFILING_DIR}/H5FDioc.c
    ${H5FD_SUBFILING_DIR}/H5FDioc_int.c
    ${H5FD_SUBFILING_DIR}/H5FDioc_threads.c
    ${H5FD_SUBFILING_DIR}/H5FDsubfiling.c
    ${H5FD_SUBFILING_DIR}/H5FDsubfile_int.c
    ${H5FD_SUBFILING_DIR}/H5subfiling_common.c
  )

  # Add Subfiling VFD sources to HDF5 library's H5FD sources
  set (H5FD_SOURCES
    ${H5FD_SOURCES}
    ${HDF5_H5FD_SUBFILING_SOURCES}
    PARENT_SCOPE
  )
endif ()

# Add Subfiling VFD public headers to HDF5 library's
# public H5FD headers, even if the Subfiling VFD isn't
# enabled and built
set (HDF5_H5FD_PUBLIC_SUBFILING_HEADERS
  ${H5FD_SUBFILING_DIR}/H5FDsubfiling.h
  ${H5FD_SUBFILING_DIR}/H5FDioc.h
)

# Add Subfiling VFD public headers to HDF5 library's public H5FD headers
set (H5FD_PUBLIC_HDRS
  ${H5FD_PUBLIC_HDRS}
  ${HDF5_H5FD_PUBLIC_SUBFILING_HEADERS}
  PARENT_SCOPE
)

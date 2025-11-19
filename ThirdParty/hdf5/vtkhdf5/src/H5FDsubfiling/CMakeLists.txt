if (FALSE) # XXX(kitware): cmake versions are handled by VTK.
cmake_minimum_required (VERSION 3.18)
endif ()
project (HDF5_H5FD_SUBFILING C)

# Sanity checking
if (NOT H5FD_HDRS)
  message (FATAL_ERROR "internal configure error - H5FD_HDRS not set")
endif ()
if (NOT H5FD_SUBFILING_DIR)
  message (FATAL_ERROR "internal configure error - H5FD_SUBFILING_DIR not set")
endif ()

if (HDF5_ENABLE_SUBFILING_VFD)
  # Sanity checking
  if (NOT H5FD_SOURCES)
    message (FATAL_ERROR "internal configure error - H5FD_SOURCES not set")
  endif ()
  if (NOT H5FD_SUBFILING_MERCURY_DIR)
    message (FATAL_ERROR "internal configure error - H5FD_SUBFILING_MERCURY_DIR not set")
  endif ()

  # Add mercury util sources to Subfiling VFD sources
  set (MERCURY_UTIL_SOURCES
    ${H5FD_SUBFILING_MERCURY_DIR}/mercury_thread.c
    ${H5FD_SUBFILING_MERCURY_DIR}/mercury_thread_condition.c
    ${H5FD_SUBFILING_MERCURY_DIR}/mercury_thread_pool.c
    ${H5FD_SUBFILING_MERCURY_DIR}/mercury_thread_mutex.c
  )

  set (HDF5_H5FD_SUBFILING_SOURCES
    ${H5FD_SUBFILING_DIR}/H5FDioc.c
    ${H5FD_SUBFILING_DIR}/H5FDioc_int.c
    ${H5FD_SUBFILING_DIR}/H5FDioc_threads.c
    ${H5FD_SUBFILING_DIR}/H5FDsubfiling.c
    ${H5FD_SUBFILING_DIR}/H5FDsubfile_int.c
    ${H5FD_SUBFILING_DIR}/H5subfiling_common.c
    ${MERCURY_UTIL_SOURCES}
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
set (HDF5_H5FD_SUBFILING_HEADERS
  ${H5FD_SUBFILING_DIR}/H5FDsubfiling.h
  ${H5FD_SUBFILING_DIR}/H5FDioc.h
)

# Add Subfiling VFD public headers to HDF5 library's public H5FD headers
set (H5FD_HDRS
  ${H5FD_HDRS}
  ${HDF5_H5FD_SUBFILING_HEADERS}
  PARENT_SCOPE
)

#
# Try to find SWT jar library path.
# Once done this will define
#
# ECLIPSE_PLUGINS_DIR = directory where Eclipse plugins reside
# ECLIPSE_SWT_LIBRARIES = full path to the SWT jar libraries
#

FIND_PATH(ECLIPSE_PLUGINS_DIR plugins
  HINTS
    $ENV{ECLIPSE_HOME}
  DOC "Eclipse plugins directory"
)

if(NOT EXISTS ${ECLIPSE_SWT_LIBRARIES})
  file(GLOB SWT_FILES "${ECLIPSE_PLUGINS_DIR}/org.eclipse.swt.*" )

  set(ECLIPSE_SWT_LIBRARIES "" CACHE FILEPATH "SWT library" FORCE)

  foreach(f ${SWT_FILES})
    if(NOT ${f} MATCHES "^.*source.*")
      set(ECLIPSE_SWT_LIBRARIES ${f} CACHE FILEPATH "SWT library" FORCE)
    endif()
  endforeach()
endif()

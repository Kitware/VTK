#
# Try to find SWT jar library path.
# Once done this will define
#
# SWT_ECLIPSE_PLUGINS_DIR = directory where Eclipse plugins reside
# SWT_ECLIPSE_LIBRARIES = full path to the SWT jar libraries

set(SWT_FOUND 0)

find_path(SWT_ECLIPSE_PLUGINS_DIR
  NAMES plugins
  HINTS $ENV{ECLIPSE_HOME}
  DOC "Eclipse plugins directory")
mark_as_advanced(SWT_ECLIPSE_PLUGINS_DIR)

if (EXISTS "${SWT_ECLIPSE_PLUGINS_DIR}")
  file(GLOB _swt_files "${SWT_ECLIPSE_PLUGINS_DIR}/org.eclipse.swt.*" )

  set(SWT_ECLIPSE_LIBRARIES "")

  foreach (_swt_file IN LISTS _swt_files)
    if (NOT _swt_file MATCHES "^.*source.*")
      set(SWT_ECLIPSE_LIBRARIES "${_swt_file}")
      set(SWT_FOUND 1)
    endif ()
  endforeach ()
  unset(_swt_file)
  unset(_swt_files)
endif ()

if (NOT SWT_FOUND)
  set(SWT_NOT_FOUND_MESSAGE
    "Failed to find SWT Eclipse plugins.")
endif ()

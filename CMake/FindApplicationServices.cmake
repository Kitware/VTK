
# -----------------------------------------------------------------------------
# Find ApplicationServices umbrella framework (Mac OS X).
#
# ApplicationServices is an umbrella framework. Notably, it includes
# CoreGraphics framework.
# CoreGraphics (aka Quartz) is the framework representation of the window
# server.
#
# Define:
# ApplicationServices_FOUND
# ApplicationServices_INCLUDE_DIR
# ApplicationServices_LIBRARY

set(ApplicationServices_FOUND false)
set(ApplicationServices_INCLUDE_DIR)
set(ApplicationServices_LIBRARY)

if(APPLE) # The only platform it makes sense to check for ApplicationServices
 find_library(ApplicationServices ApplicationServices)
 if(ApplicationServices)
  set(ApplicationServices_FOUND true)
  set(ApplicationServices_INCLUDE_DIR ${ApplicationServices})
  set(ApplicationServices_LIBRARY ${ApplicationServices})
 endif(ApplicationServices)

endif(APPLE)

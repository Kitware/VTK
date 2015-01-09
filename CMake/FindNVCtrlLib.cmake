# -----------------------------------------------------------------------------
# Find NVCtrlLib SDK
# Define:
# NVCtrlLib_FOUND
# NVCtrlLib_INCLUDE_DIR
# NVCtrlLib_LIBRARY

# typically, download the source package of nvidia-settings from:
# ftp://download.nvidia.com/XFree86/nvidia-settings/
# once uncompressed, nvidia-settings-1.0/src/libXNVCtrl directory contents
# libXNVCtrl.a, NVCtrlLib.h and NVCtrl.h.

# or if it is already installed by the distribution:
# /usr/lib/libXNVCtrl.a
# /usr/include/NVCtrl/NVCtrl.h
# /usr/include/NVCtrl/NVCtrlLib.h

# License issue:
# libXNVCtrl is intended to be MIT-X11 / BSD but is marked as GPL in older
# driver versions.
# This issue has been raised in this thread:
# http://www.nvnews.net/vbulletin/showpost.php?p=1756087&postcount=8
# it has been fixed in drivers 177.82 and above

set(NVCtrlLib_FOUND false)
set(NVCtrlLib_INCLUDE_DIR)
set(NVCtrlLib_LIBRARY)

# search for libXNVCtrl.a
# typically in nvidia-settings-1.0/src/libXNVCtrl/libXNVCtrl.a
find_library(NVCtrlLib_LIBRARY XNVCtrl /usr/lib
             DOC "Path to NVCONTROL static library (libXNVCtrl.a)")
find_path(NVCtrlLib_INCLUDE_DIR NVCtrlLib.h /usr/include/NVCtrl
          DOC "Path to NVCONTROL headers (NVCtrlLib.h and NVCtrl.h)")

if(NVCtrlLib_LIBRARY AND NVCtrlLib_INCLUDE_DIR)
 set(NVCtrlLib_FOUND true)
endif()

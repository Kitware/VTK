# Try to find VLI libraries for VolumePro.
# Once done this will define
#
#  VLI_LIBRARY_FOR_VP1000
#  VLI_INCLUDE_PATH_FOR_VP1000
#
#  The VolumePro VG500 (using the following variables) is no longer supported.
#  VLI_LIBRARY_FOR_VG500
#  VLI_INCLUDE_PATH_FOR_VG500

#FIND_LIBRARY(VLI_LIBRARY_FOR_VG500
#  NAMES vli
#  PATHS  
#    /usr/lib 
#    /usr/local/lib
#    /opt/vli/lib
#    "C:/Program Files/VolumePro/lib"
#  )

FIND_LIBRARY(VLI_LIBRARY_FOR_VP1000
  NAMES vli3
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/vli/lib
    "C:/Program Files/VolumePro1000/lib"
  )

#IF (VLI_LIBRARY_FOR_VG500)
#  IF (WIN32)
#    FIND_PATH(VLI_INCLUDE_PATH_FOR_VG500 VolumePro/inc/vli.h 
#      "C:/Program Files"
#      )
#  ELSE (WIN32)
#    FIND_PATH(VLI_INCLUDE_PATH_FOR_VG500 vli/include/vli.h
#      /usr
#      /usr/local
#      /opt
#      )
#  ENDIF (WIN32)
#ENDIF (VLI_LIBRARY_FOR_VG500)

IF (VLI_LIBRARY_FOR_VP1000)
  IF (WIN32)
    FIND_PATH(VLI_INCLUDE_PATH_FOR_VP1000 VolumePro1000/inc/vli.h 
      "C:/Program Files"
      )
  ELSE (WIN32)
    FIND_PATH(VLI_INCLUDE_PATH_FOR_VP1000 vli3/include/vli.h
      /usr
      /usr/local
      /opt/vli/include
      )
  ENDIF (WIN32)
ENDIF (VLI_LIBRARY_FOR_VP1000)

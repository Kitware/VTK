# memmgr back ends: compile only one of these into a working library
# (For now, let's use the mode that requires the image fit into memory.
# This is the recommended mode for Win32 anyway.)
SOURCE_FILES(systemdependent_SRCS jmemnobs.c)

# library object files common to compression and decompression
SOURCE_FILES(common_SRCS 
jcomapi.c jutils.c jerror.c jmemmgr.c
)

# compression library object files
SOURCE_FILES(compression_SRCS
jcapimin.c jcapistd.c jctrans.c jcparam.c jdatadst.c jcinit.c 
jcmaster.c jcmarker.c jcmainct.c jcprepct.c jccoefct.c jccolor.c
jcsample.c jchuff.c jcphuff.c jcdctmgr.c jfdctfst.c jfdctflt.c 
jfdctint.c
)

# decompression library object files
SOURCE_FILES(decompression_SRCS
jdapimin.c jdapistd.c jdtrans.c jdatasrc.c jdmaster.c 
jdinput.c jdmarker.c jdhuff.c jdphuff.c jdmainct.c jdcoefct.c 
jdpostct.c jddctmgr.c jidctfst.c jidctflt.c jidctint.c jidctred.c 
jdsample.c jdcolor.c jquant1.c jquant2.c jdmerge.c
)


CONFIGURE_FILE(
${VTK_SOURCE_DIR}/Utilities/jpeg/.NoDartCoverage 
${VTK_BINARY_DIR}/Utilities/jpeg/.NoDartCoverage)

IF (WIN32)
   IF (BUILD_SHARED_LIBS)
      ADD_DEFINITIONS( -DJPEGDLL -DUSE_WINDOWS_MESSAGEBOX )
   ELSE (BUILD_SHARED_LIBS)
      ADD_DEFINITIONS( -DJPEGSTATIC -DUSE_WINDOWS_MESSAGEBOX )
   ENDIF (BUILD_SHARED_LIBS)
ENDIF (WIN32)

ADD_LIBRARY(vtkjpeg systemdependent_SRCS common_SRCS compression_SRCS decompression_SRCS)


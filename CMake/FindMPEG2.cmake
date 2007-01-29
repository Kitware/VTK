#
# Find the MPEG2 includes and library
#
# This module (forces the user to) provide:
# vtkMPEG2Encode_INCLUDE_PATH, for use with INCLUDE_DIRECTORIES commands
# vtkMPEG2Encode_LIBRARIES, for use with TARGET_LINK_LIBRARIES commands
#
# If VTK_USE_MPEG2_ENCODER is ON, it is an error to end up with empty values
# for vtkMPEG2Encode_INCLUDE_PATH or vtkMPEG2Encode_LIBRARIES.
# The only way to fix this error, if it occurs, is to provide non-empty
# values or to turn OFF VTK_USE_MPEG2_ENCODER.
#
# Cache NOTFOUND initial values. The user will have to provide them unless
# they were provided in the initial cache.


# vtkMPEG2Encode_INCLUDE_PATH should be "/path/to/source;/path/to/binary"
# where source contains "mpeg2enc.h" and binary contains "mpeg2encDllConfig.h"
#
SET(vtkMPEG2Encode_INCLUDE_PATH "vtkMPEG2Encode_INCLUDE_PATH-NOTFOUND" CACHE STRING "Semi-colon delimited list of paths to vtkmpeg2encode header files")


# vtkMPEG2Encode_LIBRARIES should be "/path/to/binary/vtkMPEG2Encode.lib"
#
SET(vtkMPEG2Encode_LIBRARIES "vtkMPEG2Encode_LIBRARIES-NOTFOUND" CACHE STRING "Semi-colon delimited list of vtkMPEG2Encode library full path names")


# Error if values are empty:
#
IF(VTK_USE_MPEG2_ENCODER)
  IF(NOT vtkMPEG2Encode_INCLUDE_PATH)
    MESSAGE(SEND_ERROR "Could not determine value for vtkMPEG2Encode_INCLUDE_PATH. Provide value or turn off VTK_USE_MPEG2_ENCODER. Value should be a semi-colon delimited list of paths to vtkmpeg2encode header files.")
  ENDIF(NOT vtkMPEG2Encode_INCLUDE_PATH)

  IF(NOT vtkMPEG2Encode_LIBRARIES)
    MESSAGE(SEND_ERROR "Could not determine value for vtkMPEG2Encode_LIBRARIES. Provide value or turn off VTK_USE_MPEG2_ENCODER. Value should be a semi-colon delimited list of vtkMPEG2Encode library full path names.")
  ENDIF(NOT vtkMPEG2Encode_LIBRARIES)
ENDIF(VTK_USE_MPEG2_ENCODER)

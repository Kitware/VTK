# Check for vfw32 support
IF(NOT DEFINED VTK_USE_MICROSOFT_MEDIA_FOUNDATION)
  MESSAGE(STATUS "Checking if Microsoft Media Foundation is available")
  TRY_COMPILE(VTK_USE_MICROSOFT_MEDIA_FOUNDATION_DEFAULT
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp
    ${CMAKE_CURRENT_LIST_DIR}/vtkTestMicrosoftMediaFoundation.cxx
    CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=mfreadwrite;mfplat;mfuuid"
    OUTPUT_VARIABLE OUTPUT)
  IF(VTK_USE_MICROSOFT_MEDIA_FOUNDATION_DEFAULT)
    MESSAGE(STATUS "Checking if  Microsoft Media Foundation is available -- yes")
    OPTION(VTK_USE_MICROSOFT_MEDIA_FOUNDATION "Enable using Microsoft Media Foundation (mfreadwrite) for video input and output." ON)
    FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
      "Checking if  Microsoft Media Foundation is available "
      "passed with the following output:\n"
      "${OUTPUT}\n")
  ELSE()
    MESSAGE(STATUS "Checking if  Microsoft Media Foundation is available -- no")
    OPTION(VTK_USE_MICROSOFT_MEDIA_FOUNDATION "Enable using Microsoft Media Foundation (mfreadwrite) for video input and output." OFF)
    FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
      "Checking if  Microsoft Media Foundation is available "
      "failed with the following output:\n"
      "${OUTPUT}\n")
  ENDIF()
  MARK_AS_ADVANCED(VTK_USE_MICROSOFT_MEDIA_FOUNDATION)
ENDIF()

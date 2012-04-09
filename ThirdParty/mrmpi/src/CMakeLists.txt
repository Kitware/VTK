SET (mrmpi_SOURCES
        mapreduce.cpp
        keyvalue.cpp
        keymultivalue.cpp
        irregular.cpp
        hash.cpp
        memory.cpp
        error.cpp)

VTK_ADD_LIBRARY (MapReduceMPI ${mrmpi_SOURCES})
TARGET_LINK_LIBRARIES (MapReduceMPI ${mrmpi_LIBS})

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cmapreduce.h
    ${CMAKE_CURRENT_SOURCE_DIR}/error.h
    ${CMAKE_CURRENT_SOURCE_DIR}/hash.h
    ${CMAKE_CURRENT_SOURCE_DIR}/irregular.h
    ${CMAKE_CURRENT_SOURCE_DIR}/keymultivalue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/keyvalue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mapreduce.h
    ${CMAKE_CURRENT_SOURCE_DIR}/memory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mrmpi_config.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/mrmpi
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

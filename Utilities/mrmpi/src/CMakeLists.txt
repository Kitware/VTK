SET (mrmpi_SOURCES
        mapreduce.cpp
        keyvalue.cpp
        keymultivalue.cpp
        irregular.cpp
        hash.cpp
        memory.cpp
        error.cpp)

ADD_LIBRARY (MapReduceMPI ${mrmpi_SOURCES})
TARGET_LINK_LIBRARIES (MapReduceMPI ${mrmpi_LIBS})
IF(VTK_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(MapReduceMPI PROPERTIES ${VTK_LIBRARY_PROPERTIES})
ENDIF(VTK_LIBRARY_PROPERTIES)

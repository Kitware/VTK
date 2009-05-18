SET (example_LIBS mrmpi)

SET (wordfreq_SOURCES wordfreq.cpp)
ADD_EXECUTABLE (wordfreq ${wordfreq_SOURCES})
TARGET_LINK_LIBRARIES (wordfreq ${example_LIBS})

SET (rmat_SOURCES rmat.cpp)
ADD_EXECUTABLE (rmat ${rmat_SOURCES})
INCLUDE_DIRECTORIES(rmat ${VTK_SOURCE_DIR}/Utilities/mrmpi/src)
TARGET_LINK_LIBRARIES (rmat ${example_LIBS})

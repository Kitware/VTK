
SET( MFC_SRCS 
 vtkMFCWindow.cpp
) 

# add stuff to use MFC in this executable
ADD_DEFINITIONS(-D_AFXDLL)
SET(CMAKE_MFC_FLAG 6)

ADD_LIBRARY(vtkMFC ${MFC_SRCS})

TARGET_LINK_LIBRARIES( vtkMFC
  vtkRendering
  vtkGraphics
  vtkImaging
  vtkIO
  vtkFiltering
  vtkCommon
)

IF ( WIN32 )
  INSTALL_FILES( ${VTK_INSTALL_INCLUDE_DIR} .h )
  INSTALL_TARGETS( ${VTK_INSTALL_LIB_DIR} RUNTIME_DIRECTORY ${VTK_INSTALL_BIN_DIR} vtkMFC )
ENDIF ( WIN32 )



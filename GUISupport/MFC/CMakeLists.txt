
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


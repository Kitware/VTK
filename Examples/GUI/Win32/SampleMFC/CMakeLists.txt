PROJECT (Win32SampleMFC)

INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)
IF (USE_VTK_FILE)
  INCLUDE(${USE_VTK_FILE})
ENDIF (USE_VTK_FILE)

LINK_LIBRARIES( 
  vtkRendering
  vtkGraphics
  vtkImaging
  vtkIO
  vtkFiltering
  vtkCommon
)

SET(SRCS 
ChildFrm.cpp
MainFrm.cpp
Sample.cpp
Sample.rc
SampleDoc.cpp
SampleView.cpp
StdAfx.cpp
vtkMFCDocument.cpp
vtkMFCRenderView.cpp
vtkMFCView.cpp
res/Sample.ico
res/Sample.rc2
res/SampleDoc.ico
)

# add stuff to use MFC in this executable
ADD_DEFINITIONS(-D_AFXDLL)
SET(CMAKE_MFC_FLAG 6)

ADD_EXECUTABLE(Win32SampleMFC WIN32 ${SRCS})

PROJECT (Win32SampleMFC)

FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_DIR)
INCLUDE(${VTK_USE_FILE})

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

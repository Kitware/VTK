SET( vtkMDI_SRCS 
 MainFrm
 ChildFrm
 StdAfx
 vtkMDI
 vtkMDI.rc
 vtkMDIDoc
 vtkMDIView
 res/vtkMDI.ico
 res/vtkMDI.rc2
 res/vtkMDIDoc.ico
 res/Toolbar.bmp
) 

LINK_LIBRARIES( 
  vtkRendering
  vtkGraphics
  vtkImaging
  vtkIO
  vtkFiltering
  vtkCommon
)


# add stuff to use MFC in this executable
ADD_DEFINITIONS(-D_AFXDLL)
SET(CMAKE_MFC_FLAG 6)

ADD_EXECUTABLE(vtkMDI WIN32 ${vtkMDI_SRCS})

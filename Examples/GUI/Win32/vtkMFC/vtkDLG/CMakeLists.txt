
SET( vtkDLG_SRCS 
 StdAfx
 vtkDLG
 vtkDLGDlg
 vtkDLG.rc
 res/vtkDLG.ico
 res/vtkDLG.rc2
) 

LINK_LIBRARIES(
  vtkMFC 
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

ADD_EXECUTABLE(vtkDLG WIN32 ${vtkDLG_SRCS})

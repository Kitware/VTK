SET( vtkSDI_SRCS 
 MainFrm
 StdAfx
 vtkSDI
 vtkSDI.rc
 vtkSDIDoc
 vtkSDIView
 res/vtkSDI.ico
 res/vtkSDI.rc2
 res/vtkSDIDoc.ico
 res/Toolbar.bmp
) 

LINK_LIBRARIES( 
  vtkMFC
  vtkRendering
  vtkGraphics
  vtkImaging
  vtkIO
  vtkFiltering
  vtkCommon
  vtkHybrid
)


# add stuff to use MFC in this executable
ADD_DEFINITIONS(-D_AFXDLL)
SET(CMAKE_MFC_FLAG 6)

ADD_EXECUTABLE(vtkSDI WIN32 ${vtkSDI_SRCS})

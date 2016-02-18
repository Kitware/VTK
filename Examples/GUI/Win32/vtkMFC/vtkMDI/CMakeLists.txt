# This VTK_DIR usage only works in the VTK build tree...
# Make it work in the "make install" tree, too.
#
INCLUDE("${VTK_DIR}/GUISupport/MFC/VTKMFCSettings.cmake")
IF(VTK_MFC_DELAYLOAD_VTK_DLLS)
  VTK_MFC_ADD_DELAYLOAD_FLAGS(CMAKE_EXE_LINKER_FLAGS
    vtkMFC.dll
    vtkRendering.dll
    vtkIO.dll
    vtkFiltering.dll
    vtkCommon.dll
    )
ENDIF()

SET( vtkMDI_SRCS
 MainFrm
 ChildFrm
 StdAfx
 vtkMDI
 vtkMDI.rc
 vtkMDIDoc
 vtkMDIView
 res/vtkMDI.rc2
)

ADD_EXECUTABLE(vtkMDI WIN32 ${vtkMDI_SRCS})

IF(VTK_MFC_EXTRA_LIBS)
  TARGET_LINK_LIBRARIES(vtkMDI ${VTK_MFC_EXTRA_LIBS})
ENDIF()

TARGET_LINK_LIBRARIES(vtkMDI ${VTK_LIBRARIES})

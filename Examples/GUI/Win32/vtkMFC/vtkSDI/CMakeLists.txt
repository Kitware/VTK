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

SET( vtkSDI_SRCS
 MainFrm
 StdAfx
 vtkSDI
 vtkSDI.rc
 vtkSDIDoc
 vtkSDIView
 res/vtkSDI.rc2
)

ADD_EXECUTABLE(vtkSDI WIN32 ${vtkSDI_SRCS})

IF(VTK_MFC_EXTRA_LIBS)
  TARGET_LINK_LIBRARIES(vtkSDI ${VTK_MFC_EXTRA_LIBS})
ENDIF()

TARGET_LINK_LIBRARIES(vtkSDI vtkMFC)

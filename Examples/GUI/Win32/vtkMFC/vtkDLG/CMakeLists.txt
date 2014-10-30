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

SET( vtkDLG_SRCS
 StdAfx
 vtkDLG
 vtkDLGDlg
 vtkDLG.rc
 res/vtkDLG.rc2
)

ADD_EXECUTABLE(vtkDLG WIN32 ${vtkDLG_SRCS})

IF(VTK_MFC_EXTRA_LIBS)
  TARGET_LINK_LIBRARIES(vtkDLG ${VTK_MFC_EXTRA_LIBS})
ENDIF()

TARGET_LINK_LIBRARIES(vtkDLG vtkMFC)

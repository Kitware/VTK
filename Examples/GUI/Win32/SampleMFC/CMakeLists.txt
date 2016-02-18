PROJECT(Win32SampleMFC)

FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF()
INCLUDE(${VTK_USE_FILE})

# This VTK_DIR usage only works in the VTK build tree...
# Make it work in the "make install" tree, too.
#
INCLUDE("${VTK_DIR}/GUISupport/MFC/VTKMFCSettings.cmake")
IF(VTK_MFC_DELAYLOAD_VTK_DLLS)
  VTK_MFC_ADD_DELAYLOAD_FLAGS(CMAKE_EXE_LINKER_FLAGS
    vtkRendering.dll
    vtkIO.dll
    vtkFiltering.dll
    vtkCommon.dll
    )
ENDIF()

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
res/Sample.rc2
)

ADD_EXECUTABLE(Win32SampleMFC WIN32 ${SRCS})

IF(VTK_MFC_EXTRA_LIBS)
  TARGET_LINK_LIBRARIES(Win32SampleMFC ${VTK_MFC_EXTRA_LIBS})
ENDIF()

TARGET_LINK_LIBRARIES(Win32SampleMFC ${VTK_LIBRARIES})

SET ( Patented_SRCS
vtkDecimate.cxx
vtkDividingCubes.cxx
vtkGridSynchronizedTemplates3D.cxx
vtkImageMarchingCubes.cxx
vtkKitwareContourFilter.cxx
vtkMarchingContourFilter.cxx
vtkMarchingCubes.cxx
vtkMarchingSquares.cxx
vtkSliceCubes.cxx
vtkSweptSurface.cxx
vtkSynchronizedTemplates2D.cxx
vtkSynchronizedTemplates3D.cxx
)

IF(CMAKE_BUILD_TOOL MATCHES devenv)
  SET_SOURCE_FILES_PROPERTIES(vtkSweptSurface COMPILE_FLAGS /Op)
ENDIF(CMAKE_BUILD_TOOL MATCHES devenv)

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtkPatentedInstantiator PatentedInstantiator_SRCS
                       ${Patented_SRCS}
                       EXPORT_MACRO VTK_PATENTED_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtkPatented ${Patented_SRCS} ${PatentedInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtkPatented PROPERTIES LINK_FLAGS "-init _oft_initPat")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Patented/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Patented/LocalUserOptions.cmake OPTIONAL)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtkPatentedTCL PatentedTCL_SRCS ${Patented_SRCS})
  ADD_LIBRARY(vtkPatentedTCL ${PatentedTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkPatentedTCL vtkPatented vtkIOTCL vtkGraphicsTCL
                         vtkImagingTCL)
  INSTALL_TARGETS(/lib/vtk vtkPatentedTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtkPatentedPython PatentedPython_SRCS ${Patented_SRCS})
  ADD_LIBRARY(vtkPatentedPython MODULE ${PatentedPython_SRCS})
  IF(NOT APPLE)
    TARGET_LINK_LIBRARIES (vtkPatentedPython
                           vtkGraphicsPython
                           vtkImagingPython)
  ENDIF(NOT APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkPatentedPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(vtkPatentedPython vtkPatented)

  INSTALL_TARGETS(/lib/vtk vtkPatentedPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtkPatentedJava PatentedJava_SRCS ${Patented_SRCS})
  ADD_LIBRARY(vtkPatentedJava SHARED ${PatentedJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkPatentedJava vtkPatented vtkImagingJava
                         vtkGraphicsJava)
  INSTALL_TARGETS(/lib/vtk vtkPatentedJava)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES ( vtkPatented
vtkImaging
vtkGraphics
vtkIO
)

INSTALL_TARGETS(/lib/vtk vtkPatented)
INSTALL_FILES(/include/vtk .h ${Patented_SRCS})

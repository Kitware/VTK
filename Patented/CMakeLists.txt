SET(KIT Patented)
SET ( Kit_SRCS
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
VTK_MAKE_INSTANTIATOR2(vtk${KIT}Instantiator KitInstantiator_SRCS
                       ${Kit_SRCS}
                       EXPORT_MACRO VTK_PATENTED_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${KitInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtk${KIT} PROPERTIES LINK_FLAGS "-init _oft_initPat")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtk${KIT}TCL KitTCL_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} vtkIOTCL vtkGraphicsTCL
                         vtkImagingTCL)
  INSTALL_TARGETS(/lib/vtk vtk${KIT}TCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtk${KIT}Python KitPython_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS})
  IF(NOT APPLE)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python
                           vtkGraphicsPython
                           vtkImagingPython)
  ENDIF(NOT APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(vtk${KIT}Python vtk${KIT})

  INSTALL_TARGETS(/lib/vtk vtk${KIT}Python)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtk${KIT}Java KitJava_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}Java SHARED ${KitJava_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}Java vtk${KIT} vtkImagingJava
                         vtkGraphicsJava)
  INSTALL_TARGETS(/lib/vtk vtk${KIT}Java)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES ( vtk${KIT}
vtkImaging
vtkGraphics
vtkIO
)

INSTALL_TARGETS(/lib/vtk vtk${KIT})
INSTALL_FILES(/include/vtk .h ${Kit_SRCS})

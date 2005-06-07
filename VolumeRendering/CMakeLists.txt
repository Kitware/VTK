SET(KIT VolumeRendering)
SET(UKIT VOLUMERENDERING)

SET(KIT_TCL_LIBS vtkRenderingTCL vtkIOTCL)
IF(APPLE)
  SET(KIT_PYTHON_LIBS vtkRenderingPythonD vtkIOPythonD)
ELSE(APPLE)
  SET(KIT_PYTHON_LIBS vtkRenderingPython vtkIOPython)
ENDIF(APPLE)
SET(KIT_JAVA_LIBS vtkRenderingJava vtkIOJava)
SET(KIT_LIBS vtkRendering vtkIO)

IF(VTK_USE_PATENTED)
  SET(KIT_TCL_LIBS ${KIT_TCL_LIBS} vtkPatentedTCL)
  IF(APPLE)
    SET(KIT_PYTHON_LIBS ${KIT_PYTHON_LIBS} vtkPatentedPythonD)
  ELSE(APPLE)
    SET(KIT_PYTHON_LIBS ${KIT_PYTHON_LIBS} vtkPatentedPython)
  ENDIF(APPLE)
  SET(KIT_JAVA_LIBS ${KIT_JAVA_LIBS} vtkPatentedJava)
  SET(KIT_LIBS ${KIT_LIBS} vtkPatented)
ENDIF(VTK_USE_PATENTED)

SET ( Kit_SRCS
vtkDirectionEncoder.cxx
vtkEncodedGradientEstimator.cxx
vtkEncodedGradientShader.cxx
vtkFiniteDifferenceGradientEstimator.cxx
vtkFixedPointRayCastImage.cxx
vtkFixedPointVolumeRayCastCompositeGOHelper.cxx
vtkFixedPointVolumeRayCastCompositeGOShadeHelper.cxx
vtkFixedPointVolumeRayCastCompositeHelper.cxx
vtkFixedPointVolumeRayCastCompositeShadeHelper.cxx
vtkFixedPointVolumeRayCastHelper.cxx
vtkFixedPointVolumeRayCastMIPHelper.cxx
vtkFixedPointVolumeRayCastMapper.cxx
vtkRayCastImageDisplayHelper.cxx
vtkRecursiveSphereDirectionEncoder.cxx
vtkSphericalDirectionEncoder.cxx
vtkVolumeMapper.cxx
vtkVolumeRayCastCompositeFunction.cxx
vtkVolumeRayCastFunction.cxx
vtkVolumeRayCastIsosurfaceFunction.cxx
vtkVolumeRayCastMIPFunction.cxx
vtkVolumeRayCastMapper.cxx
vtkVolumeRenderingFactory.cxx
vtkVolumeTextureMapper.cxx
vtkVolumeTextureMapper2D.cxx
vtkUnstructuredGridBunykRayCastFunction.cxx
vtkUnstructuredGridHomogeneousRayIntegrator.cxx
vtkUnstructuredGridLinearRayIntegrator.cxx
vtkUnstructuredGridPartialPreIntegration.cxx
vtkUnstructuredGridPreIntegration.cxx
vtkUnstructuredGridVolumeMapper.cxx
vtkUnstructuredGridVolumeRayCastFunction.cxx
vtkUnstructuredGridVolumeRayCastIterator.cxx
vtkUnstructuredGridVolumeRayIntegrator.cxx
vtkUnstructuredGridVolumeRayCastMapper.cxx
vtkUnstructuredGridVolumeZSweepMapper.cxx
)

SET( KitOpenGL_SRCS
  vtkOpenGLRayCastImageDisplayHelper.cxx
  vtkOpenGLVolumeTextureMapper2D.cxx
)

IF(WIN32)
  ADD_EXECUTABLE(EncodeString EncodeString.cxx)

  SET ( Kit_SRCS ${Kit_SRCS} vtkVolumeTextureMapper3D.cxx )

  SET( KitOpenGL_SRCS ${KitOpenGL_SRCS}
    vtkOpenGLVolumeTextureMapper3D.cxx )

  SET_SOURCE_FILES_PROPERTIES(
    vtkVolumeTextureMapper3D
    ABSTRACT )

  SET(asm_files
    vtkVolumeTextureMapper3D_FourDependentNoShadeFP
    vtkVolumeTextureMapper3D_FourDependentShadeFP
    vtkVolumeTextureMapper3D_OneComponentNoShadeFP
    vtkVolumeTextureMapper3D_OneComponentShadeFP
    vtkVolumeTextureMapper3D_TwoDependentNoShadeFP
    vtkVolumeTextureMapper3D_TwoDependentShadeFP
    )

  IF(asm_files)
    SET(OBJ_DEPS)
    FOREACH(file ${asm_files})
      SET(src ${CMAKE_CURRENT_SOURCE_DIR}/${file}.asm)
      SET(res ${CMAKE_CURRENT_BINARY_DIR}/${file}.h)
      ADD_CUSTOM_COMMAND(
        OUTPUT ${res}
        DEPENDS ${src}
        EncodeString
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/EncodeString
        ARGS ${res} ${src} ${file}
        )
      SET(OBJ_DEPS ${OBJ_DEPS} ${res})
    ENDFOREACH(file)
    SET_SOURCE_FILES_PROPERTIES(vtkOpenGLVolumeTextureMapper3D.cxx 
      PROPERTIES OBJECT_DEPENDS "${OBJ_DEPS}")
  ENDIF(asm_files)

  SET(VTK_USE_VOLUMETEXTUREMAPPER3D 1)
ENDIF(WIN32)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkVolumeRenderingToolkit.h.in
               ${CMAKE_CURRENT_BUILD_DIR}/vtkVolumeRenderingToolkit.h )
        


SET_SOURCE_FILES_PROPERTIES(
vtkDirectionEncoder
vtkEncodedGradientEstimator
vtkFixedPointVolumeRayCastHelper
vtkRayCastImageDisplayHelper
vtkVolumeMapper
vtkVolumeRayCastFunction
vtkVolumeTextureMapper
vtkUnstructuredGridVolumeMapper
vtkUnstructuredGridVolumeRayCastFunction
vtkUnstructuredGridVolumeRayCastIterator
vtkUnstructuredGridVolumeRayIntegrator
ABSTRACT
)

IF (VTK_USE_MANGLED_MESA)
  IF (MANGLED_MESA_LIBRARY)
    SET ( KitOpenGL_SRCS ${KitOpenGL_SRCS}
                 vtkMesaRayCastImageDisplayHelper.cxx
                 vtkMesaVolumeTextureMapper2D.cxx
    )
    SET (KIT_LIBS ${KIT_LIBS} ${MANGLED_MESA_LIBRARY})
    IF(MANGLED_OSMESA_LIBRARY)
      SET( KIT_LIBS ${KIT_LIBS} ${MANGLED_OSMESA_LIBRARY} )
    ENDIF(MANGLED_OSMESA_LIBRARY)
  ENDIF (MANGLED_MESA_LIBRARY)
ELSE (VTK_USE_MANGLED_MESA)
  IF(VTK_OPENGL_HAS_OSMESA)
    IF (OSMESA_LIBRARY)
      SET(KIT_LIBS ${KIT_LIBS} ${OSMESA_LIBRARY})
    ENDIF (OSMESA_LIBRARY)
  ENDIF(VTK_OPENGL_HAS_OSMESA)
ENDIF(VTK_USE_MANGLED_MESA)
SET(Kit_SRCS ${Kit_SRCS} ${KitOpenGL_SRCS})

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)

# -----------------------------------------------------------------
# Start of common section

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR3(vtk${KIT}Instantiator KitInstantiator_SRCS
                       "${Kit_SRCS}"
                       VTK_${UKIT}_EXPORT
                       ${VTK_BINARY_DIR} "")
ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${Kit_EXTRA_SRCS} ${KitInstantiator_SRCS})

# Allow the user to customize their build with some local options
#
SET(LOCALUSERMACRODEFINED 0)
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL3(vtk${KIT}TCL KitTCL_SRCS 
                "${Kit_SRCS}"
                "${Kit_TCL_EXTRA_CMDS}")
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS} ${Kit_TCL_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} ${KIT_TCL_LIBS})
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}TCL)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON3(vtk${KIT}Python KitPython_SRCS "${Kit_SRCS}")
  IF (APPLE)
    ADD_LIBRARY(vtk${KIT}PythonD ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    ADD_LIBRARY(vtk${KIT}Python MODULE vtk${KIT}PythonInit.cxx)
    TARGET_LINK_LIBRARIES(vtk${KIT}PythonD vtk${KIT} ${KIT_PYTHON_LIBS}
                          ${VTK_PYTHON_LIBRARIES})
    TARGET_LINK_LIBRARIES(vtk${KIT}Python vtk${KIT}PythonD)
    IF(NOT VTK_INSTALL_NO_LIBRARIES)
      INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}PythonD)
    ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  ELSE (APPLE)
    ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    TARGET_LINK_LIBRARIES (vtk${KIT}Python vtk${KIT} ${KIT_PYTHON_LIBS})
  ENDIF(APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python ${VTK_PYTHON_LIBRARIES})
  ENDIF(WIN32)
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}Python)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA3(vtk${KIT}Java KitJava_SRCS "${Kit_SRCS}")
  ADD_LIBRARY(vtk${KIT}Java SHARED ${KitJava_SRCS} ${Kit_JAVA_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES(vtk${KIT}Java vtk${KIT} ${KIT_JAVA_LIBS})
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}Java)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES(vtk${KIT} ${KIT_LIBS})

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT})
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h ${Kit_SRCS})
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

VTK_EXPORT_KIT("${KIT}" "${UKIT}" "${Kit_SRCS}")

# If the user defined a custom macro, execute it now and pass in all the srcs
#
IF(LOCALUSERMACRODEFINED)
  LocalUserOptionsMacro( "${Kit_SRCS}"       "${Kit_EXTRA_SRCS}"
                         "${KitTCL_SRCS}"    "${Kit_TCL_EXTRA_SRCS}"
                         "${KitJava_SRCS}"   "${Kit_JAVA_EXTRA_SRCS}"
                         "${KitPython_SRCS}" "${Kit_PYTHON_EXTRA_SRCS}")
ENDIF(LOCALUSERMACRODEFINED)

# End of common section
# -----------------------------------------------------------------

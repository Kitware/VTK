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
vtkProjectedTetrahedraMapper.cxx
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
vtkVolumeTextureMapper3D.cxx
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
  vtkOpenGLVolumeTextureMapper3D.cxx
)

#-------------------------------------------------
# Stuff we need for the vtkVolumeTextureMapper3D
#-------------------------------------------------
ADD_EXECUTABLE(vtkVREncodeString vtkVREncodeString.cxx)
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
      vtkVREncodeString
      COMMAND ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/vtkVREncodeString
      ARGS ${res} ${src} ${file}
      )
    SET(OBJ_DEPS ${OBJ_DEPS} ${res})
  ENDFOREACH(file)
  SET_SOURCE_FILES_PROPERTIES(vtkOpenGLVolumeTextureMapper3D.cxx 
    PROPERTIES OBJECT_DEPENDS "${OBJ_DEPS}")
ENDIF(asm_files)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkVolumeRenderingToolkit.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/vtkVolumeRenderingToolkit.h )
        


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
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_SOURCE_DIR}/CMake/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

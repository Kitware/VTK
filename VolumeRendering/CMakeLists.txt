SET(KIT VolumeRendering)
SET(UKIT VOLUMERENDERING)

SET(KIT_TCL_LIBS vtkRenderingTCL vtkIOTCL)
SET(KIT_PYTHON_LIBS vtkRenderingPythonD vtkIOPythonD)
SET(KIT_JAVA_LIBS vtkRenderingJava vtkIOJava)
SET(KIT_LIBS vtkRendering vtkIO)
SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

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
vtkHAVSVolumeMapper.cxx
vtkProjectedTetrahedraMapper.cxx
vtkRayCastImageDisplayHelper.cxx
vtkRecursiveSphereDirectionEncoder.cxx
vtkSphericalDirectionEncoder.cxx
vtkVolumeMapper.cxx
vtkVolumeProMapper.cxx
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
  vtkOpenGLHAVSVolumeMapper.cxx   
  vtkOpenGLProjectedTetrahedraMapper.cxx
  vtkOpenGLRayCastImageDisplayHelper.cxx
  vtkOpenGLVolumeTextureMapper2D.cxx
  vtkOpenGLVolumeTextureMapper3D.cxx
)

#-------------------------------------------------
# Stuff we need for the vtkVolumeTextureMapper3D
#-------------------------------------------------

# The set of source files to be encoded.
SET(asm_files
  vtkHAVSVolumeMapper_kbufferVP
  vtkHAVSVolumeMapper_k2FP
  vtkHAVSVolumeMapper_k2BeginFP
  vtkHAVSVolumeMapper_k2EndFP
  vtkHAVSVolumeMapper_k6FP
  vtkHAVSVolumeMapper_k6BeginFP
  vtkHAVSVolumeMapper_k6EndFP
  vtkVolumeTextureMapper3D_FourDependentNoShadeFP
  vtkVolumeTextureMapper3D_FourDependentShadeFP
  vtkVolumeTextureMapper3D_OneComponentNoShadeFP
  vtkVolumeTextureMapper3D_OneComponentShadeFP
  vtkVolumeTextureMapper3D_TwoDependentNoShadeFP
  vtkVolumeTextureMapper3D_TwoDependentShadeFP
  )

# Create custom commands to encode each assembly file into a C string
# literal in a header file.

SET(asm_h_files)
FOREACH(file ${asm_files})
  SET(src ${VTK_SOURCE_DIR}/VolumeRendering/${file}.asm)
  SET(res ${VTK_BINARY_DIR}/VolumeRendering/${file}.cxx)
  SET(resh ${VTK_BINARY_DIR}/VolumeRendering/${file}.h)
  SET(asm_h_files ${asm_h_files} "${resh}")
  ADD_CUSTOM_COMMAND(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND ${VTK_ENCODESTRING_EXE}
    ARGS ${res} ${src} ${file} --build-header VTK_VOLUMERENDERING_EXPORT vtkSystemIncludes.h
    )
  SET(Kit_EXTRA_SRCS ${Kit_EXTRA_SRCS} ${res})
ENDFOREACH(file)

# needed by some external projects

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkVolumeRenderingToolkit.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/vtkVolumeRenderingToolkit.h )

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${asm_h_files}
    ${CMAKE_CURRENT_BINARY_DIR}/vtkVolumeRenderingToolkit.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

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
                 vtkMesaProjectedTetrahedraMapper.cxx
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

INCLUDE(${VTK_SOURCE_DIR}/CMake/FindVLI.cmake)

IF (VLI_LIBRARY_FOR_VP1000 AND VLI_INCLUDE_PATH_FOR_VP1000)
  OPTION (VTK_USE_VOLUMEPRO_1000 "Enable support for VolumePro 1000." ON)
ELSE (VLI_LIBRARY_FOR_VP1000 AND VLI_INCLUDE_PATH_FOR_VP1000)
  OPTION (VTK_USE_VOLUMEPRO_1000 "Enable support for VolumePro 1000." OFF)
ENDIF (VLI_LIBRARY_FOR_VP1000 AND VLI_INCLUDE_PATH_FOR_VP1000)

IF (VTK_USE_VOLUMEPRO_1000)
  SET (Kit_SRCS ${Kit_SRCS}
    vtkVolumeProVP1000Mapper.cxx
    )
  SET (KitOpenGL_SRCS ${KitOpenGL_SRCS}
    vtkOpenGLVolumeProVP1000Mapper.cxx
    )
  INCLUDE_DIRECTORIES (${VLI_INCLUDE_PATH_FOR_VP1000})
  SET (KIT_LIBS ${KIT_LIBS} ${VLI_LIBRARY_FOR_VP1000})
ENDIF (VTK_USE_VOLUMEPRO_1000)

MARK_AS_ADVANCED (
  VTK_USE_VOLUMEPRO_1000
  VLI_LIBRARY_FOR_VP1000
  VLI_INCLUDE_PATH_FOR_VP1000
  )

SET(Kit_SRCS ${Kit_SRCS} ${KitOpenGL_SRCS})

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

# Make sure vtkRendering is built before vtkVolumeRendering builds.
# This will make sure vtkgl.h exists.
ADD_DEPENDENCIES(vtkVolumeRendering vtkRendering)

# when crosscompiling vtkEncodeString is imported, don't add a dependency to it
# this should be handled by cmake so it doesn't have to be done here
IF(NOT CMAKE_CROSSCOMPILING)
  # Add target dependency to ensure the vtkEncodeString is built before
  # VolumeRendering is.
  ADD_DEPENDENCIES(vtk${KIT} vtkEncodeString)
ENDIF(NOT CMAKE_CROSSCOMPILING)

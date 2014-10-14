set(Module_SRCS
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
  vtkUnstructuredGridBunykRayCastFunction.cxx
  vtkUnstructuredGridHomogeneousRayIntegrator.cxx
  vtkUnstructuredGridLinearRayIntegrator.cxx
  vtkUnstructuredGridPartialPreIntegration.cxx
  vtkUnstructuredGridPreIntegration.cxx
  vtkUnstructuredGridVolumeMapper.cxx
  vtkUnstructuredGridVolumeRayCastFunction.cxx
  vtkUnstructuredGridVolumeRayCastIterator.cxx
  vtkUnstructuredGridVolumeRayCastMapper.cxx
  vtkUnstructuredGridVolumeRayIntegrator.cxx
  vtkUnstructuredGridVolumeZSweepMapper.cxx
  vtkVolumeMapper.cxx
  vtkVolumeOutlineSource.cxx
  vtkVolumePicker.cxx
  vtkVolumeRayCastCompositeFunction.cxx
  vtkVolumeRayCastFunction.cxx
  vtkVolumeRayCastIsosurfaceFunction.cxx
  vtkVolumeRayCastMIPFunction.cxx
  vtkVolumeRayCastMapper.cxx
  vtkVolumeRayCastSpaceLeapingImageFilter.cxx
)

set_source_files_properties(
  vtkDirectionEncoder
  vtkEncodedGradientEstimator
  vtkVolumeMapper
  vtkVolumeRayCastFunction
  vtkVolumeTextureMapper
  vtkUnstructuredGridVolumeMapper
  vtkUnstructuredGridVolumeRayCastFunction
  vtkUnstructuredGridVolumeRayCastIterator
  vtkUnstructuredGridVolumeRayIntegrator
  ABSTRACT
  )

# only some volume mappers are supported for the OpenGL2 backend
if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set_source_files_properties(vtkSmartVolumeMapper.cxx
    PROPERTIES COMPILE_FLAGS -DVTK_OPENGL2)
  set(Module_SRCS ${Module_SRCS}
    vtkGPUVolumeRayCastMapper.cxx
    )
else()
set(Module_SRCS ${Module_SRCS}
  vtkHAVSVolumeMapper.cxx
  vtkProjectedAAHexahedraMapper.cxx
  vtkProjectedTetrahedraMapper.cxx
  vtkSmartVolumeMapper.cxx
  vtkGPUVolumeRayCastMapper.cxx
  vtkVolumeTextureMapper2D.cxx
  vtkVolumeTextureMapper3D.cxx
  vtkVolumeTextureMapper.cxx
  )
endif()


vtk_module_library(vtkRenderingVolume ${Module_SRCS})

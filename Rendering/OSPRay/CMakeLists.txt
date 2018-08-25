set(classes
  vtkOSPRayActorNode
  vtkOSPRayAMRVolumeMapperNode
  vtkOSPRayCameraNode
  vtkOSPRayCompositePolyDataMapper2Node
  vtkOSPRayLightNode
  vtkOSPRayMaterialHelpers
  vtkOSPRayMaterialLibrary
  vtkOSPRayPass
  vtkOSPRayPolyDataMapperNode
  vtkOSPRayVolumeCache
  vtkOSPRayRendererNode
  vtkOSPRayTetrahedraMapperNode
  vtkOSPRayViewNodeFactory
  vtkOSPRayVolumeMapper
  vtkOSPRayVolumeMapperNode
  vtkOSPRayVolumeNode
  vtkOSPRayWindowNode)

vtk_object_factory_declare(
  BASE vtkOSPRayVolumeInterface
  OVERRIDE vtkOSPRayVolumeMapper)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGOSPRAY_EXPORT")

vtk_module_add_module(VTK::RenderingOSPRay
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})

option(VTKOSPRAY_ENABLE_DENOISER "build OSPRay Renderer using OpenImageDenoise")
if (VTKOSPRAY_ENABLE_DENOISER)
  find_package(OpenImageDenoise 0.8 REQUIRED)
  add_definitions(-DVTKOSPRAY_ENABLE_DENOISER)
  set(OPENIMAGEDENIOSE OpenImageDenoise)
endif()

# TODO: Send patches upstream to export targets from ospray.
find_package(ospray 1.8 REQUIRED)
vtk_module_link(VTK::RenderingOSPRay
  PUBLIC
    ${OSPRAY_LIBRARIES}
    ${OPENIMAGEDENIOSE}
    /home/carson/opt/oidn-0.8.1.x86_64.linux/lib/libOpenImageDenoise.so  #Carson: Hack
)

vtk_module_include(VTK::RenderingOSPRay
  PUBLIC
    ${OSPRAY_INCLUDE_DIRS}
    /home/carson/opt/oidn-0.8.1.x86_64.linux/include #Carson: Hack
)

#target_link_libraries(VTK::RenderingOSPRay OpenImageDenoise)

# TODO: FindOSPRay should do this.
# OSPRay_Core uses MMTime which is in it's own special library.
if (WIN32)
  vtk_module_link(VTK::RenderingOSPRay
    PRIVATE
      Winmm)
endif ()

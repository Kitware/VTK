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

# TODO: Send patches upstream to export targets from ospray.
find_package(ospray REQUIRED)
vtk_module_link(VTK::RenderingOSPRay
  PUBLIC
    ${OSPRAY_LIBRARIES})
vtk_module_include(VTK::RenderingOSPRay
  PUBLIC
    ${OSPRAY_INCLUDE_DIRS})

# TODO: FindOSPRay should do this.
# OSPRay_Core uses MMTime which is in it's own special library.
if (WIN32)
  vtk_module_link(VTK::RenderingOSPRay
    PRIVATE
      Winmm)
endif ()

find_package(OSPRay REQUIRED)

set(Module_SRCS
  vtkOSPRayActorNode.cxx
  vtkOSPRayAMRVolumeMapperNode.cxx
  vtkOSPRayVolumeNode.cxx
  vtkOSPRayCameraNode.cxx
  vtkOSPRayCompositePolyDataMapper2Node.cxx
  vtkOSPRayLightNode.cxx
  vtkOSPRayMaterialHelpers.cxx
  vtkOSPRayMaterialLibrary.cxx
  vtkOSPRayPass.cxx
  vtkOSPRayPolyDataMapperNode.cxx
  vtkOSPRayVolumeCache.cxx
  vtkOSPRayVolumeMapper.cxx
  vtkOSPRayVolumeMapperNode.cxx
  vtkOSPRayTetrahedraMapperNode.cxx
  vtkOSPRayRendererNode.cxx
  vtkOSPRayViewNodeFactory.cxx
  vtkOSPRayWindowNode.cxx
  )

include(vtkObjectFactory)
vtk_add_override(vtkOSPRayVolumeInterface vtkOSPRayVolumeMapper)
vtk_object_factory_configure("${vtk_module_overrides}")
list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

vtk_module_library(vtkRenderingOSPRay ${Module_SRCS})

target_link_libraries(${vtk-module} LINK_PUBLIC ${OSPRAY_LIBRARIES})
target_include_directories(${vtk-module} PUBLIC ${OSPRAY_INCLUDE_DIRS})

# OSPRay_Core uses MMTime which is in it's own special library.
if(WIN32)
  target_link_libraries(${vtk-module} LINK_PRIVATE Winmm)
endif()

include(vtkOpenGL)
vtk_opengl_link(${vtk-module})

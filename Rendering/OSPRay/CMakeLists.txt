find_package(OSPRay REQUIRED)
include_directories(${OSPRAY_INCLUDE_DIRS})

set(Module_SRCS
  vtkOSPRayActorNode.cxx
  vtkOSPRayVolumeNode.cxx
  vtkOSPRayCameraNode.cxx
  vtkOSPRayCompositePolyDataMapper2Node.cxx
  vtkOSPRayLightNode.cxx
  vtkOSPRayPass.cxx
  vtkOSPRayPolyDataMapperNode.cxx
  vtkOSPRayVolumeMapperNode.cxx
  vtkOSPRayRendererNode.cxx
  vtkOSPRayViewNodeFactory.cxx
  vtkOSPRayWindowNode.cxx
  )

vtk_module_library(vtkRenderingOSPRay ${Module_SRCS})

target_link_libraries(${vtk-module} LINK_PUBLIC ${OSPRAY_LIBRARIES})

# OSPRay_Core uses MMTime which is in it's own special library.
if(WIN32)
  target_link_libraries(${vtk-module} LINK_PRIVATE Winmm)
endif()

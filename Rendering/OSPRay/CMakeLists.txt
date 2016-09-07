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
  vtkOSPRayVolumeMapper.cxx
  vtkOSPRayVolumeMapperNode.cxx
  vtkOSPRayRendererNode.cxx
  vtkOSPRayViewNodeFactory.cxx
  vtkOSPRayWindowNode.cxx
  )

include(vtkObjectFactory)
vtk_add_override(vtkOSPRayVolumeInterface vtkOSPRayVolumeMapper)
vtk_object_factory_configure("${vtk_module_overrides}")
list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
set_source_files_properties(
  ${vtk-module}ObjectFactory.cxx
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkRenderingOSPRay ${Module_SRCS})

target_link_libraries(${vtk-module} LINK_PUBLIC ${OSPRAY_LIBRARIES})

# OSPRay_Core uses MMTime which is in it's own special library.
if(WIN32)
  target_link_libraries(${vtk-module} LINK_PRIVATE Winmm)
endif()

include(vtkOpenGL)
vtk_opengl_link(${vtk-module})

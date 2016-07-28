set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(OCULUSSDK REQUIRED)

find_package(SDL2 REQUIRED)

set(Module_SRCS
  vtkOculusCamera.cxx
  vtkOculusRenderer.cxx
  vtkOculusRenderWindow.cxx
  vtkOculusRenderWindowInteractor.cxx
  )

set_source_files_properties(
  vtkOculusCamera
  vtkOculusRenderer
  vtkOculusRenderWindow
  vtkOculusRenderWindowInteractor
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
)


# This option determines if the VR rendering code will become
# the default renderer
option(VTK_OCULUS_OBJECT_FACTORY
  "Make oculus the default render window for VTK" ON)

mark_as_advanced(VTK_OCULUS_OBJECT_FACTORY)

if (VTK_OCULUS_OBJECT_FACTORY)
  include(vtkObjectFactory)

  vtk_add_override(vtkCamera vtkOculusCamera)
  vtk_add_override(vtkRenderer vtkOculusRenderer)
  vtk_add_override(vtkRenderWindow vtkOculusRenderWindow)
  vtk_add_override(vtkRenderWindowInteractor vtkOculusRenderWindowInteractor)

  # add extra init code, we have to disable the existing
  # opengl overrides otherwise they will get picked first
  # instead of ours
  set (_vtk_override_includes ${_vtk_override_includes}
    "#include \"vtkOverrideInformation.h\"\n#include \"vtkOverrideInformationCollection.h\"\n")
  set (vtk-object-factory-extra-init "\
    vtkOverrideInformation *oi;\n\
    vtkOverrideInformationCollection  *oic = vtkOverrideInformationCollection::New();\n\
    vtkObjectFactory::GetOverrideInformation(\"vtkRenderWindow\",oic);\n\
    for (oic->InitTraversal(); oi = oic->GetNextItem(); )\n\
      {\n\
      oi->GetObjectFactory()->Disable(\"vtkRenderWindow\");\n\
      }\n\
    vtkObjectFactory::GetOverrideInformation(\"vtkCamera\",oic);\n\
    for (oic->InitTraversal(); oi = oic->GetNextItem(); )\n\
      {\n\
      oi->GetObjectFactory()->Disable(\"vtkCamera\");\n\
      }\n\
    vtkObjectFactory::GetOverrideInformation(\"vtkRenderWindowInteractor\",oic);\n\
    for (oic->InitTraversal(); oi = oic->GetNextItem(); )\n\
      {\n\
      oi->GetObjectFactory()->Disable(\"vtkRenderWindowInteractor\");\n\
      }\n\
    vtkObjectFactory::GetOverrideInformation(\"vtkRenderer\",oic);\n\
    for (oic->InitTraversal(); oi = oic->GetNextItem(); )\n\
      {\n\
      oi->GetObjectFactory()->Disable(\"vtkRenderer\");\n\
      }\n")
  vtk_object_factory_configure("${vtk_module_overrides}")

  set(Module_SRCS ${Module_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
    )
endif()

list(APPEND ${vtk-module}_SYSTEM_INCLUDE_DIRS ${OCULUS_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR})

vtk_module_library(${vtk-module} ${Module_SRCS})

vtk_module_link_libraries(${vtk-module} LINK_PRIVATE ${OCULUS_LIBRARIES}  ${SDL2_LIBRARY})

include(vtkOpenGL)
vtk_opengl_link(${vtk-module})

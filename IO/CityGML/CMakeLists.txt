set(Module_SRCS vtkCityGMLReader.cxx)
vtk_module_library(vtkIOCityGML ${Module_SRCS})

# pugixml does not set _INCLUDE_DIRS or _LIBRARIES
if(VTK_USE_SYSTEM_PUGIXML AND NOT pugixml_INCLUDE_DIRS)
  find_package(pugixml REQUIRED)
  get_target_property(pugixml_INCLUDE_DIRS pugixml INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(pugixml_LIBRARIES pugixml LOCATION)
  include_directories(${pugixml_INCLUDE_DIRS})
  vtk_module_link_libraries(vtkIOCityGML LINK_PRIVATE ${pugixml_LIBRARIES})
endif()

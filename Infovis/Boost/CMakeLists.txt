set(headers
  vtkVariantBoostSerialization.h)

vtk_module_add_module(VTK::InfovisBoost
  HEADERS ${headers}
  HEADER_ONLY)

vtk_module_find_package(
  PACKAGE     Boost
  COMPONENTS  serialization
  FORWARD_VERSION_REQ EXACT
  VERSION_VAR         "@Boost_MAJOR_VERSION@.@Boost_MINOR_VERSION@.@Boost_SUBMINOR_VERSION@")
vtk_module_link(VTK::InfovisBoost
  INTERFACE
    Boost::serialization)

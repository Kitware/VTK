vtk_module_third_party_external(
  PACKAGE catalyst
  COMPONENTS SDK
  TARGETS catalyst::catalyst
  VERSION "2.0")

# Forward Catalyst "conduit is external" property.
get_property(catalyst_conduit_is_external TARGET catalyst::catalyst
  PROPERTY CATALYST_WITH_EXTERNAL_CONDUIT)
vtk_module_set_property(VTK::catalyst
  PROPERTY  CATALYST_WITH_EXTERNAL_CONDUIT
  VALUE     "${catalyst_conduit_is_external}")
vtk_module_set_property(VTK::catalyst APPEND
  PROPERTY  EXPORT_PROPERTIES
  VALUE     CATALYST_WITH_EXTERNAL_CONDUIT)

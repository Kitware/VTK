# Enforce compatibility if Catalyst is also enabled
if (TARGET VTK::catalyst)
  vtk_module_get_property(VTK::catalyst
    PROPERTY CATALYST_WITH_EXTERNAL_CONDUIT
    VARIABLE catalyst_ext_conduit
  )
  if (NOT catalyst_ext_conduit)
    message(FATAL_ERROR
      "Conflict detected: VTK::conduit is enabled, but VTK::catalyst is "
      "configured to use its internal mangled Conduit. Please reconfigure "
      "with -DCATALYST_WITH_EXTERNAL_CONDUIT=ON so they can safely share "
      "the external Conduit library."
    )
  endif()
endif()

vtk_module_third_party_external(
  PACKAGE Conduit
  TARGETS conduit::conduit
)

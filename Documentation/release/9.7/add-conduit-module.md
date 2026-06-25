## Add a Conduit module

VTK modules can now depend on `VTK::conduit` to satisfy an external dependency
on Conduit. Since VTK also provides `VTK::catalyst`, which in turn depends on
either vendored or external Conduit, this module enforces an ORDER_DEPENDS
on `VTK::catalyst`, and fails with a helpful error message if `VTK::catalyst`
exists and was configured to use its internal Conduit.

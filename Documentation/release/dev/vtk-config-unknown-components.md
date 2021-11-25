## `find_package(VTK)` rejects unknown components

VTK will no longer report that components that are not part of its build are
`_FOUND` because they are not part of VTK's build (but happen to have matching
targets). The guarantees about the usability of `VTK::Component` when
`VTK_Component_FOUND` is set does not apply to them and are, as such, rejected.

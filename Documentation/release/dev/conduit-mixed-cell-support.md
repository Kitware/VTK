# Support for mixed cell unstructured grids in catalyst/conduit

After introduction of the mixed cell protocol in Conduit/MeshBlueprint
the support for mixed cells is now available in vtkConduitSource.
This means that grids consisting of mixtures of hexahedra, tetrahedra
and polyhedra can be defined. Other cell types (pyramids, wedges) need
to be defined as polyhedra, as MeshBlueprint has no native support for
these cell types.

For more details, see the ValidateMeshTypeMixed and ValidateMeshTypeMixed2D
tests in IO/CatalystConduit/Testing/Cxx/TestConduitSource.cxx and the examples
in conduit:src/libs/blueprint/conduit\_blueprint\_mesh\_examples.cpp on
https://github.com/LLNL/conduit

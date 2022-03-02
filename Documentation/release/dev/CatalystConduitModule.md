# Introduce Catalyst Conduit Module

[Conduit](https://llnl-conduit.readthedocs.io/en/latest/index.html) is a standard to describe and exchange simulation data, with HPC context in mind.

CatalystConduit is a new optional module, relying on [Catalyst Conduit](https://catalyst-in-situ.readthedocs.io/en/latest/introduction.html#relationship-with-conduit).
This is an external dependency so on have to get the [Catalyst lib](https://gitlab.kitware.com/paraview/catalyst) on their own.

We rely on Catalyst version of Conduit, to ensure compatibility with Catalyst implementations.

Main content:
 * `vtkConduitSource` : a VTK source that can generate a `vtkPartionedDataSet` or `vtkPartionedDataSetCollection` from a given Conduit node.
 It also has an option to output `vtkMultiBlockDataSet` for historical reason.
 * `vtkDataObjectToConduit` is a utility namespace to get a conduit node from any `vtkDataObject`.

 Also note that VTK now provide a stub implementation of Catalyst.

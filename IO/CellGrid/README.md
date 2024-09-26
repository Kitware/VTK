# VTK::IOCellGrid

This module contains a reader and writer for
[vtkCellGrid](https://vtk.org/doc/nightly/html/classvtkCellGrid.html) data
that uses human-readable JSON and/or binary MessagePack as the on-disk format.

Both the reader and writer classes use the
[vtkCellGridIOQuery](https://vtk.org/doc/nightly/html/classvtkCellGridIOQuery.html)
to accomplish their work.
This query can be set to serialize data to or deserialize data from an `nlohmann::json`
object, which is in turn written to or read from disk.
Note that this query is used by the `VTK::IOLegacy` module as well – it embeds
MessagePack-formatted data for cell-grids into the existing legacy VTK file format.

A query responder, [vtkDGIOResponder](https://vtk.org/doc/nightly/html/classvtkDGIOResponder.html),
is provided to process cell grids containing `vtkDGCell` instances.
If you add a new type of cell, you should register your own responder
to the `vtkCellGridIOQuery` class; once you do, your new cell type will
be supported for I/O.
Additionally, because ParaView uses the `VTK::IOLegacy` module to transfer data
between a client and rank-0 server processes, adding an I/O responder for your new
cell type will enable client-side rendering of remote data in ParaView.

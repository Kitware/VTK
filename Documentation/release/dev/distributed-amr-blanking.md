## Distributed overlapping AMR: read in parallel and blanked correctly

Reading a `vtkOverlappingAMR` in parallel had two problems. `vtkHDFReader` gave every rank the whole AMR, so the data
was duplicated on each rank. `vtkXMLUniformGridAMRReader` distributed the blocks, but a block was only blanked by the
children that the same rank had read, so cells covered by a child on another rank were left visible. This also showed up
downstream of the reader, in slices and in conversions to `vtkPartitionedDataSetCollection` or `vtkMultiBlockDataSet`.

Both come down to the fact that blanking only needs to know which blocks exist and their AMR boxes, and not where the
blocks are. To say so, `vtkAMRUtilities::BlankCells` has a new overload that takes a map from each block to the process
that holds it. `vtkParallelAMRUtilities::BlankCells` now gathers that map and calls it, so blanking lives in one place.

`vtkHDFReader` uses it to distribute the AMR: the boxes are read on all ranks, each block is read by one rank only
(blocks are dealt out in turn), and the other ranks get a null block, blanked as in a serial read.
`vtkXMLUniformGridAMRReader` uses it so that a distributed read is blanked exactly like a serial one.

To test a distributed AMR against a serial one, `vtkTestUtilities::CompareDataObjects` now also compares the metadata of
an AMR: its type, the number of blocks per level and the grid description, and for an overlapping AMR the AMR boxes, the
spacing of each level and the origin. The block bounds are left out, since they come from the blocks that are present.
Blocks that are null in either input were already skipped, so a distributed AMR can be compared with a serial one.

## Catalyst Conduit: Ghost cells on amrmesh domains

The Conduit `amrmesh` ingest now reads `state/nghosts` on each domain. A simulation can pass grids
that include its ghost cells, such as AMReX grow cells, with the coordset describing the grown grid.
The AMR boxes, origin and bounds come from the valid region, the ghost layers are placed at negative
and past-the-end extent indices, and they are flagged as `DUPLICATECELL` unless the domain provides
its own ghost array. Before, the coordset was taken as the AMR box, so grids with ghost cells were
shifted by the ghost layers and their blanking was wrong.

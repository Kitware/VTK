## Support nodal main multifab in vtkAMReXGridReader

`vtkAMReXGridReader` now detects the topology of the main AMReX multifab
and exposes its variables as point data when the multifab is nodal
(vertex-centered).  Previously the reader unconditionally registered
main-multifab variables as cell data, so plotfiles whose main multifab
was nodal were loaded with the wrong centering and incorrect block
dimensions.

The reader inspects the type vector of the level-0 box: all-zero entries
indicate cell-centered data and all-one entries indicate nodal data.
Face- and edge-centered main multifabs remain unsupported and are
flagged with a warning.

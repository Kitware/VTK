# OpenFOAM reader updates

- added support for reading of internal dimensioned fields

- updated string expansion for newer syntax:
  - `#sinclude` directive (silent include) - OpenFOAM-v1806
  - `<case>`, `<constant>`, ` <system>` tagss - OpenFOAM-v1806

- removed very old `include` compatibility keyword, which was poorly
  documented, rarely (if ever) used.
  Was deprecated in 2008 (OpenFOAM-v1.5)

- remove special handling of uniformFixedValue.
  The content that can be specified as a "uniformValue" has evolved
  well beyond a simple value that can be handled without using
  OpenFOAM libraries.


# OpenFOAM bugfixes / improvements

- ensure 32/64 bit information is propagated throughout to increase
  robustness for mixed precision workflows

- avoid 32bit overflow when constructing face and cell lists

- fix segfault or bad reads with large meshes (#18082)

- correctly handle multi-region cases without a default region (#18091)

- respect the boundary "inGroups" entry for selection of multiple
  boundary patches by group.

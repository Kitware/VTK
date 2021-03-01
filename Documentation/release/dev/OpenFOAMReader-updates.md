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


2021-03-01

# OpenFOAM bugfixes / improvements

- support floating-point dimensions entries (fixes #18103)

- respect point patch value fields (fixes #18125)

- properly handle empty zones

- improve zone addressing and handling. Basic support for face zones

- reduced disk IO when setting up cases,
  when scanning time directories, do cheaper string operations before
  filestat.
  This eliminates unnecessary checks for items that cannot be times anyhow.

- avoid lagrangian and region name ambiguity.

  Now prefix all non-default regions as "/regionName/...".
  The default region has now prefix. This eliminates any
  possible ambiguity if we happen to have slightly odd
  region names like "patch", "lagrangian", "internalMesh", ...

  - Drop old OpenFOAM 1.3 cloud naming.
    These were simply dumped into the time directory without any region
    qualifications. Defunct since about 2007.

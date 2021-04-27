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

2021-03-19

# OpenFOAM bugfixes / improvements

- respect point patch value fields (fixes #18125),
  use the correct "visit" order for the points

- remove hard-coded limits on polyhedral size

  For some systems, can have a so-called "single-cell" OpenFOAM mesh
  with a single polyhedral cell that has LOTS of faces.

  This can be used for mapping surface noise data to retain most of
  the geometry but entirely discarding the internal field. It can also
  potentially arise from finiteArea situations.


2021-04-22

# OpenFOAM bugfixes / improvements

- preserve uncollated lagrangian information (fixes #18179)

  Old version assumed lagrangian data are available on all processor
  sub-directories and used a central naming mechanism accordingly.

  This meant that missing clouds on higher processors would
  effectively block out clouds. Likely didn't work properly with
  clouds in multiple regions.

- cleanup/simplify processor directory detection

- avoid rescanning of time directories for decomposed
  Avoids repetitive calls to directory listings, which tend to really
  slow down loading.

- fixed handling of SetTimeValue() in vtkPOpenFOAMReader.
  Mostly affected direct calls from VTK (not from ParaView)

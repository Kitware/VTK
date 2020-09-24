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

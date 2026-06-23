## Viskores Implementation of an ANARI Rendering Device

This code provides an implementation of an ANARI rendering device that uses
functionality in Viskores worklets to generate images. This device can be used
both for Viskores applications as well as other ANARI clients that do not use
Viskores. Clients use the standard ANARI device connection to the device named
`viskores`, which should link to the `libanari_library_viskores.so` shared
library (or equivalent name for the OS being used).

This implementation is based on the Helium device stubs provided by the ANARI
SDK. Because the code in this directory is an ANARI implementation, the coding
style in this directory deviates from that typical elsewhere in Viskores. In
particular, things like file extensions, identifier names, and include
conventions are different than elsewhere in Viskores.

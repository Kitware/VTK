## Updated ANARI to 1.1 specification

The ANARI 1.0 specification has an extension named KHR_AREA_LIGHTS that defines
the radius and other parameters of lights that give the lights a physical size
and creates effects soft shadows. However, this extension was removed from ANARI
specification 1.1. The parameters have been moved to the extensions that define
lights (which are free to ignore them if soft shadows are not supported).

The ANARI 1.1 specification was first supported by the ANARI SDK version 0.16.

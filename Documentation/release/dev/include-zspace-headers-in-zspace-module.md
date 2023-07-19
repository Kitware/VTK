## Include zSpace headers inside zSpace module

Add zSpace headers and related license into the zSpace VTK module.
Since zSpace Core Compatibility libraries are only searched during runtime,
we also stop searching for them when VTK_ZSPACE_USE_COMPAT_SDK is ON (that is the default value).

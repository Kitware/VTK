## ANARI renderer warnings

The original implementation of the ANARI scenegraph checks for backend features and issues warnings
at each frame causing the output buffer to be filled up fast. This has been fixed so that missing
feature warnings are only issued once per renderer per device.

Various compiler warnings have been fixed in the ANARI integration classes.

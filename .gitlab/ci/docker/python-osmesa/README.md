## OSMesa wheel docker

This Docker image builds LLVM and OSMesa to facilitate making VTK wheels which
use OSMesa instead of the system OpenGL for rendering. Note that Docker images
are not friendly to "layering" shifts and while this does make *an* image, it
is not used in CI. Instead, the `extract.sh` script is used to pull the built
OSMesa from the image and used to make a tarball that is extracted at CI time.
This allows the Python base image to be updated with new Python releases
without having to rebuild our OSMesa all the time.

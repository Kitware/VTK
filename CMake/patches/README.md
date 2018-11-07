# CMake backports

This directory contains backports from newer CMake versions to help support
actually using older CMake versions for building VTK. The directory name is the
minimum version of CMake for which the contained files are no longer necessary.
For example, the files under the `3.12` directory are not needed for 3.12 or
3.13, but are for 3.11.

The `99` directory contains files which are not in any CMake version yet and
once they are, they should be moved to an appropriately named directory.

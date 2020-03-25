# update-minimum-cmake

VTK now requires CMake 3.12 to build and to consume. This is primarily pushed
by the need for the `SHELL:` syntax introduced in CMake 3.12 in order to fix
some issues with MPI support in some cases. However, it does allow for some
simplification since the module system may now assume that kits are always
available and also removes some other code to workaround behaviors changed or
fixed in newer CMake versions.

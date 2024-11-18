# Python for Wheels

This directory contains scripts to create artifacts for CI to use when building
wheels on macOS and Windows. The official binaries end up having behaviors that
are not suitable for CI purposes and therefore cannot be created in CI either,
so it is a manual process.

## macOS

The official macOS binaries are not "relocatable" and if unpacked somewhere
other than the expected location will fail to compile C API-using code because
the include paths and the like are hard-coded at some level. There is the
`relocatable-python` project that supports making a relocatable framework from
the official installers.

The `.tar.xz` that comes from the `prep_python_macos_$arch.sh` scripts is
directly usable as an upload.

## Windows

The official installers leave registry rubble around. The
`prep_python_windows.ps1` script creates a zip from the official installer by
installing into a local directory and then packing it up. Once the `.zip` is
created, move it to a Unix machine and unpack it into a directory with the
basename of the `.zip` file. Then repack the `.zip` to include this directory.
Also be sure to use the Windows Add/Remove Programs process to remove the
installation made during this process.

## Uploading

Upload using [the upload process][upload-files] to
`https://www.paraview.org/files/dependencies/python-for-wheels`.

[upload-files]: https://gitlab.kitware.com/utils/git-workflow/-/wikis/File-hosting

## Updating CI

Update the `.gitlab/ci/download_wheel_python.cmake` file with the version and
hashes as needed. When adding a new minor version, it is recommended to also
update any patch releases as available.

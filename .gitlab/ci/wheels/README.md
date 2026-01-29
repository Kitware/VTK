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

### Free-threaded builds

To create a free-threaded Python distribution (Python 3.13+), pass the `-t`
flag:

```sh
./prep_python_macos_arm64.sh 3.13.1 -t
```

This will produce a tarball with `t` in the name (e.g.,
`python-3.13.1t-macos-arm64.tar.xz`).

## Windows

The official installers leave registry rubble around. The
`prep_python_windows.ps1` script creates a zip from the official installer by
installing into a local directory and then packing it up. Once the `.zip` is
created, move it to a Unix machine and unpack it into a directory with the
basename of the `.zip` file. Then repack the `.zip` to include this directory.
Also be sure to use the Windows Add/Remove Programs process to remove the
installation made during this process.

### Free-threaded builds

To create a free-threaded Python distribution (Python 3.13+), pass the
`-freethreading` flag:

```powershell
.\prep_python_windows.ps1 -version 3.13.1 -freethreading
```

This will produce a zip with `t` in the name (e.g.,
`python-3.13.1t-windows-x86_64.zip`).

## Uploading

Upload using [the upload process][upload-files] to
`https://www.paraview.org/files/dependencies/python-for-wheels`.

[upload-files]: https://gitlab.kitware.com/utils/git-workflow/-/wikis/File-hosting

## Updating CI

Update the `.gitlab/ci/download_wheel_python.cmake` file with the version and
hashes as needed. When adding a new minor version, it is recommended to also
update any patch releases as available.

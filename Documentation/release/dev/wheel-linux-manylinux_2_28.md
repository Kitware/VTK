# Linux x86_64 wheels now target manylinux_2_28

VTK's Linux x86_64 Python wheels are now built in the
`quay.io/pypa/manylinux_2_28_x86_64` image instead of
`quay.io/pypa/manylinux2014_x86_64`. The wheels now require glibc 2.28 or
newer (e.g., RHEL/AlmaLinux/Rocky 8, Debian 10, Ubuntu 18.10 and later).
Systems with an older glibc, such as CentOS 7, can no longer install the
x86_64 wheels from PyPI.

This matches the Linux aarch64 wheels, which already target `manylinux_2_28`.

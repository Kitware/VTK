ARG ARCH
FROM quay.io/pypa/manylinux_2_28_$ARCH
MAINTAINER Ben Boeckel <ben.boeckel@kitware.com>

COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

COPY install_mesa_egl.sh /root/install_mesa_egl.sh
RUN sh /root/install_mesa_egl.sh

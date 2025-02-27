ARG TARGET=manylinux_2_28
ARG ARCH
FROM quay.io/pypa/${TARGET}_$ARCH
MAINTAINER Ben Boeckel <ben.boeckel@kitware.com>

COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

COPY install_mesa_egl.sh /root/install_mesa_egl.sh
RUN sh /root/install_mesa_egl.sh

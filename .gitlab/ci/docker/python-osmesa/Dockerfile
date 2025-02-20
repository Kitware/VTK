ARG TARGET=manylinux2014
ARG ARCH
FROM quay.io/pypa/${TARGET}_$ARCH
MAINTAINER Ben Boeckel <ben.boeckel@kitware.com>

COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

COPY install_osmesa.sh /root/install_osmesa.sh
RUN sh /root/install_osmesa.sh

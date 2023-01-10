FROM quay.io/pypa/manylinux2014_x86_64
MAINTAINER Ben Boeckel <ben.boeckel@kitware.com>

COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

COPY install_osmesa.sh /root/install_osmesa.sh
RUN sh /root/install_osmesa.sh

FROM fedora:39
MAINTAINER Ben Boeckel <ben.boeckel@kitware.com>

COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

COPY install_cuda.sh /root/install_cuda.sh
RUN sh /root/install_cuda.sh

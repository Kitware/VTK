FROM fedora:33
MAINTAINER Ben Boeckel <ben.boeckel@kitware.com>

COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

COPY install_cuda.sh /root/install_cuda.sh
RUN sh /root/install_cuda.sh

COPY install_adios.sh /root/install_adios.sh
RUN sh /root/install_adios.sh

COPY install_openvr.sh /root/install_openvr.sh
RUN sh /root/install_openvr.sh

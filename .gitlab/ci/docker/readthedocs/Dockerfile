FROM readthedocs/build:ubuntu-24.04-2024.06.17
LABEL maintainer="Ben Boeckel <ben.boeckel@kitware.com>"

USER root
COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

USER docs

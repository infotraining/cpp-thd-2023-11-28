FROM gcc:latest

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
     && apt-get -y install --no-install-recommends cmake curl zip unzip tar

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="3.25.3"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

RUN git clone https://github.com/Microsoft/vcpkg.git /opt/vcpkg

WORKDIR /opt/vcpkg

RUN ./bootstrap-vcpkg.sh && ./vcpkg integrate install && ./vcpkg integrate bash && echo 'export PATH=$PATH:/opt/vcpkg' >>~/.bashrc
RUN echo 'export VCPKG_ROOT=/opt/vcpkg' >>~/.bashrc

#--------------------------------
# Ninja

RUN git clone https://github.com/ninja-build/ninja.git /opt/ninja

WORKDIR /opt/ninja

RUN cmake -Bbuild-cmake && cmake --build build-cmake && chmod +x ./build-cmake/ninja && cp ./build-cmake/ninja /usr/bin

WORKDIR /root

# [Optional] Uncomment this section to install additional packages.
# RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
#     && apt-get -y install --no-install-recommends <your-package-list-here>

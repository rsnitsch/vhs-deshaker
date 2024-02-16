FROM debian:12 AS builder
LABEL org.opencontainers.image.authors="mail@robertnitsch.de"
LABEL org.opencontainers.image.version="2.0.0"
LABEL org.opencontainers.image.title="Fix horizontal shaking in digitized VHS videos"
LABEL org.opencontainers.image.url="https://github.com/rsnitsch/vhs-deshaker"

WORKDIR /vhs-deshaker

RUN apt-get update && apt-get -y upgrade && apt-get install -y build-essential \
    cmake \
    libopencv-dev \
    && rm -rf /var/lib/apt/lists/*

COPY CMakeLists.txt ./
COPY include ./include
COPY src ./src

RUN cmake -H. -B_build -DCMAKE_INSTALL_PREFIX=_install -DCMAKE_BUILD_TYPE=Release
RUN cmake --build _build/
RUN cmake --install _build/

# -----------------------------------

FROM debian:stable
RUN apt-get update && apt-get -y upgrade && apt-get install -y libopencv-core406 \
    libopencv-highgui406 \
    libopencv-imgproc406 \
    libopencv-videoio406 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=builder /vhs-deshaker/_install/bin/vhs-deshaker /usr/bin/vhs-deshaker
WORKDIR /videos
ENTRYPOINT ["vhs-deshaker"]

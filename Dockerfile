FROM alpine:latest

# Install toolchain
RUN apk update && \
    apk upgrade && \
    apk add git \
            python3 \
            py3-pip \
            cmake \
            build-base \
            gcc-arm-none-eabi \
            g++-arm-none-eabi \
            ninja

ENV PICO_SDK_PATH=/home/dev/external/pico-sdk

# syntax=docker/dockerfile:1
####
# Dockerfile: a Dockerfile for building Zephyr for CI purposes
####
# Stage 1: Download the Zephyr SDK
FROM ubuntu:24.04 AS zephyr-sdk-download
ARG ZEPHYR_SDK_VERSION=0.17.1
ARG TARGETPLATFORM

RUN apt-get -y update && apt-get -y install --no-install-recommends \
        build-essential \
        ca-certificates \
        wget \
        xz-utils && \
    rm -rf /var/lib/apt/lists/*

ADD scripts/install-zephyr-sdk /tmp/install-zephyr-sdk
RUN /tmp/install-zephyr-sdk "${ZEPHYR_SDK_VERSION}" "${TARGETPLATFORM}"

# Stage 2: Run SDK setup.sh
FROM ubuntu:24.04 AS zephyr-sdk-setup
ARG ZEPHYR_SDK_VERSION=0.17.1
COPY --from=zephyr-sdk-download /opt/zephyr-sdk-${ZEPHYR_SDK_VERSION} /opt/zephyr-sdk-${ZEPHYR_SDK_VERSION}

RUN apt-get -y update && apt-get -y install --no-install-recommends \
        ca-certificates \
        cmake \
        file \
        python3 \
        wget \
        xz-utils && \
    rm -rf /var/lib/apt/lists/*
RUN yes | /opt/zephyr-sdk-${ZEPHYR_SDK_VERSION}/setup.sh

# Stage 3: Assemble Zephyr Build Container from parts
FROM ubuntu:24.04
ARG ZEPHYR_SDK_VERSION=0.17.1

# Base requirements for an F Prime build not provided by requirements.txt
RUN apt-get -y update && apt-get -y install --no-install-recommends \
        build-essential \
        git \
        python3 \
        python3-pip \
        python3-setuptools \
        python3-venv \
        python3-wheel && \
    rm -rf /var/lib/apt/lists/*

COPY --from=zephyr-sdk-setup /opt/zephyr-sdk-${ZEPHYR_SDK_VERSION} /opt/zephyr-sdk-${ZEPHYR_SDK_VERSION}
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk-${ZEPHYR_SDK_VERSION}
